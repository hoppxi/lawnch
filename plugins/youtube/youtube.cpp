#include "PluginBase.hpp"
#include <curl/curl.h>

namespace {
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

std::vector<lawnch::Result> do_youtube_query(const std::string &term) {
  std::string display = term.empty() ? "Type to search YouTube..." : term;
  std::string encoded_term = url_encode(term);
  std::string command =
      "xdg-open \"https://www.youtube.com/results?search_query=" +
      encoded_term + "\"";

  lawnch::Result r;
  r.name = display;
  r.comment = "Search YouTube (Enter to open)";
  r.icon = "multimedia-video-player";
  r.command = command;
  r.type = "youtube";

  return {r};
}

class YoutubePlugin : public lawnch::Plugin {
public:
  std::vector<std::string> get_triggers() override {
    return {":youtube", ":yt"};
  }

  lawnch::Result get_help() override {
    lawnch::Result r;
    r.name = ":youtube / :yt";
    r.comment = "Search YouTube";
    r.icon = "multimedia-video-player";
    r.type = "help";
    return r;
  }

  std::vector<lawnch::Result> query(const std::string &term) override {
    return do_youtube_query(term);
  }
};

} // namespace

LAWNCH_PLUGIN_DEFINE(YoutubePlugin)
