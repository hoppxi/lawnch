#include "PluginBase.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace {

std::vector<lawnch::Result> do_url_query(const std::string &term) {
  std::vector<lawnch::Result> results;
  if (term.empty()) {
    lawnch::Result r;
    r.name = "URL";
    r.comment = "Enter URL to open";
    r.icon = "network-server";
    r.type = "info";
    results.push_back(r);
    return results;
  }

  std::string s = term;
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
            return !std::isspace(c);
          }));
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char c) { return !std::isspace(c); })
              .base(),
          s.end());

  auto error_result = [&](const std::string &msg) {
    lawnch::Result r;
    r.name = term;
    r.comment = msg;
    r.icon = "dialog-error";
    r.type = "info";
    results.push_back(r);
    return results;
  };

  if (s.empty()) {
    return error_result("Enter a valid URL");
  }

  bool has_space = s.find(' ') != std::string::npos;
  bool has_scheme = s.rfind("http://", 0) == 0 || s.rfind("https://", 0) == 0;
  bool looks_like_url =
      !has_space && (has_scheme || (s.find('.') != std::string::npos &&
                                    s.find("..") == std::string::npos));

  if (!looks_like_url) {
    return error_result("Enter a valid URL");
  }

  std::string url = has_scheme ? s : "https://" + s;

  lawnch::Result r;
  r.name = url;
  r.comment = "Open URL";
  r.icon = "network-server";
  r.command = "xdg-open \"" + url + "\"";
  r.type = "url";
  results.push_back(r);

  return results;
}

} // namespace

class UrlPlugin : public lawnch::Plugin {
public:
  std::vector<std::string> get_triggers() override { return {":url", ":u"}; }

  lawnch::Result get_help() override {
    lawnch::Result r;
    r.name = ":url / :u";
    r.comment = "Open a URL";
    r.icon = "network-server";
    r.type = "help";
    return r;
  }

  std::vector<lawnch::Result> query(const std::string &term) override {
    return do_url_query(term);
  }
};

LAWNCH_PLUGIN_DEFINE(UrlPlugin)
