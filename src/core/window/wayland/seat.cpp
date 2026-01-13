#include "seat.hpp"
#include "../../../helpers/logger.hpp"
#include <cstring>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace Lawnch::Core::Window::Wayland {

static const struct wl_seat_listener seat_listener = {
    .capabilities = Seat::seat_capabilities_handler,
    .name = [](void *, auto...) {},
};

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = Seat::keyboard_keymap_handler,
    .enter = Seat::keyboard_enter_handler,
    .leave = Seat::keyboard_leave_handler,
    .key = Seat::keyboard_key_handler,
    .modifiers = Seat::keyboard_modifiers_handler,
    .repeat_info = Seat::keyboard_repeat_info_handler,
};

static const struct wl_pointer_listener pointer_listener = {
    .enter = Seat::pointer_enter_handler,
    .leave = Seat::pointer_leave_handler,
    .motion = Seat::pointer_motion_handler,
    .button = Seat::pointer_button_handler,
    .axis = Seat::pointer_axis_handler,
    .frame = [](void *, auto...) {},
    .axis_source = [](void *, auto...) {},
    .axis_stop = [](void *, auto...) {},
};

Seat::Seat(struct wl_seat *s, Input::Keyboard &k, Input::Pointer &p)
    : seat(s), keyboard_logic(k), pointer_logic(p) {

  xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  repeat_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);

  wl_seat_add_listener(seat, &seat_listener, this);
}

Seat::~Seat() {
  if (wl_keyboard)
    wl_keyboard_destroy(wl_keyboard);
  if (wl_pointer)
    wl_pointer_destroy(wl_pointer);
  if (xkb_state)
    xkb_state_unref(xkb_state);
  if (xkb_keymap)
    xkb_keymap_unref(xkb_keymap);
  if (xkb_context)
    xkb_context_unref(xkb_context);
  if (repeat_timer_fd >= 0)
    close(repeat_timer_fd);
}

void Seat::handle_repeat() {
  uint64_t exp;
  if (read(repeat_timer_fd, &exp, sizeof(exp)) > 0) {
    if (last_key > 0) {
      keyboard_logic.handle_key(last_key, last_sym, xkb_state);
    }
  }
}

void Seat::update_repeat_timer(uint32_t key, xkb_keysym_t sym, bool pressed) {
  if (!pressed) {
    last_key = 0;
    struct itimerspec its = {};
    timerfd_settime(repeat_timer_fd, 0, &its, NULL);
    return;
  }

  last_key = key;
  last_sym = sym;

  struct itimerspec its;
  its.it_value.tv_sec = repeat_delay / 1000;
  its.it_value.tv_nsec = (repeat_delay % 1000) * 1000000;
  int interval_ms = (repeat_rate > 0) ? (1000 / repeat_rate) : 100;
  its.it_interval.tv_sec = interval_ms / 1000;
  its.it_interval.tv_nsec = (interval_ms % 1000) * 1000000;
  timerfd_settime(repeat_timer_fd, 0, &its, NULL);
}

void Seat::seat_capabilities_handler(void *data, struct wl_seat *seat,
                                     uint32_t caps) {
  auto self = static_cast<Seat *>(data);

  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !self->wl_keyboard) {
    self->wl_keyboard = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(self->wl_keyboard, &keyboard_listener, self);
  }
  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !self->wl_pointer) {
    self->wl_pointer = wl_seat_get_pointer(seat);
    wl_pointer_add_listener(self->wl_pointer, &pointer_listener, self);
  }
}

void Seat::keyboard_keymap_handler(void *data, struct wl_keyboard *,
                                   uint32_t format, int32_t fd, uint32_t size) {
  auto self = static_cast<Seat *>(data);
  char *map = (char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map == MAP_FAILED) {
    close(fd);
    return;
  }

  if (self->xkb_keymap)
    xkb_keymap_unref(self->xkb_keymap);
  self->xkb_keymap = xkb_keymap_new_from_string(self->xkb_context, map,
                                                XKB_KEYMAP_FORMAT_TEXT_V1,
                                                XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(map, size);
  close(fd);

  if (self->xkb_state)
    xkb_state_unref(self->xkb_state);
  if (self->xkb_keymap)
    self->xkb_state = xkb_state_new(self->xkb_keymap);
}

void Seat::keyboard_enter_handler(void *data, struct wl_keyboard *,
                                  uint32_t serial, struct wl_surface *,
                                  struct wl_array *) {
  // focused
}

void Seat::keyboard_leave_handler(void *data, struct wl_keyboard *,
                                  uint32_t serial, struct wl_surface *) {
  auto self = static_cast<Seat *>(data);
  self->update_repeat_timer(0, 0, false);
}

void Seat::keyboard_key_handler(void *data, struct wl_keyboard *,
                                uint32_t serial, uint32_t time, uint32_t key,
                                uint32_t state) {
  auto self = static_cast<Seat *>(data);
  uint32_t code = key + 8;
  xkb_keysym_t sym = XKB_KEY_NoSymbol;
  if (self->xkb_state) {
    sym = xkb_state_key_get_one_sym(self->xkb_state, code);
  }

  bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
  if (pressed) {
    self->keyboard_logic.handle_key(code, sym, self->xkb_state);
  }
  self->update_repeat_timer(code, sym, pressed);
}

void Seat::keyboard_modifiers_handler(void *data, struct wl_keyboard *,
                                      uint32_t, uint32_t dep, uint32_t lat,
                                      uint32_t loc, uint32_t grp) {
  auto self = static_cast<Seat *>(data);
  if (self->xkb_state) {
    xkb_state_update_mask(self->xkb_state, dep, lat, loc, 0, 0, grp);
  }
}

void Seat::keyboard_repeat_info_handler(void *data, struct wl_keyboard *,
                                        int32_t rate, int32_t delay) {
  auto self = static_cast<Seat *>(data);
  self->repeat_rate = rate;
  self->repeat_delay = delay;
}

void Seat::pointer_enter_handler(void *, struct wl_pointer *, uint32_t,
                                 struct wl_surface *, wl_fixed_t, wl_fixed_t) {}
void Seat::pointer_leave_handler(void *, struct wl_pointer *, uint32_t,
                                 struct wl_surface *) {}
void Seat::pointer_motion_handler(void *, struct wl_pointer *, uint32_t,
                                  wl_fixed_t, wl_fixed_t) {}
void Seat::pointer_button_handler(void *, struct wl_pointer *, uint32_t,
                                  uint32_t, uint32_t, uint32_t) {}

void Seat::pointer_axis_handler(void *data, struct wl_pointer *, uint32_t,
                                uint32_t axis, wl_fixed_t value) {
  auto self = static_cast<Seat *>(data);
  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
    double d = wl_fixed_to_double(value);
    self->pointer_logic.handle_axis(d);
  }
}

} // namespace Lawnch::Core::Window::Wayland
