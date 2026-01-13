#include "parser.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace Lawnch::CLI {

Options Parser::parse(int argc, char **argv) {
  Options options;
  std::vector<std::string> args(argv + 1, argv + argc);

  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--help" || args[i] == "-h") {
      options.help = true;
    } else if (args[i] == "--version" || args[i] == "-v") {
      options.version = true;
    } else if (args[i] == "--kill") {
      options.kill = true;
    } else if (args[i] == "--config" || args[i] == "-c") {
      if (i + 1 < args.size()) {
        options.config_path = args[i + 1];
        i++; // Skip next arg
      } else {
        std::cerr << "Error: --config requires a path argument" << std::endl;
      }
    }
  }
  return options;
}

void Parser::print_help() {
  std::cout
      << "Usage: lawnch [OPTIONS]\n\n"
      << "Options:\n"
      << "  -c, --config <path>   Specify a custom configuration file path\n"
      << "      --kill            Kill the running instance of Lawnch\n"
      << "  -h, --help            Show this help message\n"
      << "  -v, --version         Show version information\n";
}

void Parser::print_version() { std::cout << "Lawnch v0.1.0" << std::endl; }

} // namespace Lawnch::CLI
