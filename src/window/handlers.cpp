#include "../config/config_manager.hpp"
#include "../helpers/logger.hpp"
#include "window.hpp"
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>

// These listeners are used within the handlers or registration
static const struct wl_shm_listener shm_listener = {
    .format = LauncherWindow::shm_format_handler,
};
static const struct wl_seat_listener seat_listener = {
    .capabilities = LauncherWindow::seat_capabilities_handler,
    .name = [](void *, auto...) {},
};
static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = LauncherWindow::keyboard_keymap_handler,
    .enter = LauncherWindow::keyboard_enter_handler,
    .leave = LauncherWindow::keyboard_leave_handler,
    .key = LauncherWindow::keyboard_key_handler,
    .modifiers = LauncherWindow::keyboard_modifiers_handler,
    .repeat_info = LauncherWindow::keyboard_repeat_info_handler,
};
static const struct wl_pointer_listener pointer_listener = {
    .enter = LauncherWindow::pointer_enter_handler,
    .leave = LauncherWindow::pointer_leave_handler,
    .motion = LauncherWindow::pointer_motion_handler,
    .button = LauncherWindow::pointer_button_handler,
    .axis = LauncherWindow::pointer_axis_handler,
    .frame = [](void *, auto...) {},
    .axis_source = [](void *, auto...) {},
    .axis_stop = [](void *, auto...) {},
};

void LauncherWindow::registry_global_handler(void *data, struct wl_registry *r,
                                             uint32_t name, const char *iface,
                                             uint32_t ver) {
  auto s = (LauncherWindow *)data;
  if (strcmp(iface, wl_compositor_interface.name) == 0)
    s->compositor =
        (wl_compositor *)wl_registry_bind(r, name, &wl_compositor_interface, 4);
  else if (strcmp(iface, wl_shm_interface.name) == 0) {
    s->shm = (wl_shm *)wl_registry_bind(r, name, &wl_shm_interface, 1);
    wl_shm_add_listener(s->shm, &shm_listener, s);
  } else if (strcmp(iface, wl_seat_interface.name) == 0) {
    s->seat = (wl_seat *)wl_registry_bind(r, name, &wl_seat_interface, 7);
    wl_seat_add_listener(s->seat, &seat_listener, s);
  } else if (strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0)
    s->layer_shell = (zwlr_layer_shell_v1 *)wl_registry_bind(
        r, name, &zwlr_layer_shell_v1_interface, 1);
  else if (strcmp(iface, wl_output_interface.name) == 0 && !s->output)
    s->output = (wl_output *)wl_registry_bind(r, name, &wl_output_interface, 1);
  else if (strcmp(iface, wl_seat_interface.name) == 0 && !s->seat) {
    s->seat = (wl_seat *)wl_registry_bind(r, name, &wl_seat_interface, 1);
    wl_seat_add_listener(s->seat, &seat_listener, s);
  }
}
void LauncherWindow::registry_global_remove_handler(void *,
                                                    struct wl_registry *,
                                                    uint32_t) {}
