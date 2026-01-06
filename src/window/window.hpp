#pragma once

#include <cairo.h>
#include <pango/pangocairo.h>
#include <string>
#include <vector>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

extern "C" {
#define namespace _namespace
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace
}

#include "../search/search.hpp"

struct Buffer {
  struct wl_buffer *buffer = nullptr;
  cairo_surface_t *surface = nullptr;
  cairo_t *cairo = nullptr;
  int width = 0, height = 0;
};

class LauncherWindow {
public:
  LauncherWindow(Search &search);
  ~LauncherWindow();

  void run();
  void stop();
  void push_undo();

  Search &search;

  // Wayland Globals
  struct wl_display *display = nullptr;
  struct wl_registry *registry = nullptr;
  struct wl_compositor *compositor = nullptr;
  struct wl_shm *shm = nullptr;
  struct wl_seat *seat = nullptr;
  struct zwlr_layer_shell_v1 *layer_shell = nullptr;
  struct wl_output *output = nullptr;

  // Wayland Objects
  struct wl_surface *surface = nullptr;
  struct zwlr_layer_surface_v1 *layer_surface = nullptr;
  struct wl_keyboard *keyboard = nullptr;
  struct wl_pointer *pointer = nullptr;

  // XKB
  struct xkb_context *xkb_context = nullptr;
  struct xkb_keymap *xkb_keymap = nullptr;
  struct xkb_state *xkb_state = nullptr;

  // Repeat Logic
  int32_t repeat_rate = 25;
  int32_t repeat_delay = 600;
  int repeat_timer_fd = -1;
  uint32_t last_key_code = 0;
  xkb_keysym_t last_key_sym = 0;

  // State
  Buffer buffers[2];
  Buffer *current_buffer = nullptr;
  bool running = true;
  uint32_t serial = 0;
  std::string search_text;
  int caret_position = 0;
  std::vector<SearchResult> current_results;
  int selected_index = 0;
  int scroll_offset = 0;
  bool input_selected = false;

  void render();
  void create_buffer(Buffer &buffer, int width, int height);
  void destroy_buffer(Buffer &buffer);
  void update_results();
  void handle_key_press_action(uint32_t keycode, xkb_keysym_t sym);
  void update_repeat_timer(uint32_t keycode, xkb_keysym_t sym, bool pressed);

  // Handlers
  static void registry_global_handler(void *data, struct wl_registry *registry,
                                      uint32_t name, const char *interface,
                                      uint32_t version);
  static void registry_global_remove_handler(void *data,
                                             struct wl_registry *registry,
                                             uint32_t name);
  static void shm_format_handler(void *data, struct wl_shm *wl_shm,
                                 uint32_t format);
  static void seat_capabilities_handler(void *data, struct wl_seat *seat,
                                        uint32_t capabilities);
  static void keyboard_keymap_handler(void *data, struct wl_keyboard *keyboard,
                                      uint32_t format, int32_t fd,
                                      uint32_t size);
  static void keyboard_enter_handler(void *data, struct wl_keyboard *keyboard,
                                     uint32_t serial,
                                     struct wl_surface *surface,
                                     struct wl_array *keys);
  static void keyboard_leave_handler(void *data, struct wl_keyboard *keyboard,
                                     uint32_t serial,
                                     struct wl_surface *surface);
  static void keyboard_key_handler(void *data, struct wl_keyboard *keyboard,
                                   uint32_t serial, uint32_t time, uint32_t key,
                                   uint32_t state);
  static void keyboard_modifiers_handler(void *data,
                                         struct wl_keyboard *keyboard,
                                         uint32_t serial,
                                         uint32_t mods_depressed,
                                         uint32_t mods_latched,
                                         uint32_t mods_locked, uint32_t group);
  static void keyboard_repeat_info_handler(void *data,
                                           struct wl_keyboard *keyboard,
                                           int32_t rate, int32_t delay);
  static void pointer_enter_handler(void *data, struct wl_pointer *pointer,
                                    uint32_t serial, struct wl_surface *surface,
                                    wl_fixed_t sx, wl_fixed_t sy);
  static void pointer_leave_handler(void *data, struct wl_pointer *pointer,
                                    uint32_t serial,
                                    struct wl_surface *surface);
  static void pointer_motion_handler(void *data, struct wl_pointer *pointer,
                                     uint32_t time, wl_fixed_t sx,
                                     wl_fixed_t sy);
  static void pointer_button_handler(void *data, struct wl_pointer *pointer,
                                     uint32_t serial, uint32_t time,
                                     uint32_t button, uint32_t state);
  static void pointer_axis_handler(void *data, struct wl_pointer *pointer,
                                   uint32_t time, uint32_t axis,
                                   wl_fixed_t value);
  static void layer_surface_configure_handler(
      void *data, struct zwlr_layer_surface_v1 *layer_surface, uint32_t serial,
      uint32_t width, uint32_t height);
  static void
  layer_surface_closed_handler(void *data,
                               struct zwlr_layer_surface_v1 *layer_surface);
};