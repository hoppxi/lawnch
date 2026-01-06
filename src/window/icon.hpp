#pragma once

#include <cairo.h>
#include <librsvg/rsvg.h>
#include <map>
#include <string>
#include <vector>

class IconManager {
public:
  static IconManager &get();

  IconManager(const IconManager &) = delete;
  IconManager &operator=(const IconManager &) = delete;
  void render_icon(cairo_t *cr, const std::string &name, double x, double y,
                   double size);

private:
  std::map<std::string, RsvgHandle *> cache;
  std::vector<std::string> missing_cache;
  std::vector<std::string> theme_search_paths;
  std::vector<std::string> category_subdirs;

  IconManager();
  ~IconManager();

  void init_paths();
  std::string detect_gtk_theme();
  RsvgHandle *get_handle(const std::string &name);
  std::string find_icon_path(const std::string &name);
};