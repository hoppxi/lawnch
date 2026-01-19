#pragma once

#include <string>
#include <vector>

namespace Lawnch::CLI {

class ThemeManager {
public:
  static int handle_command(const std::vector<std::string> &args);
  static void print_help();

private:
  static void install(const std::string &path);
  static void install_all(const std::string &path);
  static void uninstall(const std::string &name);
  static void list();
  static void switch_theme(const std::string &name);
  static void validate(const std::string &path);
  static void current();
};

} // namespace Lawnch::CLI
