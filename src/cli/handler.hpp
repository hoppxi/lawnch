#pragma once

#include "parser.hpp"
#include "../ipc/server.hpp"
#include <memory>

namespace Lawnch::CLI {

enum class HandleResult {
  Continue,
  ExitSuccess,
  ExitFailure
};

class Handler {
public:
  static HandleResult handle_cli(int argc, char **argv, Options &out_options,
                                 std::unique_ptr<IPC::Server> &out_server);
};

} // namespace Lawnch::CLI
