#include "../config.hpp"
#include "../search/search.hpp"
#include "../utils.hpp"
#include "icon.hpp"
#include "window.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <librsvg/rsvg.h>
#include <map>
#include <sstream>
#include <sys/mman.h>
#include <unistd.h>

void LauncherWindow::create_buffer(Buffer &buffer, int width, int height) {
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
  int size = stride * height;
  if (size <= 0)
    return;

  int fd = memfd_create("lawnch-buffer", MFD_CLOEXEC);
  if (fd < 0)
    return;
  if (ftruncate(fd, size) < 0) {
    close(fd);
    return;
  }

  void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
  buffer.buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
                                            WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);

  buffer.surface = cairo_image_surface_create_for_data(
      (unsigned char *)data, CAIRO_FORMAT_ARGB32, width, height, stride);
  buffer.cairo = cairo_create(buffer.surface);
  buffer.width = width;
  buffer.height = height;
}

void LauncherWindow::destroy_buffer(Buffer &b) {
  if (b.buffer)
    wl_buffer_destroy(b.buffer);
  if (b.cairo)
    cairo_destroy(b.cairo);
  if (b.surface) {
    void *data = cairo_image_surface_get_data(b.surface);
    int stride = cairo_image_surface_get_stride(b.surface);
    munmap(data, stride * b.height);
    cairo_surface_destroy(b.surface);
  }
  b = {};
}

void LauncherWindow::update_results() {
  current_results = search.query(search_text);
  selected_index = 0;
  scroll_offset = 0;
  render();
}

