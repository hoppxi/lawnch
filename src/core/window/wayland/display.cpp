#include "display.hpp"
#include <iostream>
#include <stdexcept>

namespace Lawnch::Core::Window::Wayland {

Display::Display() { connect(); }

Display::~Display() { disconnect(); }

void Display::connect() {
  display = wl_display_connect(nullptr);
  if (!display) {
    throw std::runtime_error("No Wayland display found");
  }
}

void Display::disconnect() {
  if (registry) {
    wl_registry_destroy(registry);
    registry = nullptr;
  }
  if (display) {
    wl_display_disconnect(display);
    display = nullptr;
  }
}

int Display::dispatch() { return wl_display_dispatch(display); }

int Display::roundtrip() { return wl_display_roundtrip(display); }

int Display::flush() { return wl_display_flush(display); }

int Display::prepare_read() { return wl_display_prepare_read(display); }

int Display::dispatch_pending() { return wl_display_dispatch_pending(display); }

void Display::cancel_read() { wl_display_cancel_read(display); }

int Display::read_events() { return wl_display_read_events(display); }

int Display::get_fd() const { return wl_display_get_fd(display); }

struct wl_registry *Display::get_registry() {
  if (!registry && display) {
    registry = wl_display_get_registry(display);
  }
  return registry;
}

} // namespace Lawnch::Core::Window::Wayland