void LauncherWindow::shm_format_handler(void *, struct wl_shm *, uint32_t) {}
void LauncherWindow::seat_capabilities_handler(void *data, struct wl_seat *seat,
                                               uint32_t cap) {
  auto s = (LauncherWindow *)data;
  if ((cap & WL_SEAT_CAPABILITY_KEYBOARD) && !s->keyboard) {
    s->keyboard = wl_seat_get_keyboard(s->seat);
    wl_keyboard_add_listener(s->keyboard, &keyboard_listener, s);
  }
  if ((cap & WL_SEAT_CAPABILITY_POINTER) && !s->pointer) {
    s->pointer = wl_seat_get_pointer(s->seat);
    wl_pointer_add_listener(s->pointer, &pointer_listener, s);
  }
}
void LauncherWindow::keyboard_keymap_handler(void *data, struct wl_keyboard *,
                                             uint32_t format, int32_t fd,
                                             uint32_t size) {
  auto s = (LauncherWindow *)data;
  char *map = (char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (s->xkb_keymap)
    xkb_keymap_unref(s->xkb_keymap);

  s->xkb_keymap =
      xkb_keymap_new_from_string(s->xkb_context, map, XKB_KEYMAP_FORMAT_TEXT_V1,
                                 XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!s->xkb_keymap) {
    Logger::log("wayland", Logger::LogLevel::ERROR,
                "Failed to create XKB keymap");
  }

  munmap(map, size);
  close(fd);
  if (s->xkb_state)
    xkb_state_unref(s->xkb_state);

  if (s->xkb_keymap)
    s->xkb_state = xkb_state_new(s->xkb_keymap);
}
void LauncherWindow::keyboard_enter_handler(void *data, struct wl_keyboard *,
                                            uint32_t serial,
                                            struct wl_surface *,
                                            struct wl_array *) {
  ((LauncherWindow *)data)->serial = serial;
}
void LauncherWindow::keyboard_leave_handler(void *data, struct wl_keyboard *,
                                            uint32_t serial,
                                            struct wl_surface *) {
  auto s = (LauncherWindow *)data;
  s->serial = serial;
  s->update_repeat_timer(0, 0, false);
}
void LauncherWindow::keyboard_key_handler(void *data, struct wl_keyboard *,
                                          uint32_t serial, uint32_t time,
                                          uint32_t key, uint32_t state) {
  auto s = (LauncherWindow *)data;
  uint32_t code = key + 8;
  xkb_keysym_t sym = XKB_KEY_NoSymbol;
  if (s->xkb_state) {
    sym = xkb_state_key_get_one_sym(s->xkb_state, code);
  }
  s->update_repeat_timer(code, sym, state == WL_KEYBOARD_KEY_STATE_PRESSED);
}
void LauncherWindow::keyboard_modifiers_handler(void *data,
                                                struct wl_keyboard *, uint32_t,
                                                uint32_t dep, uint32_t lat,
                                                uint32_t loc, uint32_t grp) {
  auto s = (LauncherWindow *)data;
  if (s->xkb_state)
    xkb_state_update_mask(s->xkb_state, dep, lat, loc, 0, 0, grp);
}
void LauncherWindow::keyboard_repeat_info_handler(void *data,
                                                  struct wl_keyboard *,
                                                  int32_t rate, int32_t delay) {
  auto s = (LauncherWindow *)data;
  s->repeat_rate = rate;
  s->repeat_delay = delay;
}
void LauncherWindow::pointer_enter_handler(void *, struct wl_pointer *,
                                           uint32_t, struct wl_surface *,
                                           wl_fixed_t, wl_fixed_t) {}
void LauncherWindow::pointer_leave_handler(void *, struct wl_pointer *,
                                           uint32_t, struct wl_surface *) {}
void LauncherWindow::pointer_motion_handler(void *, struct wl_pointer *,
                                            uint32_t, wl_fixed_t, wl_fixed_t) {}
void LauncherWindow::pointer_button_handler(void *, struct wl_pointer *,
                                            uint32_t, uint32_t, uint32_t,
                                            uint32_t) {}
void LauncherWindow::pointer_axis_handler(void *data, struct wl_pointer *,
                                          uint32_t, uint32_t axis,
                                          wl_fixed_t value) {
  auto s = (LauncherWindow *)data;
  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
    double d = wl_fixed_to_double(value);
    int steps = 0;
    // Sensitivity threshold
    if (d > 5.0)
      steps = 1;
    else if (d < -5.0)
      steps = -1;

    if (steps != 0) {
      s->scroll_offset += steps;
      if (s->scroll_offset < 0)
        s->scroll_offset = 0;

      int max_idx = (int)s->current_results.size() - 1;
      if (s->scroll_offset > max_idx)
        s->scroll_offset = max_idx;

      s->render();
    }
  }
}

void LauncherWindow::layer_surface_configure_handler(
    void *data, struct zwlr_layer_surface_v1 *ls, uint32_t serial, uint32_t w,
    uint32_t h) {
  auto s = (LauncherWindow *)data;
  if (w == 0 || h == 0) {
    const auto &cfg = ConfigManager::Instance().Get();
    w = cfg.window_width;
    h = cfg.window_height;
  }
  zwlr_layer_surface_v1_ack_configure(ls, serial);
  s->render();
}
void LauncherWindow::layer_surface_closed_handler(
    void *data, struct zwlr_layer_surface_v1 *) {
  ((LauncherWindow *)data)->stop();
}
