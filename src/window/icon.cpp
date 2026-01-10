#include "icon.hpp"
#include "../helpers/fs.hpp"
#include "../helpers/logger.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

extern "C" {
#include "ini.h"
}

namespace fs = std::filesystem;

// ini handler
struct ThemeParseState {
  std::string inherits;
  std::string directories;
};

static int theme_ini_handler(void *user, const char *section, const char *name,
                             const char *value) {
  ThemeParseState *state = (ThemeParseState *)user;

  // We only care about the [Icon Theme] section
  std::string sec(section);
  if (sec == "Icon Theme") {
    std::string key(name);
    if (key == "Inherits") {
      state->inherits = value;
    } else if (key == "Directories") {
      state->directories = value;
    }
  }
  return 1;
}

struct ConfigFindState {
  std::string target_section;
  std::string target_key;
  std::string found_value;
};

static int config_find_handler(void *user, const char *section,
                               const char *name, const char *value) {
  ConfigFindState *state = (ConfigFindState *)user;

  if (state->target_section == section && state->target_key == name) {
    state->found_value = value;
    return 0;
  }
  return 1;
}

// Icon manager

IconManager &IconManager::get() {
  static IconManager instance;
  return instance;
}

IconManager::IconManager() { init(); }

IconManager::~IconManager() {
  for (auto &pair : handle_cache) {
    if (pair.second)
      g_object_unref(pair.second);
  }
}

void IconManager::render_icon(cairo_t *cr, const std::string &name, double x,
                              double y, double size) {
  if (name.empty())
    return;

  RsvgHandle *handle = get_handle(name);
  if (!handle) {
    return;
  }

  RsvgRectangle viewport = {x, y, size, size};
  GError *error = nullptr;

  if (!rsvg_handle_render_document(handle, cr, &viewport, &error)) {
    if (error) {
      std::stringstream ss;
      ss << "Icon render error for '" << name << "': " << error->message;
      Logger::log("IconManager", Logger::LogLevel::ERROR, ss.str());
      g_error_free(error);
    }
  }
}

void IconManager::init() {
  const char *home = getenv("HOME");
  const char *xdg_data = getenv("XDG_DATA_DIRS");

  if (home) {
    base_search_paths.push_back(std::string(home) + "/.icons");
    base_search_paths.push_back(std::string(home) + "/.local/share/icons");
  }

  if (xdg_data) {
    std::stringstream ss(xdg_data);
    std::string item;
    while (std::getline(ss, item, ':')) {
      if (!item.empty())
        base_search_paths.push_back(item + "/icons");
    }
  } else {
    base_search_paths.push_back("/usr/local/share/icons");
    base_search_paths.push_back("/usr/share/icons");
    // Flatpak support
    base_search_paths.push_back("/var/lib/flatpak/exports/share/icons");
  }

  std::string current_theme = detect_system_theme();
  if (current_theme.empty())
    current_theme = "hicolor";

  Logger::log("IconManager", Logger::LogLevel::INFO,
              "Current Theme: " + current_theme);

  std::vector<std::string> visited;
  load_theme_recursive(current_theme, visited);

  bool has_hicolor = false;
  for (const auto &t : active_theme_stack) {
    if (t.name == "hicolor")
      has_hicolor = true;
  }
  if (!has_hicolor) {
    load_theme_recursive("hicolor", visited);
  }
}

void IconManager::load_theme_recursive(const std::string &theme_name,
                                       std::vector<std::string> &visited) {
  // Stop infinite recursion
  for (const auto &v : visited) {
    if (v == theme_name)
      return;
  }
  visited.push_back(theme_name);

  IconTheme theme;
  if (load_theme_definition(theme_name, theme)) {
    active_theme_stack.push_back(theme);

    for (const auto &parent : theme.inherits) {
      load_theme_recursive(parent, visited);
    }
  }
}

