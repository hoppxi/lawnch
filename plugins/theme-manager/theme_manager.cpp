#include "lawnch_plugin_api.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

extern "C" {
#include <ini.h>
}

namespace fs = std::filesystem;

static char *c_strdup(const std::string &s) {
  char *c = new char[s.size() + 1];
  std::memcpy(c, s.c_str(), s.size() + 1);
  return c;
}

static std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

static std::string get_config_path() {
  const char *config_home = std::getenv("XDG_CONFIG_HOME");
  if (!config_home)
    return "";
  return fs::path(config_home) / "lawnch" / "config.ini";
}

static std::string g_current_theme = "unknown";

static int config_handler(void *user, const char *section, const char *name,
                          const char *value) {
  (void)user;
  if (std::string(section) == "layout" && std::string(name) == "theme") {
    g_current_theme = value;
  }
  return 1;
}

static std::string get_current_theme() {
  g_current_theme = "unknown"; // Reset
  std::string path = get_config_path();
  if (path.empty() || !fs::exists(path))
    return "unknown";

  if (ini_parse(path.c_str(), config_handler, nullptr) < 0) {
    return "unknown";
  }
  return g_current_theme;
}

void plugin_init(const LawnchHostApi *host) { (void)host; }

void plugin_destroy(void) {}

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":th", ":theme", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  LawnchResult *r = new LawnchResult;
  r->name = c_strdup(":th / :theme");
  r->comment = c_strdup("Manage themes (:c for current)");
  r->icon = c_strdup("preferences-desktop-theme");
  r->command = c_strdup("");
  r->type = c_strdup("help");
  r->preview_image_path = c_strdup("");
  return r;
}

static void parse_query(const std::string &term, bool &show_current,
                        std::string &filter) {
  std::string q = term;
  q.erase(0, q.find_first_not_of(" "));

  if (q == ":c" || q.find(":c ") == 0) {
    show_current = true;
    filter = "";
  } else {
    show_current = false;
    filter = q;
  }
}

LawnchResult *plugin_query(const char *term, int *num_results) {
  std::string full_term = term ? term : "";
  bool show_current = false;
  std::string filter;

  parse_query(full_term, show_current, filter);

  if (show_current) {
    std::string current = get_current_theme();
    *num_results = 1;
    LawnchResult *arr = new LawnchResult[1];
    arr[0].name = c_strdup(current);
    arr[0].comment = c_strdup("Current Active Theme");
    arr[0].icon = c_strdup("preferences-desktop-theme");
    arr[0].command = c_strdup("");
    arr[0].type = c_strdup("theme-current");
    arr[0].preview_image_path = c_strdup("");
    return arr;
  }

  std::vector<fs::path> search_paths;
  const char *data_home = std::getenv("XDG_DATA_HOME");
  if (data_home)
    search_paths.push_back(fs::path(data_home) / "lawnch" / "themes");
  search_paths.push_back("/usr/share/lawnch/themes");
  search_paths.push_back("/usr/local/share/lawnch/themes");

  std::vector<LawnchResult> results;
  std::string q_lower = to_lower(filter);

  std::string config_path = get_config_path();

  for (const auto &dir : search_paths) {
    if (fs::exists(dir) && fs::is_directory(dir)) {
      for (const auto &entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".ini") {
          std::string name = entry.path().stem().string();
          if (!q_lower.empty() &&
              to_lower(name).find(q_lower) == std::string::npos) {
            continue;
          }

          LawnchResult r;
          r.name = c_strdup(name);
          r.comment = c_strdup(dir.string());
          r.icon = c_strdup("preferences-desktop-theme");

          std::string cmd =
              "sed -i '/^\\[layout\\]/,/^\\[/ s/^theme[[:space:]]*=.*/theme=" +
              name + "/' " + config_path;

          r.command = c_strdup(cmd);
          r.type = c_strdup("theme");
          r.preview_image_path = c_strdup("");
          results.push_back(r);
        }
      }
    }
  }

  if (results.empty()) {
    *num_results = 0;
    return nullptr;
  }

  *num_results = results.size();
  LawnchResult *arr = new LawnchResult[*num_results];
  std::copy(results.begin(), results.end(), arr);
  return arr;
}

void plugin_free_results(LawnchResult *results, int n) {
  for (int i = 0; i < n; ++i) {
    delete[] results[i].name;
    delete[] results[i].comment;
    delete[] results[i].icon;
    delete[] results[i].command;
    delete[] results[i].type;
    delete[] results[i].preview_image_path;
  }
  delete[] results;
}

LawnchPluginVTable g_vtable = {.plugin_api_version = LAWNCH_PLUGIN_API_VERSION,
                               .init = plugin_init,
                               .destroy = plugin_destroy,
                               .get_triggers = plugin_get_triggers,
                               .get_help = plugin_get_help,
                               .query = plugin_query,
                               .free_results = plugin_free_results};

PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void) { return &g_vtable; }
