#include "PluginBase.hpp"
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

class ThemeManagerPlugin : public lawnch::Plugin {
private:
  std::string current_theme_ = "unknown";

  static int config_handler(void *user, const char *section, const char *name,
                            const char *value) {
    ThemeManagerPlugin *self = static_cast<ThemeManagerPlugin *>(user);
    if (std::string(section) == "layout" && std::string(name) == "theme") {
      self->current_theme_ = value;
    }
    return 1;
  }

  std::string get_config_path() {
    if (host_ && host_->fs_api && host_->fs_api->get_config_home) {
      char *res = host_->fs_api->get_config_home();
      if (res) {
        fs::path p = fs::path(res) / "lawnch" / "config.ini";
        host_->fs_api->free_path(res);
        return p.string();
      }
    }
    // Fallback
    const char *config_home = std::getenv("XDG_CONFIG_HOME");
    if (!config_home)
      return "";
    return fs::path(config_home) / "lawnch" / "config.ini";
  }

  std::string get_current_theme() {
    current_theme_ = "unknown"; // Reset
    std::string path = get_config_path();
    if (path.empty() || !fs::exists(path))
      return "unknown";

    if (ini_parse(path.c_str(), config_handler, this) < 0) {
      return "unknown";
    }
    return current_theme_;
  }

  std::string to_lower(const std::string &s) {
    std::string ret = s;
    std::transform(ret.begin(), ret.end(), ret.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ret;
  }

  void parse_query(const std::string &term, bool &show_current,
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

public:
  std::vector<std::string> get_triggers() override { return {":th", ":theme"}; }

  lawnch::Result get_help() override {
    lawnch::Result r;
    r.name = ":th / :theme";
    r.comment = "Manage themes (:c for current)";
    r.icon = "preferences-desktop-theme";
    r.type = "help";
    return r;
  }

  std::vector<lawnch::Result> query(const std::string &term) override {
    std::string full_term = term;
    bool show_current = false;
    std::string filter;

    parse_query(full_term, show_current, filter);

    if (show_current) {
      std::string current = get_current_theme();
      lawnch::Result r;
      r.name = current;
      r.comment = "Current Active Theme";
      r.icon = "preferences-desktop-theme";
      r.type = "theme-current";
      return {r};
    }

    std::vector<fs::path> search_paths;
    const char *data_home = std::getenv("XDG_DATA_HOME");
    if (data_home)
      search_paths.push_back(fs::path(data_home) / "lawnch" / "themes");
    search_paths.push_back("/usr/share/lawnch/themes");
    search_paths.push_back("/usr/local/share/lawnch/themes");

    std::vector<lawnch::Result> results;
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

            lawnch::Result r;
            r.name = name;
            r.comment = dir.string();
            r.icon = "preferences-desktop-theme";

            std::string cmd = "sed -i '/^\\[layout\\]/,/^\\[/ "
                              "s/^theme[[:space:]]*=.*/theme=" +
                              name + "/' " + config_path;

            r.command = cmd;
            r.type = "theme";
            results.push_back(r);
          }
        }
      }
    }

    return results;
  }
};

LAWNCH_PLUGIN_DEFINE(ThemeManagerPlugin)
