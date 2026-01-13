#include "theme_loader.hpp"
#include "../../helpers/fs.hpp"
#include "../../helpers/logger.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

extern "C" {
#include <ini.h>
}

namespace fs = std::filesystem;

namespace Lawnch::Core::Icons {

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

ThemeLoader::ThemeLoader() {}
ThemeLoader::~ThemeLoader() {}

void ThemeLoader::set_custom_theme(const std::string &name) {
  custom_theme_name = name;
  initialized = false;
}

bool ThemeLoader::init() {
  active_theme_stack.clear();
  base_search_paths = ::Lawnch::Fs::get_icon_dirs();

  std::string current_theme = custom_theme_name;

  if (current_theme.empty()) {
    current_theme = detect_system_theme();
  }

  if (current_theme.empty()) {
    current_theme = "hicolor";
  }

  ::Lawnch::Logger::log("ThemeLoader", ::Lawnch::Logger::LogLevel::INFO,
                        "Loading Theme: " + current_theme);

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

  initialized = true;
  return true;
}

void ThemeLoader::ensure_initialized() {
  if (!initialized) {
    init();
  }
}

void ThemeLoader::load_theme_recursive(const std::string &theme_name,
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

bool ThemeLoader::load_theme_definition(const std::string &theme_name,
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

std::string ThemeLoader::detect_system_theme() {
  std::string theme;
  fs::path home = ::Lawnch::Fs::get_home_path();
  fs::path config_home = ::Lawnch::Fs::get_config_home();

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

  // TODO: parse GTK 2.0 theme

  const char *env_theme = getenv("GTK_THEME");
  if (env_theme)
    return std::string(env_theme);

  return "hicolor";
}

std::string ThemeLoader::lookup_icon(const std::string &icon_name) {
  ensure_initialized();
  return find_icon_path(icon_name);
}

std::string ThemeLoader::find_icon_path(const std::string &name) {
  if (name.empty())
    return "";
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

  // Fallback /usr/share/pixmaps
  fs::path pixmap = fs::path("/usr/share/pixmaps") / (name + ".svg");
  if (fs::exists(pixmap))
    return pixmap.string();
  pixmap = fs::path("/usr/share/pixmaps") / (name + ".png");
  if (fs::exists(pixmap))
    return pixmap.string();

  return "";
}

std::vector<std::string> ThemeLoader::split_string(const std::string &str,
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

} // namespace Lawnch::Core::Icons
