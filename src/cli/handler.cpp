#include "handler.hpp"
#include "parser.hpp"
#include "pm.hpp"
#include "tm.hpp"
#include "../core/config/validator.hpp"
#include "../helpers/fs.hpp"
#include "../ipc/client.hpp"
#include "../ipc/server.hpp"

#include <iostream>
#include <toml++/toml.hpp>

namespace Lawnch::CLI {

HandleResult Handler::handle_cli(int argc, char **argv, Options &options,
                                 std::unique_ptr<IPC::Server> &out_server) {
  if (argc > 1) {
    std::string cmd = argv[1];
    if (cmd == "help") {
      Parser::print_help();
      return HandleResult::ExitSuccess;
    }
    if (cmd == "pm") {
      std::vector<std::string> args(argv + 2, argv + argc);
      return PluginManager::handle_command(args) == 0 ? HandleResult::ExitSuccess
                                                      : HandleResult::ExitFailure;
    }
    if (cmd == "tm") {
      std::vector<std::string> args(argv + 2, argv + argc);
      return ThemeManager::handle_command(args) == 0 ? HandleResult::ExitSuccess
                                                     : HandleResult::ExitFailure;
    }
  }

  options = Parser::parse(argc, argv);

  if (options.help) {
    Parser::print_help();
    return HandleResult::ExitSuccess;
  }

  if (options.version) {
    Parser::print_version();
    return HandleResult::ExitSuccess;
  }

  if (options.validate_config) {
    try {
      std::string path_to_validate;
      if (options.config_path.has_value()) {
        path_to_validate = *options.config_path;
      } else {
        path_to_validate = (Fs::get_config_home() / "lawnch" / "config.toml").string();
      }

      auto tbl = toml::parse_file(path_to_validate);
      auto val_res = Core::Config::Validator::validateConfig(tbl, false);

      std::cout << "[Config Validation: " << path_to_validate << "]\n";
      if (val_res.success && val_res.warnings.empty()) {
        std::cout << "  \033[32m\u2705 Config is valid!\033[0m\n";
      } else {
        if (!val_res.errors.empty()) {
          std::cerr << "  \033[31m\u274C Found " << val_res.errors.size()
                    << " error(s):\033[0m\n";
          for (const auto &err : val_res.errors) {
            std::cerr << "    - " << err << "\n";
          }
        }
        if (!val_res.warnings.empty()) {
          std::cerr << "  \033[33m\u26A0\uFE0F Found " << val_res.warnings.size()
                    << " warning(s):\033[0m\n";
          for (const auto &warn : val_res.warnings) {
            std::cerr << "    - " << warn << "\n";
          }
        }
      }
    } catch (const toml::parse_error &e) {
      std::cerr << "  \033[31m\u274C Parse Error:\033[0m " << e.what() << "\n";
    }
    return HandleResult::ExitSuccess;
  }

  out_server = std::make_unique<IPC::Server>();
  bool input_lock = out_server->try_lock();

  // if no lock, another instance is running
  if (!input_lock) {
    if (options.kill) {
      IPC::Client ipc_client;
      if (ipc_client.send_kill()) {
        std::cout << "Lawnch instance killed." << std::endl;
      } else {
        std::cerr << "Failed to kill Lawnch instance." << std::endl;
        return HandleResult::ExitFailure;
      }
      return HandleResult::ExitSuccess;
    }

    std::cerr << "Lawnch is already running." << std::endl;
    return HandleResult::ExitSuccess;
  }

  if (options.kill) {
    std::cout << "No Lawnch instance found running." << std::endl;
    return HandleResult::ExitSuccess;
  }

  return HandleResult::Continue;
}

} // namespace Lawnch::CLI
