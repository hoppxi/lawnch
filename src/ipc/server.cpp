#include "server.hpp"
#include "../helpers/fs.hpp"
#include "../helpers/logger.hpp"
#include "const.hpp"

#include <fcntl.h>
#include <stdexcept>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace Lawnch::IPC {

Server::Server() {}

Server::~Server() {
  if (server_fd != -1) {
    close(server_fd);
    std::string socket_path = Fs::get_socket_path("lawnch.sock").string();
    std::filesystem::remove(socket_path);
  }
  if (lock_fd != -1) {
    close(lock_fd);
    std::string lock_path = Fs::get_socket_path("lawnch.lock").string();
    std::filesystem::remove(lock_path);
  }
}

bool Server::try_lock() {
  std::string lock_path = Fs::get_socket_path("lawnch.lock").string();
  lock_fd = open(lock_path.c_str(), O_RDWR | O_CREAT, 0666);
  if (lock_fd == -1) {
    return false;
  }

  if (flock(lock_fd, LOCK_EX | LOCK_NB) == -1) {
    close(lock_fd);
    lock_fd = -1;
    return false;
  }

  return true;
}

void Server::init() {
  server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd == -1) {
    throw std::runtime_error("Failed to create IPC socket");
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;

  std::string path = Fs::get_socket_path("lawnch.sock").string();
  std::filesystem::remove(path); // Ensure previous socket is gone
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    close(server_fd);
    throw std::runtime_error("Failed to bind IPC socket");
  }

  if (listen(server_fd, 5) == -1) {
    close(server_fd);
    throw std::runtime_error("Failed to listen on IPC socket");
  }

  Logger::log("IPC", Logger::LogLevel::INFO, "IPC Server listening on " + path);
}

int Server::get_fd() const { return server_fd; }

void Server::set_on_kill(KillCallback callback) { on_kill = callback; }

void Server::process_messages() {
  int client_fd = accept(server_fd, nullptr, nullptr);
  if (client_fd == -1)
    return;

  handle_client(client_fd);
  close(client_fd);
}

void Server::handle_client(int client_fd) {
  uint8_t cmd;
  ssize_t n = read(client_fd, &cmd, sizeof(cmd));

  if (n > 0) {
    uint8_t response = Constants::RESP_OK;

    switch (cmd) {
    case Constants::CMD_PING:
      // Just respond OK
      break;
    case Constants::CMD_KILL:
      Logger::log("IPC", Logger::LogLevel::INFO, "Received KILL command");
      if (on_kill) {
        on_kill();
      }
      break;
    default:
      response = Constants::RESP_ERROR;
      break;
    }
    if (write(client_fd, &response, sizeof(response)) == -1) {
      Logger::log("IPC", Logger::LogLevel::ERROR,
                  "Failed to write response to client");
    }
  }
}

} // namespace Lawnch::IPC
