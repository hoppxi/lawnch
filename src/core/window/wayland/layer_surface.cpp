#include "layer_surface.hpp"
#include "../../../helpers/string.hpp"
#include <iostream>

namespace Lawnch::Core::Window::Wayland {

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = LayerSurface::configure_handler,
    .closed = LayerSurface::closed_handler,
};

LayerSurface::LayerSurface(struct wl_compositor *c,
                           struct zwlr_layer_shell_v1 *shell,
                           struct wl_output *o) {
  surface = wl_compositor_create_surface(c);
  layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      shell, surface, o, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "lawnch");
  zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener,
                                     this);
}

LayerSurface::~LayerSurface() { close(); }

void LayerSurface::close() {
  if (layer_surface) {
    zwlr_layer_surface_v1_destroy(layer_surface);
    layer_surface = nullptr;
  }
  if (surface) {
    wl_surface_destroy(surface);
    surface = nullptr;
  }
}

void LayerSurface::apply_config(const Config::Config &cfg) {
  if (!layer_surface)
    return;

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
    m_top = cfg.window_margin.top;
  }
  if (has_bottom) {
    anchor_opts |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    m_bottom = cfg.window_margin.bottom;
  }
  if (has_left) {
    anchor_opts |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
    m_left = cfg.window_margin.left;
  }
  if (has_right) {
    anchor_opts |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    m_right = cfg.window_margin.right;
  }

  zwlr_layer_surface_v1_set_anchor(layer_surface, anchor_opts);

  int exclusive_zone = -1;
  if (cfg.window_exclusive_zone) {
    if (has_top || has_bottom) {
      exclusive_zone = cfg.window_height;
    } else if (has_left || has_right) {
      exclusive_zone = cfg.window_width;
    } else {
      exclusive_zone = cfg.window_height;
    }
  }
  zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, exclusive_zone);

  zwlr_layer_surface_v1_set_margin(layer_surface, m_top, m_right, m_bottom,
                                   m_left);

  uint32_t kb_interactivity =
      ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE;
  if (Lawnch::Str::iequals(cfg.window_keyboard_interactivity, "none")) {
    kb_interactivity = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE;
  } else if (Lawnch::Str::iequals(cfg.window_keyboard_interactivity,
                                  "on_demand")) {
    kb_interactivity = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND;
  }
  zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface,
                                                   kb_interactivity);

  wl_surface_commit(surface);
}

void LayerSurface::commit(struct wl_buffer *buffer) {
  if (surface && buffer) {
    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(surface);
  }
}

void LayerSurface::configure_handler(void *data,
                                     struct zwlr_layer_surface_v1 *ls,
                                     uint32_t serial, uint32_t w, uint32_t h) {
  auto self = static_cast<LayerSurface *>(data);
  self->width = w;
  self->height = h;
  zwlr_layer_surface_v1_ack_configure(ls, serial);

  if (self->on_configure)
    self->on_configure(w, h);
}

void LayerSurface::closed_handler(void *data, struct zwlr_layer_surface_v1 *) {
  auto self = static_cast<LayerSurface *>(data);
  if (self->on_closed)
    self->on_closed();
}

} // namespace Lawnch::Core::Window::Wayland
