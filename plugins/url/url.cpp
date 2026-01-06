#include "../lawnch_plugin_api.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace {
char *c_strdup(const std::string &s) {
  char *cstr = new char[s.length() + 1];
  std::strcpy(cstr, s.c_str());
  return cstr;
}

std::vector<LawnchResult> do_url_query(const std::string &term) {
  std::vector<LawnchResult> results;
  if (term.empty()) {
    results.push_back({c_strdup("URL"), c_strdup("Enter URL to open"),
                       c_strdup("network-server"), c_strdup(""),
                       c_strdup("info")});
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
  if (s.empty()) {
    results.push_back({c_strdup(term), c_strdup("Enter a valid URL"),
                       c_strdup("dialog-error"), c_strdup(""),
                       c_strdup("info")});
    return results;
  }

  bool has_space = s.find(' ') != std::string::npos;
  bool has_scheme = s.rfind("http://", 0) == 0 || s.rfind("https://", 0) == 0;
  bool looks_like_url =
      !has_space && (has_scheme || (s.find('.') != std::string::npos &&
                                    s.find("..") == std::string::npos));

  if (!looks_like_url) {
    results.push_back({c_strdup(term), c_strdup("Enter a valid URL"),
                       c_strdup("dialog-error"), c_strdup(""),
                       c_strdup("info")});
    return results;
  }

  std::string url = has_scheme ? s : "https://" + s;

  results.push_back({c_strdup(url), c_strdup("Open URL"),
                     c_strdup("network-server"),
                     c_strdup("xdg-open \"" + url + "\""), c_strdup("url")});

  return results;
}

} // namespace

void plugin_init(const LawnchHostApi *host) {}
void plugin_destroy(void) {}

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":url", ":u", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  auto result = new LawnchResult;
  result->name = c_strdup(":url / :u");
  result->comment = c_strdup("Open a URL");
  result->icon = c_strdup("network-server");
  result->command = c_strdup("");
  result->type = c_strdup("help");
  return result;
}

void plugin_free_results(LawnchResult *results, int num_results) {
  if (!results)
    return;
  for (int i = 0; i < num_results; ++i) {
    delete[] results[i].name;
    delete[] results[i].comment;
    delete[] results[i].icon;
    delete[] results[i].command;
    delete[] results[i].type;
  }
  delete[] results;
}

LawnchResult *plugin_query(const char *term, int *num_results) {
  auto results_vec = do_url_query(term);
  *num_results = results_vec.size();
  if (*num_results == 0) {
    return nullptr;
  }
  auto result_arr = new LawnchResult[*num_results];
  for (size_t i = 0; i < results_vec.size(); ++i) {
    result_arr[i] = results_vec[i];
  }
  return result_arr;
}

static LawnchPluginVTable g_vtable = {.plugin_api_version =
                                          LAWNCH_PLUGIN_API_VERSION,
                                      .init = plugin_init,
                                      .destroy = plugin_destroy,
                                      .get_triggers = plugin_get_triggers,
                                      .get_help = plugin_get_help,
                                      .query = plugin_query,
                                      .free_results = plugin_free_results};

PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void) { return &g_vtable; }