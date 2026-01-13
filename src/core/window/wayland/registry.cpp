#include "registry.hpp"
#include <iostream>

namespace Lawnch::Core::Window::Wayland {

static const struct wl_registry_listener registry_listener = {
    .global = Registry::global_handler,
    .global_remove = Registry::global_remove_handler,
};

Registry::Registry(Display &d) : display(d) {
  struct wl_registry *reg = display.get_registry();
  if (reg) {
    wl_registry_add_listener(reg, &registry_listener, this);
    display.roundtrip();
  }
}

Registry::~Registry() {
  if (layer_shell)
    zwlr_layer_shell_v1_destroy(layer_shell);
  if (seat)
    wl_seat_destroy(seat);
  if (shm)
    wl_shm_destroy(shm);
  if (compositor)
    wl_compositor_destroy(compositor);
  if (output)
    wl_output_destroy(output);
}

bool Registry::has_required_globals() const {
  return compositor && shm && layer_shell && seat && output;
}

void Registry::global_handler(void *data, struct wl_registry *r, uint32_t name,
                              const char *iface, uint32_t ver) {
  auto self = static_cast<Registry *>(data);

  if (strcmp(iface, wl_compositor_interface.name) == 0) {
    self->compositor = (struct wl_compositor *)wl_registry_bind(
        r, name, &wl_compositor_interface, 4);
  } else if (strcmp(iface, wl_shm_interface.name) == 0) {
    self->shm =
        (struct wl_shm *)wl_registry_bind(r, name, &wl_shm_interface, 1);
  } else if (strcmp(iface, wl_seat_interface.name) == 0) {
    self->seat =
        (struct wl_seat *)wl_registry_bind(r, name, &wl_seat_interface, 7);
  } else if (strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0) {
    self->layer_shell = (struct zwlr_layer_shell_v1 *)wl_registry_bind(
        r, name, &zwlr_layer_shell_v1_interface, 1);
  } else if (strcmp(iface, wl_output_interface.name) == 0 && !self->output) {
    self->output =
        (struct wl_output *)wl_registry_bind(r, name, &wl_output_interface, 1);
  }
}

void Registry::global_remove_handler(void *data, struct wl_registry *registry,
                                     uint32_t name) {
  // TODO: handle removal
}

} // namespace Lawnch::Core::Window::Wayland
