#pragma once

#include <cairo.h>
#include <librsvg/rsvg.h>
#include <list>
#include <map>
#include <string>
#include <vector>

struct IconTheme {
  std::string name;
  std::string full_path;
  std::vector<std::string> subdirs;
  std::vector<std::string> inherits;
};

class IconManager {
public:
  static IconManager &get();

  IconManager(const IconManager &) = delete;
  IconManager &operator=(const IconManager &) = delete;
  void render_icon(cairo_t *cr, const std::string &name, double x, double y,
                   double size);

private:
  std::map<std::string, RsvgHandle *> handle_cache;
  std::vector<std::string> missing_cache;

  std::vector<std::string> base_search_paths;
  std::list<IconTheme>
      active_theme_stack; // Priority: Current -> Inherited -> ... -> hicolor

  IconManager();
  ~IconManager();

  void init();
  void load_theme_recursive(const std::string &theme_name,
                            std::vector<std::string> &visited);

  std::string detect_system_theme();
  bool load_theme_definition(const std::string &theme_name,
                             IconTheme &out_theme);
  RsvgHandle *get_handle(const std::string &name);
  std::string find_icon_path(const std::string &name);
  std::vector<std::string> split_string(const std::string &str, char delimiter);
};