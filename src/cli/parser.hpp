#pragma once

#include <optional>
#include <string>

namespace Lawnch::CLI {

struct Options {
  bool help = false;
  bool version = false;
  bool kill = false;
  int verbosity = 3;
  std::optional<std::string> config_path;
};

class Parser {
public:
  static Options parse(int argc, char **argv);
  static void print_help();
  static void print_version();
};

} // namespace Lawnch::CLI
