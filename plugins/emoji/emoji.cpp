#include "../lawnch_plugin_api.h"
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
struct PluginState {
  json emoji_cache;
  bool loaded = false;
};

static PluginState g_state;

char *c_strdup(const std::string &s) {
  char *cstr = new char[s.length() + 1];
  std::strcpy(cstr, s.c_str());
  return cstr;
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

} // namespace

void plugin_init(const LawnchHostApi *host) {
  if (g_state.loaded)
    return;
  const char *data_dir_path = host->get_data_dir(host);
  if (!data_dir_path) {
    std::cerr << "Emoji plugin: Host did not provide a data directory."
              << std::endl;
    return;
  }

  fs::path emoji_path = fs::path(data_dir_path) / "assets" / "emoji.json";

  std::ifstream f(emoji_path);
  if (f.good()) {
    try {
      f >> g_state.emoji_cache;
      g_state.loaded = true;
    } catch (const std::exception &e) {
      std::cerr << "Emoji plugin: Failed to parse " << emoji_path << ": "
                << e.what() << std::endl;
    }
  } else {
    std::cerr << "Emoji plugin: Failed to open " << emoji_path << std::endl;
  }
}

void plugin_destroy(void) {
  if (g_state.loaded) {
    g_state.emoji_cache.clear();
    g_state.loaded = false;
  }
}

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":emoji", ":e", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  LawnchResult *result = new LawnchResult;
  result->name = c_strdup(":emoji / :e");
  result->comment = c_strdup("Search for emoji");
  result->icon = c_strdup("face-smile");
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
  if (!g_state.loaded) {
    *num_results = 1;
    LawnchResult *err_result = new LawnchResult;
    err_result->name = c_strdup("Error: emoji.json not loaded");
    err_result->comment = c_strdup("Could not find or parse asset file");
    err_result->icon = c_strdup("dialog-error");
    err_result->command = c_strdup("");
    err_result->type = c_strdup("error");
    return err_result;
  }

  std::string term_str = term;
  std::vector<LawnchResult> results_vec;

  for (const auto &item : g_state.emoji_cache) {
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
      results_vec.push_back(
          {c_strdup(emoji + "  " + text),
           c_strdup(item.value("category", "").c_str()), c_strdup(""),
           c_strdup(("echo -n '" + emoji + "' | wl-copy").c_str()),
           c_strdup("emoji")});
      if (results_vec.size() >= 50)
        break;
    }
  }

  *num_results = results_vec.size();
  if (*num_results == 0)
    return nullptr;

  LawnchResult *result_arr = new LawnchResult[*num_results];
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
