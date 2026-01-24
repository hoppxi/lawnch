#include "PluginBase.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

bool contains_ignore_case(const std::string &str, const std::string &sub) {
  if (sub.empty())
    return true;
  auto it = std::search(str.begin(), str.end(), sub.begin(), sub.end(),
                        [](char ch1, char ch2) {
                          return std::toupper(ch1) == std::toupper(ch2);
                        });
  return (it != str.end());
}

} // namespace

class EmojiPlugin : public lawnch::Plugin {
private:
  struct PluginState {
    json emoji_cache;
    bool loaded = false;
  };
  PluginState state_;

  std::vector<lawnch::Result> do_emoji_query(const std::string &term) {
    if (!state_.loaded) {
      lawnch::Result err_result;
      err_result.name = "Error: emoji.json not loaded";
      err_result.comment = "Could not find or parse asset file";
      err_result.icon = "dialog-error";
      err_result.type = "error";
      return {err_result};
    }

    std::string term_str = term;
    std::vector<lawnch::Result> results_vec;

    for (const auto &item : state_.emoji_cache) {
      if (!item.is_object())
        continue;
      std::string text = item.value("text", "");
      std::string emoji = item.value("emoji", "");

      bool match = contains_ignore_case(text, term_str);
      if (!match && item.contains("keywords")) {
        for (const auto &keyword_item : item["keywords"]) {
          if (contains_ignore_case(keyword_item.get<std::string>(), term_str)) {
            match = true;
            break;
          }
        }
      }

      if (match) {
        lawnch::Result r;
        r.name = emoji + "  " + text;
        r.comment = item.value("category", "");
        r.command = "echo -n '" + emoji + "' | wl-copy";
        r.type = "emoji";
        results_vec.push_back(r);

        if (results_vec.size() >= 50)
          break;
      }
    }
    return results_vec;
  }

public:
  void init(const LawnchHostApi *host) override {
    Plugin::init(host);
    if (state_.loaded)
      return;
    const char *data_dir_path = host->get_data_dir(host);
    if (!data_dir_path) {
      if (host && host->log_api)
        host->log_api->log("EmojiPlugin", LAWNCH_LOG_ERROR,
                           "Host did not provide a data directory.");
      return;
    }

    fs::path emoji_path = fs::path(data_dir_path) / "emoji.json";

    std::ifstream f(emoji_path);
    if (f.good()) {
      try {
        f >> state_.emoji_cache;
        state_.loaded = true;
      } catch (const std::exception &e) {
        if (host && host->log_api) {
          std::string msg = std::string("Failed to parse ") +
                            emoji_path.string() + ": " + e.what();
          host->log_api->log("EmojiPlugin", LAWNCH_LOG_ERROR, msg.c_str());
        }
      }
    } else {
      if (host && host->log_api) {
        std::string msg = std::string("Failed to open ") + emoji_path.string();
        host->log_api->log("EmojiPlugin", LAWNCH_LOG_ERROR, msg.c_str());
      }
    }
  }

  void destroy() override {
    if (state_.loaded) {
      state_.emoji_cache.clear();
      state_.loaded = false;
    }
  }

  std::vector<std::string> get_triggers() override { return {":emoji", ":e"}; }

  lawnch::Result get_help() override {
    lawnch::Result r;
    r.name = ":emoji / :e";
    r.comment = "Search for emoji";
    r.icon = "face-smile";
    r.type = "help";
    return r;
  }

  std::vector<lawnch::Result> query(const std::string &term) override {
    return do_emoji_query(term);
  }
};

LAWNCH_PLUGIN_DEFINE(EmojiPlugin)
