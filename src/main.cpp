#include "app/application.hpp"
#include "cli/parser.hpp"
#include "ipc/client.hpp"
#include "ipc/server.hpp"
#include <iostream>
#include <memory>

using namespace Lawnch;

int main(int argc, char **argv) {
  try {
    auto options = CLI::Parser::parse(argc, argv);

    if (options.help) {
      CLI::Parser::print_help();
      return 0;
    }

    if (options.version) {
      CLI::Parser::print_version();
      return 0;
    }

    auto ipc_server = std::make_unique<IPC::Server>();
    bool input_lock = ipc_server->try_lock();

    // if no lock, another instance is running
    if (!input_lock) {
      if (options.kill) {
        IPC::Client ipc_client;
        if (ipc_client.send_kill()) {
          std::cout << "Lawnch instance killed." << std::endl;
        } else {
          std::cerr << "Failed to kill Lawnch instance." << std::endl;
          return 1;
        }
        return 0;
      }

      std::cerr << "Lawnch is already running." << std::endl;
      return 0;
    }

    if (options.kill) {
      std::cout << "No Lawnch instance found running." << std::endl;
      return 0;
    }

    App::Application app(std::move(ipc_server), options.config_path,
                         options.verbosity);
    app.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
