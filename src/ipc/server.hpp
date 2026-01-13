#pragma once

#include <functional>

namespace Lawnch::IPC {

class Server {
public:
  using KillCallback = std::function<void()>;

  Server();
  ~Server();

  bool try_lock();
  void init();
  void process_messages();
  int get_fd() const;

  void set_on_kill(KillCallback callback);

private:
  int server_fd = -1;
  int lock_fd = -1;
  KillCallback on_kill;

  void handle_client(int client_fd);
};

} // namespace Lawnch::IPC
