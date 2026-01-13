#pragma once

#include "../../config/config.hpp"
#include <functional>
#include <wayland-client.h>

extern "C" {
#define namespace _namespace
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace
}

namespace Lawnch::Core::Window::Wayland {

class LayerSurface {
public:
  LayerSurface(struct wl_compositor *compositor,
               struct zwlr_layer_shell_v1 *shell, struct wl_output *output);
  ~LayerSurface();

  void apply_config(const Config::Config &cfg);

  struct wl_surface *get_surface() const { return surface; }
  int get_width() const { return width; }
  int get_height() const { return height; }

  bool is_configured() const { return width > 0 && height > 0; }

  void commit(struct wl_buffer *buffer);
  void close();

  std::function<void(int w, int h)> on_configure;
  std::function<void()> on_closed;

  static void configure_handler(void *data, struct zwlr_layer_surface_v1 *ls,
                                uint32_t serial, uint32_t w, uint32_t h);
  static void closed_handler(void *data, struct zwlr_layer_surface_v1 *ls);

private:
  struct wl_surface *surface = nullptr;
  struct zwlr_layer_surface_v1 *layer_surface = nullptr;

  int width = 0;
  int height = 0;
};

} // namespace Lawnch::Core::Window::Wayland
