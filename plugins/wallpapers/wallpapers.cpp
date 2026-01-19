#include "lawnch_plugin_api.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct PluginState {
  std::string wallpaper_dir;
  std::string command_template;
  size_t max_results = 50;
};

static PluginState g_state;

static char *c_strdup(const std::string &s) {
  char *cstr = new char[s.size() + 1];
  std::memcpy(cstr, s.c_str(), s.size() + 1);
  return cstr;
}

static std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

static std::string expand_home(const std::string &path) {
  if (!path.empty() && path[0] == '~') {
    const char *home = std::getenv("HOME");
    if (home)
      return std::string(home) + path.substr(1);
  }
  return path;
}

static bool is_image_ext(const std::string &ext) {
  std::string e = to_lower(ext);
  return e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".webp" ||
         e == ".bmp";
}

static std::string build_command(const std::string &tpl,
                                 const std::string &path) {
  std::string cmd = tpl;
  size_t pos = cmd.find("{}");
  if (pos != std::string::npos) {
    cmd.replace(pos, 2, path);
  }
  return cmd;
}

void plugin_init(const LawnchHostApi *host) {
  const char *dir = host->get_config_value(host, "dir");
  if (dir && *dir) {
    g_state.wallpaper_dir = expand_home(dir);
  } else {
    g_state.wallpaper_dir = expand_home("~/Pictures/Wallpapers");
  }

  const char *max_res = host->get_config_value(host, "max_results");
  if (max_res) {
    try {
      g_state.max_results = std::stoul(max_res);
    } catch (...) {
      g_state.max_results = 50;
    }
  }

  // Command template
  const char *cmd = host->get_config_value(host, "command");
  if (cmd && *cmd) {
    g_state.command_template = cmd;
  } else {
    g_state.command_template =
        ""; // by default we don't exec on select, user
            // is expected to add dir to the config, e.g.
            // command awww --img "{}" and {} will be replaced with the filepath
  }
}

void plugin_destroy(void) {
  g_state.wallpaper_dir.clear();
  g_state.command_template.clear();
}

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":wall", ":wp", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  LawnchResult *r = new LawnchResult;
  r->name = c_strdup(":wall / :wp");
  r->comment = c_strdup("Set wallpaper");
  r->icon = c_strdup("preferences-desktop-wallpaper");
  r->command = c_strdup("");
  r->type = c_strdup("help");
  r->preview_image_path = c_strdup("");
  return r;
}

LawnchResult *plugin_query(const char *term, int *num_results) {
  *num_results = 0;

  if (!fs::exists(g_state.wallpaper_dir) ||
      !fs::is_directory(g_state.wallpaper_dir)) {
    return nullptr;
  }

  std::string search = to_lower(term);
  std::vector<LawnchResult> results;
  results.reserve(g_state.max_results);

  fs::recursive_directory_iterator it(
      g_state.wallpaper_dir, fs::directory_options::skip_permission_denied);

  for (const auto &entry : it) {
    if (results.size() >= g_state.max_results)
      break;

    if (!entry.is_regular_file())
      continue;

    const fs::path &p = entry.path();
    if (!is_image_ext(p.extension().string()))
      continue;

    std::string fname = to_lower(p.filename().string());
    if (!search.empty() && fname.find(search) == std::string::npos)
      continue;

    std::string abs_path = p.string();

    LawnchResult r;
    r.name = c_strdup(p.filename().string());
    r.comment = c_strdup(p.parent_path().string());
    r.icon = c_strdup("preferences-desktop-wallpaper");
    r.command = c_strdup(build_command(g_state.command_template, abs_path));
    r.type = c_strdup("wallpaper-plugin");
    r.preview_image_path = c_strdup((p.parent_path() / p.filename()).string());

    results.push_back(r);
  }

  if (results.empty())
    return nullptr;

  LawnchResult *out = new LawnchResult[results.size()];
  std::copy(results.begin(), results.end(), out);
  *num_results = results.size();
  return out;
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
    delete[] results[i].preview_image_path;
  }
  delete[] results;
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
