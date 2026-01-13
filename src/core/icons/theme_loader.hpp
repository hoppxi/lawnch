#pragma once

#include <list>
#include <string>
#include <vector>

namespace Lawnch::Core::Icons {

struct IconTheme {
  std::string name;
  std::string full_path;
  std::vector<std::string> subdirs;
  std::vector<std::string> inherits;
};

class ThemeLoader {
public:
  ThemeLoader();
  ~ThemeLoader();

  std::string lookup_icon(const std::string &icon_name);
  void set_custom_theme(const std::string &name);
  bool init();

private:
  void ensure_initialized();

  bool initialized = false;
  std::string custom_theme_name;
  std::vector<std::string> base_search_paths;
  std::list<IconTheme> active_theme_stack;

  void load_theme_recursive(const std::string &theme_name,
                            std::vector<std::string> &visited);
  bool load_theme_definition(const std::string &theme_name,
                             IconTheme &out_theme);
  std::string detect_system_theme();
  std::string find_icon_path(const std::string &name);
  std::vector<std::string> split_string(const std::string &str, char delimiter);
};

} // namespace Lawnch::Core::Icons