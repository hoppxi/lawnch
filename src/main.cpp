#include "app/application.hpp"
#include "cli/handler.hpp"
#include "ipc/server.hpp"
#include <iostream>
#include <memory>
#include <string>

using namespace Lawnch;

int main(int argc, char **argv) {
  try {
    CLI::Options options;
    std::unique_ptr<IPC::Server> ipc_server;

    auto result = CLI::Handler::handle_cli(argc, argv, options, ipc_server);

    if (result == CLI::HandleResult::ExitSuccess) {
      return 0;
    } else if (result == CLI::HandleResult::ExitFailure) {
      return 1;
    }

    App::Application app(std::move(ipc_server), options.config_path,
                         options.merge_config_path, options.verbose,
                         options.print_logs);
    app.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
