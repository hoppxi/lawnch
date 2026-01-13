#pragma once

#include <cstdint>

namespace Lawnch::IPC {

class Client {
public:
  Client();
  ~Client();

  bool connect();
  bool send_kill();
  bool is_running();

private:
  int socket_fd = -1;
  bool connected = false;

  bool send_command(uint8_t cmd);
};

} // namespace Lawnch::IPC
