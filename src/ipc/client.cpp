#include "client.hpp"
#include "../helpers/fs.hpp"
#include "const.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace Lawnch::IPC {

Client::Client() {}

Client::~Client() {
  if (socket_fd != -1) {
    close(socket_fd);
  }
}

bool Client::connect() {
  socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd == -1)
    return false;

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;

  std::string path = Fs::get_socket_path("lawnch.sock").string();
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (::connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    close(socket_fd);
    socket_fd = -1;
    return false;
  }

  connected = true;
  return true;
}

bool Client::send_command(uint8_t cmd) {
  if (!connected)
    return false;

  if (write(socket_fd, &cmd, sizeof(cmd)) != sizeof(cmd)) {
    return false;
  }

  uint8_t response;
  if (read(socket_fd, &response, sizeof(response)) != sizeof(response)) {
    return false;
  }

  return response == Constants::RESP_OK;
}

bool Client::send_kill() { return send_command(Constants::CMD_KILL); }

bool Client::is_running() {
  if (!connect())
    return false;
  return send_command(Constants::CMD_PING);
}

} // namespace Lawnch::IPC
