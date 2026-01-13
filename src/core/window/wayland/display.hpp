#pragma once

#include <cerrno>
#include <stdexcept>
#include <wayland-client.h>

namespace Lawnch::Core::Window::Wayland {

class Display {
public:
  Display();
  ~Display();

  void connect();
  void disconnect();

  int dispatch();
  int roundtrip();
  int flush();
  int prepare_read();
  int dispatch_pending();
  void cancel_read();
  int read_events();

  int get_fd() const;
  wl_display *get() const { return display; }
  struct wl_registry *get_registry();

private:
  struct wl_display *display = nullptr;
  struct wl_registry *registry = nullptr;
};

} // namespace Lawnch::Core::Window::Wayland