void LauncherWindow::render() {
  if (!running)
    return;

  if (!current_buffer) {
    create_buffer(buffers[0], ConfigManager::get().window_width,
                  ConfigManager::get().window_height);
    create_buffer(buffers[1], ConfigManager::get().window_width,
                  ConfigManager::get().window_height);
    current_buffer = &buffers[0];
  }

  const auto &cfg = ConfigManager::get();
  cairo_t *cr = current_buffer->cairo;

  // Clear background
  cairo_save(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint(cr);
  cairo_restore(cr);

  // Draw Main Window
  Utils::draw_rounded_rect(cr, 0, 0, current_buffer->width,
                           current_buffer->height, cfg.window_border_radius);
  cairo_set_source_rgba(
      cr, cfg.window_background_color.r, cfg.window_background_color.g,
      cfg.window_background_color.b, cfg.window_background_color.a);
  cairo_fill_preserve(cr);
  cairo_set_source_rgba(cr, cfg.window_border_color.r,
                        cfg.window_border_color.g, cfg.window_border_color.b,
                        cfg.window_border_color.a);
  cairo_set_line_width(cr, cfg.window_border_width);
  cairo_stroke(cr);

  // Input Rendering
  PangoLayout *layout = pango_cairo_create_layout(cr);

  std::string font_desc_str = cfg.input_font_family;
  if (!cfg.input_font_weight.empty()) {
    font_desc_str += " " + cfg.input_font_weight;
  }

  PangoFontDescription *font =
      pango_font_description_from_string(font_desc_str.c_str());
  pango_font_description_set_size(font, cfg.input_font_size * PANGO_SCALE);
  pango_layout_set_font_description(layout, font);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

  double input_x = cfg.input_padding_left + cfg.window_border_width;
  double input_y = cfg.input_padding_top + cfg.window_border_width;
  double input_w = current_buffer->width -
                   (cfg.input_padding_left + cfg.input_padding_right +
                    cfg.window_border_width * 2);

  pango_layout_set_width(layout, input_w * PANGO_SCALE);
  if (cfg.input_horizontal_align == "center") {
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
  } else if (cfg.input_horizontal_align == "right") {
    pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
  } else {
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
  }

  if (search_text.empty()) {
    pango_layout_set_text(layout, "Search or type :h or for more...", -1);
    cairo_set_source_rgba(
        cr, cfg.input_placeholder_color.r, cfg.input_placeholder_color.g,
        cfg.input_placeholder_color.b, cfg.input_placeholder_color.a);
  } else {
    pango_layout_set_text(layout, search_text.c_str(), -1);

    // Highlight background if text is selected (not fully implemented
    // in snippet)
    if (input_selected) {
      //
    }

    cairo_set_source_rgba(cr, cfg.input_text_color.r, cfg.input_text_color.g,
                          cfg.input_text_color.b, cfg.input_text_color.a);
  }

  cairo_move_to(cr, input_x, input_y);
  pango_cairo_show_layout(cr, layout);

  // Caret Rendering
  if (!input_selected) {
    PangoRectangle strong_pos;
    PangoRectangle weak_pos;
    pango_layout_get_cursor_pos(layout, caret_position, &strong_pos, &weak_pos);

    // pango_layout_get_cursor_pos gives coordinates relative to the layout
    // origin (input_x, input_y) It automatically accounts for alignment shifts.
    double caret_x = input_x + (double)strong_pos.x / PANGO_SCALE;
    double caret_y = input_y + (double)strong_pos.y / PANGO_SCALE;
    double caret_h = (double)strong_pos.height / PANGO_SCALE;

    // Fallback height if empty
    if (caret_h == 0) {
      PangoFontMetrics *metrics = pango_context_get_metrics(
          pango_layout_get_context(layout), font, nullptr);
      caret_h = (pango_font_metrics_get_ascent(metrics) +
                 pango_font_metrics_get_descent(metrics)) /
                PANGO_SCALE;
      pango_font_metrics_unref(metrics);
    }

    cairo_set_source_rgba(cr, cfg.input_caret_color.r, cfg.input_caret_color.g,
                          cfg.input_caret_color.b, cfg.input_caret_color.a);
    cairo_move_to(cr, caret_x, caret_y);
    cairo_line_to(cr, caret_x, caret_y + caret_h);
    cairo_stroke(cr);
  }

  // Results Rendering

  // Reset font description to result font (ignore input weight if needed, or
  // re-parse)
  pango_font_description_free(font);
  font = pango_font_description_from_string(cfg.results_font_family.c_str());
  pango_font_description_set_size(font, cfg.results_font_size * PANGO_SCALE);

  PangoFontDescription *comment_font = pango_font_description_copy(font);
  pango_font_description_set_size(comment_font,
                                  cfg.results_comment_font_size * PANGO_SCALE);

  // Calculate Layout Metrics
  double start_y = input_y + 30 + cfg.results_item_spacing;
  double text_gap = 4.0;
  double name_h_approx = cfg.results_font_size * 1.4;
  double comment_h_approx = cfg.results_comment_font_size * 1.4;

  // Dynamic Height Calculation
  double item_inner_h = name_h_approx;
  if (cfg.results_enable_comment) {
    item_inner_h += text_gap + comment_h_approx;
  }

  double item_height =
      cfg.results_item_padding * 2 + item_inner_h + cfg.results_item_spacing;
  double available_h = current_buffer->height - start_y;

  if (item_height <= 0)
    item_height = 1;

  // Scroll Logic
  int visible_count = std::max(1, (int)std::floor(available_h / item_height));
  if (selected_index < scroll_offset) {
    scroll_offset = selected_index;
  } else if (selected_index >= scroll_offset + visible_count) {
    scroll_offset = selected_index - visible_count + 1;
  }

  int end_index =
      std::min((int)current_results.size(), scroll_offset + visible_count);

  // Render Items
  for (int i = scroll_offset; i < end_index; ++i) {
    const auto &res = current_results[i];
    int rel_i = i - scroll_offset;
    double item_y = start_y + rel_i * item_height;

    // Use input padding for result horizontal margins
    double box_x = cfg.input_padding_left;
    double box_w = current_buffer->width - cfg.input_padding_left -
                   cfg.input_padding_right;
    double box_h = item_height - cfg.results_item_spacing;

    // Selection Background
    if (i == selected_index) {
      Utils::draw_rounded_rect(cr, box_x, item_y, box_w, box_h,
                               cfg.results_item_border_radius);

      cairo_set_source_rgba(cr, cfg.results_selected_background_color.r,
                            cfg.results_selected_background_color.g,
                            cfg.results_selected_background_color.b,
                            cfg.results_selected_background_color.a);
      cairo_fill(cr);
      cairo_set_source_rgba(cr, cfg.results_selected_text_color.r,
                            cfg.results_selected_text_color.g,
                            cfg.results_selected_text_color.b,
                            cfg.results_selected_text_color.a);
    } else {
      cairo_set_source_rgba(cr, cfg.results_default_text_color.r,
                            cfg.results_default_text_color.g,
                            cfg.results_default_text_color.b,
                            cfg.results_default_text_color.a);
    }

    double content_start_x = box_x + cfg.results_item_padding;
    double current_x = content_start_x;
    double center_y = item_y + box_h / 2.0;

    // Render Icon
    if (cfg.results_enable_icon) {
      double icon_draw_y = center_y - (cfg.results_icon_size / 2.0);

      IconManager::get().render_icon(cr, res.icon, current_x, icon_draw_y,
                                     cfg.results_icon_size);

      current_x +=
          cfg.results_icon_size + 12; // 12 is gap between icon and text
    }

    // Render Text
    // Calculate available width strictly to avoid overflow when icon is present
    double available_text_w =
        (box_x + box_w - cfg.results_item_padding) - current_x;
    if (available_text_w < 0)
      available_text_w = 0;

    // Name
    double name_y;
    if (cfg.results_enable_comment) {
      // If comment enabled, name is top-aligned
      name_y = item_y + cfg.results_item_padding;
    } else {
      // If no comment, center the name vertically
      name_y = center_y - (name_h_approx / 2.0);
    }

    pango_layout_set_font_description(layout, font);
    pango_layout_set_text(layout, res.name.c_str(), -1);
    pango_layout_set_width(layout, available_text_w * PANGO_SCALE);
    pango_layout_set_alignment(
        layout, PANGO_ALIGN_LEFT); // Always left align result text for now

    cairo_move_to(cr, current_x, name_y);
    pango_cairo_show_layout(cr, layout);

    // Render comment
    // comment positioning
    int name_w, name_h_pixels;
    pango_layout_get_pixel_size(layout, &name_w, &name_h_pixels);

    if (cfg.results_enable_comment && !res.comment.empty()) {
      pango_layout_set_font_description(layout, comment_font);
      pango_layout_set_text(layout, res.comment.c_str(), -1);
      pango_layout_set_width(layout, available_text_w * PANGO_SCALE);

      cairo_set_source_rgba(cr, cfg.results_selected_comment_color.r,
                            cfg.results_selected_comment_color.g,
                            cfg.results_selected_comment_color.b,
                            cfg.results_selected_comment_color.a);

      if (i != selected_index) {
        cairo_set_source_rgba(
            cr, cfg.results_comment_color.r, cfg.results_comment_color.g,
            cfg.results_comment_color.b, cfg.results_comment_color.a);
      }

      cairo_move_to(cr, current_x, name_y + name_h_pixels + text_gap);
      pango_cairo_show_layout(cr, layout);
    }
  }

  g_object_unref(layout);
  pango_font_description_free(font);
  pango_font_description_free(comment_font);

  wl_surface_attach(surface, current_buffer->buffer, 0, 0);
  wl_surface_damage_buffer(surface, 0, 0, INT32_MAX, INT32_MAX);
  wl_surface_commit(surface);
}