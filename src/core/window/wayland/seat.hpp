#pragma once

#include "../input/keyboard.hpp"
#include "../input/pointer.hpp"
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

namespace Lawnch::Core::Window::Wayland {

class Seat {
public:
  Seat(struct wl_seat *seat, Input::Keyboard &keyboard,
       Input::Pointer &pointer);
  ~Seat();

  int get_repeat_timer_fd() const { return repeat_timer_fd; }
  void handle_repeat();

  static void seat_capabilities_handler(void *data, struct wl_seat *seat,
                                        uint32_t caps);

  static void keyboard_keymap_handler(void *data, struct wl_keyboard *kb,
                                      uint32_t format, int32_t fd,
                                      uint32_t size);
  static void keyboard_enter_handler(void *data, struct wl_keyboard *kb,
                                     uint32_t serial, struct wl_surface *surf,
                                     struct wl_array *keys);
  static void keyboard_leave_handler(void *data, struct wl_keyboard *kb,
                                     uint32_t serial, struct wl_surface *surf);
  static void keyboard_key_handler(void *data, struct wl_keyboard *kb,
                                   uint32_t serial, uint32_t time, uint32_t key,
                                   uint32_t state);
  static void keyboard_modifiers_handler(void *data, struct wl_keyboard *kb,
                                         uint32_t serial, uint32_t dep,
                                         uint32_t lat, uint32_t loc,
                                         uint32_t grp);
  static void keyboard_repeat_info_handler(void *data, struct wl_keyboard *kb,
                                           int32_t rate, int32_t delay);

  static void pointer_enter_handler(void *data, struct wl_pointer *ptr,
                                    uint32_t serial, struct wl_surface *surf,
                                    wl_fixed_t sx, wl_fixed_t sy);
  static void pointer_leave_handler(void *data, struct wl_pointer *ptr,
                                    uint32_t serial, struct wl_surface *surf);
  static void pointer_motion_handler(void *data, struct wl_pointer *ptr,
                                     uint32_t time, wl_fixed_t sx,
                                     wl_fixed_t sy);
  static void pointer_button_handler(void *data, struct wl_pointer *ptr,
                                     uint32_t serial, uint32_t time,
                                     uint32_t button, uint32_t state);
  static void pointer_axis_handler(void *data, struct wl_pointer *ptr,
                                   uint32_t time, uint32_t axis,
                                   wl_fixed_t value);

private:
  void update_repeat_timer(uint32_t key, xkb_keysym_t sym, bool pressed);

private:
  struct wl_seat *seat;
  struct wl_keyboard *wl_keyboard = nullptr;
  struct wl_pointer *wl_pointer = nullptr;

  Input::Keyboard &keyboard_logic;
  Input::Pointer &pointer_logic;

  struct xkb_context *xkb_context = nullptr;
  struct xkb_keymap *xkb_keymap = nullptr;
  struct xkb_state *xkb_state = nullptr;

  int repeat_timer_fd = -1;
  int32_t repeat_rate = 25;
  int32_t repeat_delay = 600;

  uint32_t last_key = 0;
  xkb_keysym_t last_sym = 0;
};

} // namespace Lawnch::Core::Window::Wayland
