#include "parser.hpp"
#include <iostream>
#include <lawnch_plugin_api.h>
#include <string>
#include <vector>

#ifndef LAWNCH_VERSION
#define LAWNCH_VERSION "unknown"
#endif

#ifndef LAWNCH_PLUGIN_API_VERSION_STR
#define LAWNCH_PLUGIN_API_VERSION_STR "unknown"
#endif

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
    } else if (args[i] == "--merge-config" || args[i] == "-m") {
      if (i + 1 < args.size()) {
        options.merge_config_path = args[i + 1];
        i++; // Skip next arg
      } else {
        std::cerr << "Error: --merge-config requires a path argument"
                  << std::endl;
      }
    } else if (args[i] == "--verbosity") {
      if (i + 1 < args.size()) {
        try {
          int val = std::stoi(args[i + 1]);
          if (val >= 1 && val <= 4) {
            options.verbosity = val;
          } else {
            std::cerr << "Error: --verbosity must be between 1 and 4"
                      << std::endl;
          }
        } catch (const std::exception &) {
          std::cerr << "Error: --verbosity requires a numeric argument"
                    << std::endl;
        }
        i++; // Skip next arg
      } else {
        std::cerr << "Error: --verbosity requires a level (1-4)" << std::endl;
      }
    }
  }
  return options;
}

void Parser::print_help() {
  std::cout
      << "Usage: lawnch [OPTIONS] [COMMAND]\n\n"
      << "Commands:\n"
      << "  pm                        Plugin Manager (Run 'lawnch pm help' for "
         "more)\n"
      << "  tm                        Theme Manager (Run 'lawnch tm help' for "
         "more)\n\n"
      << "Options:\n"
      << "  -c, --config <path>       Specify a custom configuration file "
         "path\n"
      << "  -m, --merge-config <path> Load an additional config file to "
         "override "
         "defaults\n"
      << "      --kill                Kill the running instance of Lawnch\n"
      << "      --verbosity <1-4>     Set log verbosity (1:Error, 2:Warn, "
         "3:Info, "
         "4:Debug)\n"
      << "  -h, --help                Show this help message\n"
      << "  -v, --version             Show version information\n";
}

void Parser::print_version() {
  std::cout << "lawnch v" << LAWNCH_VERSION << "\n"
            << "lawnch-plugin-api v" << LAWNCH_PLUGIN_API_VERSION_STR << "\n"
            << "lawnch-plugin-api-version " << LAWNCH_PLUGIN_API_VERSION
            << "\n";
}

} // namespace Lawnch::CLI
