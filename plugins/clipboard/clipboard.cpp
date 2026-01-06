#include "../lawnch_plugin_api.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
char *c_strdup(const std::string &s) {
  char *cstr = new char[s.length() + 1];
  std::strcpy(cstr, s.c_str());
  return cstr;
}

std::string exec(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

bool contains_ignore_case(const std::string &str, const std::string &sub) {
  if (sub.empty())
    return true;
  auto it = std::search(str.begin(), str.end(), sub.begin(), sub.end(),
                        [](char ch1, char ch2) {
                          return std::toupper(ch1) == std::toupper(ch2);
                        });
  return (it != str.end());
}

std::vector<LawnchResult> do_clip_query(const std::string &term) {
  std::vector<LawnchResult> results;
  std::string output;
  try {
    output = exec("cliphist list");
  } catch (const std::exception &e) {
    results.push_back({c_strdup("Clipboard unavailable"), c_strdup(e.what()),
                       c_strdup("dialog-error"), c_strdup(""),
                       c_strdup("info")});
    return results;
  }

  if (output.empty()) {
    results.push_back({c_strdup("Clipboard unavailable"),
                       c_strdup("cliphist not found or empty"),
                       c_strdup("dialog-error"), c_strdup(""),
                       c_strdup("info")});
    return results;
  }

  std::stringstream ss(output);
  std::string line;

  while (std::getline(ss, line)) {
    auto tab = line.find('\t');
    if (tab == std::string::npos)
      continue;

    std::string id = line.substr(0, tab);
    std::string content = line.substr(tab + 1);

    if (!term.empty() && !contains_ignore_case(content, term))
      continue;

    if (content.size() > 100)
      content = content.substr(0, 100) + "…";

    results.push_back({c_strdup(content), c_strdup("Cliphist ID: " + id),
                       c_strdup("edit-paste"),
                       c_strdup("cliphist decode " + id + " | wl-copy"),
                       c_strdup("clipboard")});
  }

  if (results.empty() && !term.empty()) {
    results.push_back({c_strdup("No matches"),
                       c_strdup("Clipboard history exists but no match"),
                       c_strdup("dialog-information"), c_strdup(""),
                       c_strdup("info")});
  }

  return results;
}

} // namespace

void plugin_init(const LawnchHostApi *host) {}
void plugin_destroy(void) {}

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":clip", ":c", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  LawnchResult *result = new LawnchResult;
  result->name = c_strdup(":clip / :c");
  result->comment = c_strdup("Clipboard history");
  result->icon = c_strdup("edit-paste");
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
  auto results_vec = do_clip_query(term);
  *num_results = results_vec.size();
  if (*num_results == 0) {
    return nullptr;
  }
  LawnchResult *result_arr = new LawnchResult[*num_results];
  // Manually copy since vector contains allocated char*
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