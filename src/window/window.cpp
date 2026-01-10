#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "window.hpp"
#include "../config/config_manager.hpp"
#include "../helpers/string.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <unistd.h>

// Listener structs are local to the compilation unit where they are used
// (Constructor)
static const struct wl_registry_listener registry_listener = {
    .global = LauncherWindow::registry_global_handler,
    .global_remove = LauncherWindow::registry_global_remove_handler,
};
static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = LauncherWindow::layer_surface_configure_handler,
    .closed = LauncherWindow::layer_surface_closed_handler,
};

LauncherWindow::LauncherWindow(Search &search) : search(search) {
  std::memset(buffers, 0, sizeof(buffers));
  repeat_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);

  display = wl_display_connect(nullptr);
  if (!display)
    throw std::runtime_error("No Wayland display found");

  registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener, this);
  wl_display_roundtrip(display);

  if (!compositor || !shm || !layer_shell || !seat || !output) {
    throw std::runtime_error(
        "Required Wayland protocols not supported by compositor");
  }

  xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  surface = wl_compositor_create_surface(compositor);

  const auto &cfg = ConfigManager::Instance().Get();

  layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      layer_shell, surface, output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
      "lawnch");

  zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener,
                                     this);
  zwlr_layer_surface_v1_set_size(layer_surface, cfg.window_width,
                                 cfg.window_height);

  uint32_t anchor_opts = 0;
  int m_top = 0, m_right = 0, m_bottom = 0, m_left = 0;
  std::string anchor = cfg.window_anchor;

  bool has_top = Lawnch::Str::contains_ic(anchor, "top");
  bool has_bottom = Lawnch::Str::contains_ic(anchor, "bottom");
  bool has_left = Lawnch::Str::contains_ic(anchor, "left");
  bool has_right = Lawnch::Str::contains_ic(anchor, "right");

  if (has_top) {
    anchor_opts |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
    m_top = cfg.window_margin_top;
  }
  if (has_bottom) {
    anchor_opts |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    m_bottom = cfg.window_margin_bottom;
  }
  if (has_left) {
    anchor_opts |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
    m_left = cfg.window_margin_left;
  }
  if (has_right) {
    anchor_opts |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    m_right = cfg.window_margin_right;
  }

  zwlr_layer_surface_v1_set_anchor(layer_surface, anchor_opts);
  zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, -1);
  zwlr_layer_surface_v1_set_margin(layer_surface, m_top, m_right, m_bottom,
                                   m_left);

  zwlr_layer_surface_v1_set_keyboard_interactivity(
      layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);

  wl_surface_commit(surface);
  wl_display_roundtrip(display);
}

LauncherWindow::~LauncherWindow() {
  if (repeat_timer_fd >= 0)
    close(repeat_timer_fd);
  destroy_buffer(buffers[0]);
  destroy_buffer(buffers[1]);
  if (xkb_state)
    xkb_state_unref(xkb_state);
  if (xkb_keymap)
    xkb_keymap_unref(xkb_keymap);
  if (xkb_context)
    xkb_context_unref(xkb_context);
  if (keyboard)
    wl_keyboard_destroy(keyboard);
  if (pointer)
    wl_pointer_destroy(pointer);
  if (layer_surface)
    zwlr_layer_surface_v1_destroy(layer_surface);
  if (surface)
    wl_surface_destroy(surface);
  if (layer_shell)
    zwlr_layer_shell_v1_destroy(layer_shell);
  if (seat)
    wl_seat_destroy(seat);
  if (shm)
    wl_shm_destroy(shm);
  if (compositor)
    wl_compositor_destroy(compositor);
  if (registry)
    wl_registry_destroy(registry);
  if (display)
    wl_display_disconnect(display);
}

void LauncherWindow::run() {
  struct pollfd fds[2];
  int wl_fd = wl_display_get_fd(display);

  while (running) {
    while (wl_display_prepare_read(display) != 0) {
      if (wl_display_dispatch_pending(display) < 0) {
        running = false;
        break;
      }
    }
    if (!running)
      break;

    int flush_ret = wl_display_flush(display);
    short wl_poll_events = POLLIN;
    if (flush_ret < 0 && errno == EAGAIN) {
      wl_poll_events |= POLLOUT;
    } else if (flush_ret < 0) {
      wl_display_cancel_read(display);
      running = false;
      break;
    }

    fds[0] = {wl_fd, wl_poll_events, 0};
    fds[1] = {repeat_timer_fd, POLLIN, 0};

    int ret = poll(fds, 2, -1);

    if (ret < 0 && errno != EINTR) {
      wl_display_cancel_read(display);
      break;
    }

    if (ret > 0 && (fds[0].revents & POLLIN)) {
      if (wl_display_read_events(display) < 0) {
        running = false;
        break;
      }
    } else {
      wl_display_cancel_read(display);
    }

    if (wl_display_dispatch_pending(display) < 0) {
      running = false;
      break;
    }

    if (ret > 0 && (fds[1].revents & POLLIN)) {
      uint64_t exp;
      if (read(repeat_timer_fd, &exp, sizeof(exp)) > 0) {
        if (last_key_code > 0)
          handle_key_press_action(last_key_code, last_key_sym);
      }
    }
  }
}

void LauncherWindow::stop() { running = false; }
