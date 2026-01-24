#include "PluginBase.hpp"
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <curl/curl.h>
#include <deque>
#include <map>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <vector>

namespace {

// Helper to encode URLs
std::string url_encode(const std::string &value) {
  CURL *curl = curl_easy_init();
  if (curl) {
    char *output = curl_easy_escape(curl, value.c_str(), value.length());
    if (output) {
      std::string result(output);
      curl_free(output);
      curl_easy_cleanup(curl);
      return result;
    }
    curl_easy_cleanup(curl);
  }
  return "";
}

// Curl Write Callback
size_t write_callback(void *contents, size_t size, size_t nmemb,
                      std::string *s) {
  size_t newLength = size * nmemb;
  try {
    s->append((char *)contents, newLength);
  } catch (std::bad_alloc &e) {
    return 0;
  }
  return newLength;
}

// Parse Google JSON: ["query", ["sug1", "sug2", ...], ...]
std::vector<std::string> parse_google_json(const std::string &json) {
  std::vector<std::string> suggestions;

  // Find the start of the second array (the suggestions list)
  // Pattern: [ "query", [ "sug1", "sug2"
  size_t outer_bracket = json.find('[');
  if (outer_bracket == std::string::npos)
    return suggestions;

  size_t inner_start = json.find('[', outer_bracket + 1);
  if (inner_start == std::string::npos)
    return suggestions;

  size_t inner_end = json.find(']', inner_start);
  if (inner_end == std::string::npos)
    return suggestions;

  std::string list_str =
      json.substr(inner_start + 1, inner_end - inner_start - 1);

  // Simple manual CSV parsing specifically for Google's format
  // We can't just split by comma because commas can be inside quotes.
  bool in_quote = false;
  std::string current;
  for (size_t i = 0; i < list_str.length(); ++i) {
    char c = list_str[i];
    if (c == '"') {
      in_quote = !in_quote;
    } else if (c == ',' && !in_quote) {
      if (!current.empty()) {
        suggestions.push_back(current);
        current.clear();
      }
    } else {
      current += c;
    }
  }
  if (!current.empty())
    suggestions.push_back(current);

  return suggestions;
}

} // namespace

class GooglePlugin : public lawnch::Plugin {
private:
  std::map<std::string, std::vector<std::string>> cache_;
  std::deque<std::string> query_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> running_{false};
  std::thread worker_thread_;

  void worker_loop() {
    while (running_) {
      std::string query;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !query_queue_.empty() || !running_; });

        if (!running_)
          break;

        query = query_queue_.front();
        query_queue_.pop_front();

        if (cache_.count(query))
          continue;
      }

      CURL *curl = curl_easy_init();
      if (curl) {
        std::string encoded = url_encode(query);
        std::string url = "http://suggestqueries.google.com/complete/"
                          "search?client=firefox&q=" +
                          encoded;
        std::string response_string;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK && !response_string.empty()) {
          auto sugs = parse_google_json(response_string);

          {
            std::lock_guard<std::mutex> lock(mutex_);
            cache_[query] = sugs;
          }
        }
      }
    }
  }

  std::vector<lawnch::Result> do_google_query(const std::string &term) {
    std::string query_str = term;
    std::string display =
        query_str.empty() ? "Type to search Google..." : query_str;

    std::vector<lawnch::Result> results;

    std::string encoded_term = url_encode(query_str);
    std::string command =
        "xdg-open \"https://www.google.com/search?q=" + encoded_term + "\"";

    lawnch::Result r;
    r.name = display;
    r.comment = "Search Google (Enter to open)";
    r.icon = "web-browser";
    r.command = command;
    r.type = "google";
    results.push_back(r);

    bool cache_hit = false;
    std::vector<std::string> cached_sugs;

    if (!query_str.empty()) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (cache_.count(query_str)) {
        cached_sugs = cache_[query_str];
        cache_hit = true;
      } else {
        query_queue_.push_back(query_str);
        cv_.notify_one();
      }
    }

    if (cache_hit) {
      for (const auto &sug : cached_sugs) {
        std::string sug_cmd =
            "xdg-open \"https://www.google.com/search?q=" + url_encode(sug) +
            "\"";

        lawnch::Result sr;
        sr.name = sug;
        sr.comment = "Google Suggestion";
        sr.icon = "system-search";
        sr.command = sug_cmd;
        sr.type = "google-suggest";
        results.push_back(sr);
      }
    }
    return results;
  }

public:
  void init(const LawnchHostApi *host) override {
    Plugin::init(host);
    curl_global_init(CURL_GLOBAL_ALL);
    running_ = true;
    worker_thread_ = std::thread(&GooglePlugin::worker_loop, this);
  }

  void destroy() override {
    running_ = false;
    cv_.notify_all();
    if (worker_thread_.joinable()) {
      worker_thread_.join();
    }
    curl_global_cleanup();
  }

  std::vector<std::string> get_triggers() override { return {":google", ":g"}; }

  lawnch::Result get_help() override {
    lawnch::Result r;
    r.name = ":google / :g";
    r.comment = "Search Google";
    r.icon = "web-browser";
    r.type = "help";
    return r;
  }

  std::vector<lawnch::Result> query(const std::string &term) override {
    return do_google_query(term);
  }
};

LAWNCH_PLUGIN_DEFINE(GooglePlugin)
