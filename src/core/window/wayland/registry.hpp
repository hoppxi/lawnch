#pragma once

#include "display.hpp"
#include <cstring>
#include <functional>
#include <wayland-client.h>

extern "C" {
#define namespace _namespace
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace
}

namespace Lawnch::Core::Window::Wayland {

class Registry {
public:
  Registry(Display &display);
  ~Registry();

  struct wl_compositor *compositor = nullptr;
  struct wl_shm *shm = nullptr;
  struct wl_seat *seat = nullptr;
  struct zwlr_layer_shell_v1 *layer_shell = nullptr;
  struct wl_output *output = nullptr;

  bool has_required_globals() const;

  static void global_handler(void *data, struct wl_registry *registry,
                             uint32_t name, const char *interface,
                             uint32_t version);
  static void global_remove_handler(void *data, struct wl_registry *registry,
                                    uint32_t name);

private:
  Display &display;
};

} // namespace Lawnch::Core::Window::Wayland
