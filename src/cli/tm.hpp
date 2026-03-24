#pragma once

#include <string>
#include <vector>

namespace Lawnch::CLI {

class ThemeManager {
public:
  static int handle_command(const std::vector<std::string> &args);
  static void print_help();

private:
  static void install_theme(const std::string &path);
  static void uninstall_theme(const std::string &name);
  static void list_themes();

  static void install_preset(const std::string &path);
  static void uninstall_preset(const std::string &name);
  static void list_presets();

  static void switch_theme(const std::string &name);
  static void switch_preset(const std::string &name);
  static void current(bool is_theme, bool is_preset);
  static void validate(const std::string &path, bool is_theme);
};

} // namespace Lawnch::CLI
