#include "icon.hpp"
#include "../utils.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

IconManager &IconManager::get() {
  static IconManager instance;
  return instance;
}

IconManager::IconManager() { init_paths(); }

IconManager::~IconManager() {
  // Cleanup handles on exit
  for (auto &pair : cache) {
    if (pair.second)
      g_object_unref(pair.second);
  }
}

void IconManager::render_icon(cairo_t *cr, const std::string &name, double x,
                              double y, double size) {
  if (name.empty())
    return;

  RsvgHandle *handle = get_handle(name);
  if (!handle)
    return;

  RsvgRectangle viewport = {x, y, size, size};

  GError *error = nullptr;
  if (!rsvg_handle_render_document(handle, cr, &viewport, &error)) {
    if (error) {
      std::stringstream ss;
      ss << "Icon render error: " << error->message;
      Utils::log("IconManager", Utils::LogLevel::ERROR, ss.str());
      g_error_free(error);
    }
  }
}

void IconManager::init_paths() {
  std::vector<std::string> base_dirs;
  const char *home = getenv("HOME");
  const char *xdg_data = getenv("XDG_DATA_DIRS");

  if (home)
    base_dirs.push_back(std::string(home) + "/.local/share");

  if (xdg_data) {
    std::stringstream ss(xdg_data);
    std::string item;
    while (std::getline(ss, item, ':')) {
      if (!item.empty())
        base_dirs.push_back(item);
    }
  } else {
    base_dirs.push_back("/usr/local/share");
    base_dirs.push_back("/usr/share");
  }

  std::string theme_name = detect_gtk_theme();
  if (theme_name.empty())
    theme_name = "Adwaita";

  std::vector<std::string> theme_names = {theme_name, "Papirus", "hicolor"};

  for (const auto &base : base_dirs) {
    for (const auto &t_name : theme_names) {
      std::string p = base + "/icons/" + t_name;
      if (fs::exists(p) && fs::is_directory(p)) {
        theme_search_paths.push_back(p);
      }
    }
    std::string pix = base + "/pixmaps";
    if (fs::exists(pix))
      theme_search_paths.push_back(pix);
  }

  category_subdirs = {
      "scalable/apps",   "scalable/actions", "scalable/devices",
      "scalable/places", "scalable/status",  "scalable/categories",
      "48x48/apps",      "48x48/actions",    "48x48/devices",
      "48x48/places",    "32x32/apps",       "32x32/actions",
      "32x32/devices",   "32x32/places",     "24x24/apps",
      "24x24/actions",   "24x24/devices",    "24x24/places",
      "16x16/apps",      "16x16/actions",    "apps",
      "actions",         "mimetypes"};
}

std::string IconManager::detect_gtk_theme() {
  const char *home = getenv("HOME");
  if (!home)
    return "";

  std::string config_path = std::string(home) + "/.config/gtk-3.0/settings.ini";
  std::ifstream file(config_path);
  std::string line;

  if (file.is_open()) {
    while (std::getline(file, line)) {
      if (line.find("gtk-icon-theme-name") != std::string::npos) {
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
          std::string theme = line.substr(eq + 1);
          theme.erase(0, theme.find_first_not_of(" \t\r\n"));
          theme.erase(theme.find_last_not_of(" \t\r\n") + 1);
          return theme;
        }
      }
    }
  }
  return "";
}

RsvgHandle *IconManager::get_handle(const std::string &name) {
  if (cache.count(name))
    return cache[name];

  for (const auto &missing : missing_cache) {
    if (missing == name)
      return nullptr;
  }

  std::string full_path;

  if (name[0] == '/') {
    if (fs::exists(name))
      full_path = name;
  }

  else {
    full_path = find_icon_path(name);
  }

  if (full_path.empty()) {
    missing_cache.push_back(name); // Remember this failure
    return nullptr;
  }

  GError *error = NULL;
  RsvgHandle *handle = rsvg_handle_new_from_file(full_path.c_str(), &error);

  if (error) {
    std::stringstream ss;
    ss << "Failed to load icon '" << name << "': " << error->message;
    Utils::log("IconManager", Utils::LogLevel::ERROR, ss.str());
    g_error_free(error);
    missing_cache.push_back(name);
    return nullptr;
  }

  cache[name] = handle;
  return handle;
}

std::string IconManager::find_icon_path(const std::string &name) {
  for (const auto &theme_root : theme_search_paths) {

    for (const auto &sub : category_subdirs) {
      std::string p_svg = theme_root + "/" + sub + "/" + name + ".svg";
      if (fs::exists(p_svg))
        return p_svg;

      std::string p_png = theme_root + "/" + sub + "/" + name + ".png";
      if (fs::exists(p_png))
        return p_png;
    }

    std::string p_svg = theme_root + "/" + name + ".svg";
    if (fs::exists(p_svg))
      return p_svg;
    std::string p_png = theme_root + "/" + name + ".png";
    if (fs::exists(p_png))
      return p_png;
  }
  return "";
}