bool IconManager::load_theme_definition(const std::string &theme_name,
                                        IconTheme &out_theme) {
  for (const auto &base : base_search_paths) {
    fs::path theme_path = fs::path(base) / theme_name;
    fs::path index_path = theme_path / "index.theme";

    if (fs::exists(index_path)) {
      out_theme.name = theme_name;
      out_theme.full_path = theme_path.string();

      ThemeParseState state;
      if (ini_parse(index_path.c_str(), theme_ini_handler, &state) < 0) {
        // Even if parse fails, we might have gotten partial
        // data. Still, mostly < 0 means file error.
      }

      out_theme.inherits = split_string(state.inherits, ',');
      out_theme.subdirs = split_string(state.directories, ',');

      for (auto &s : out_theme.inherits) {
        s.erase(0, s.find_first_not_of(" \t"));
        s.erase(s.find_last_not_of(" \t") + 1);
      }

      return true; // Found and loaded
    }
  }
  return false;
}

std::string IconManager::detect_system_theme() {
  std::string theme;
  fs::path home = Lawnch::Fs::get_home_path();
  fs::path config_home = Lawnch::Fs::get_config_home();

  // KDE (kdeglobals) [Icons] Theme
  {
    std::string kde_path = config_home / "kdeglobals";
    if (fs::exists(kde_path)) {
      ConfigFindState state = {"Icons", "Theme", ""};
      ini_parse(kde_path.c_str(), config_find_handler, &state);
      if (!state.found_value.empty())
        return state.found_value;
    }
  }

  // GTK 3.0 [Settings] gtk-icon-theme-name
  {
    std::string gtk3_path = config_home / "gtk-3.0/settings.ini";
    if (fs::exists(gtk3_path)) {
      ConfigFindState state = {"Settings", "gtk-icon-theme-name", ""};
      ini_parse(gtk3_path.c_str(), config_find_handler, &state);
      if (!state.found_value.empty())
        return state.found_value;
    }
  }

  {
    std::string gtk2_path = home / ".gtkrc-2.0";
    // Parsing .gtkrc-2.0 is harder (not standard INI). mostly covered by GTK3
    // or Defaults
    // It still could be parsed from .config/gtk-2.0 though.
  }

  const char *env_theme = getenv("GTK_THEME");
  if (env_theme)
    return std::string(env_theme);

  return "Adwaita";
}

RsvgHandle *IconManager::get_handle(const std::string &name) {
  if (handle_cache.count(name))
    return handle_cache[name];

  for (const auto &missing : missing_cache) {
    if (missing == name)
      return nullptr;
  }

  std::string full_path = find_icon_path(name);

  if (full_path.empty()) {
    missing_cache.push_back(name);
    return nullptr;
  }

  GError *error = NULL;
  RsvgHandle *handle = rsvg_handle_new_from_file(full_path.c_str(), &error);

  if (error) {
    std::stringstream ss;
    ss << "Failed to load icon '" << name << "' at " << full_path << ": "
       << error->message;
    Logger::log("IconManager", Logger::LogLevel::ERROR, ss.str());
    g_error_free(error);
    missing_cache.push_back(name);
    return nullptr;
  }

  handle_cache[name] = handle;
  return handle;
}

std::string IconManager::find_icon_path(const std::string &name) {
  if (name.front() == '/') {
    if (fs::exists(name))
      return name;
    return "";
  }

  for (const auto &theme : active_theme_stack) {

    for (const auto &subdir : theme.subdirs) {
      fs::path dir = fs::path(theme.full_path) / subdir;

      std::vector<fs::path> candidates;
      candidates.push_back(dir / (name + ".svg"));
      candidates.push_back(dir / (name + ".png"));

      for (const auto &p : candidates) {
        if (fs::exists(p))
          return p.string();
      }
    }

    fs::path root_svg = fs::path(theme.full_path) / (name + ".svg");
    if (fs::exists(root_svg))
      return root_svg.string();

    fs::path root_png = fs::path(theme.full_path) / (name + ".png");
    if (fs::exists(root_png))
      return root_png.string();
  }

  return "";
}

std::vector<std::string> IconManager::split_string(const std::string &str,
                                                   char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delimiter)) {
    if (!token.empty())
      tokens.push_back(token);
  }
  return tokens;
}
