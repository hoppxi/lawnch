#include "renderer.hpp"
#include "../../../helpers/gfx.hpp"
#include "../../../helpers/string.hpp"
#include "../../icons/manager.hpp"
#include "background.hpp"
#include "input_box.hpp"
#include "results_list.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Lawnch::Core::Window::Render {

static BLRgba32 ToBLColor(const Lawnch::Config::Color &c) {
  return BLRgba32(c.r * 255, c.g * 255, c.b * 255, c.a * 255);
}

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::render(BLContext &ctx, int width, int height,
                      const Config::Config &cfg, const RenderState &state) {
  if (!cached_metrics.valid)
    update_metrics(cfg);

  Background::draw(ctx, width, height, cfg);

  double input_h = 0;
  InputBox::draw(ctx, width, cfg, state.search_text, state.caret_position,
                 state.input_selected, input_h);

  double current_y = cfg.window_border_width + input_h;

  if (cfg.results_count_enable) {
    BLFont count_font = Lawnch::Gfx::get_font(cfg.results_count_font_family,
                                              cfg.results_count_font_size,
                                              cfg.results_count_font_weight);
    BLFontMetrics fm = count_font.metrics();
    double count_h = fm.ascent + fm.descent + cfg.results_count_padding.top +
                     cfg.results_count_padding.bottom;

    std::string count_text =
        Lawnch::Str::replace_all(cfg.results_count_format, "{count}",
                                 std::to_string(state.results.size()));

    double text_y = current_y + cfg.results_count_padding.top + fm.ascent;
    double text_x = cfg.window_border_width + cfg.results_count_padding.left;

    BLGlyphBuffer gb;
    gb.set_utf8_text(count_text.c_str(), count_text.size());
    count_font.shape(gb);
    BLTextMetrics tm;
    count_font.get_text_metrics(gb, tm);

    double avail_w = width - (cfg.window_border_width * 2) -
                     cfg.results_count_padding.left -
                     cfg.results_count_padding.right;

    if (cfg.results_count_text_align == "center") {
      text_x = cfg.window_border_width + cfg.results_count_padding.left +
               (avail_w - tm.advance.x) / 2.0;
    } else if (cfg.results_count_text_align == "right") {
      text_x = width - cfg.window_border_width -
               cfg.results_count_padding.right - tm.advance.x;
    }

    ctx.set_fill_style(ToBLColor(cfg.results_count_text_color));
    ctx.fill_utf8_text(BLPoint(text_x, text_y), count_font, count_text.c_str());

    current_y += count_h;
  }

  double potential_preview_h = 0;
  if (cfg.preview_enable && !state.results.empty()) {
    potential_preview_h = cfg.preview_icon_size + cfg.preview_padding.top +
                          cfg.preview_padding.bottom;
  }

  double preview_h = 0;

  if (potential_preview_h > 0) {
    double results_avail_h_full = height - cfg.window_border_width - current_y;
    double results_overhead =
        cfg.results_margin.top + cfg.results_margin.bottom +
        cfg.results_padding.top + cfg.results_padding.bottom;

    double max_items_h_full = results_avail_h_full - results_overhead;
    double required_items_h = 0;
    if (cached_metrics.item_height > 0) {
      required_items_h = state.results.size() * cached_metrics.item_height;
    }

    if (required_items_h <= max_items_h_full) {
      preview_h = potential_preview_h;
    }
  }

  double results_end_y = height - cfg.window_border_width - preview_h;
  ResultsList::draw(ctx, width, results_end_y, current_y, cfg, state.results,
                    state.scroll_offset, state.selected_index,
                    cached_metrics.item_height, state.search_text);

  // preview
  if (preview_h > 0 && state.selected_index >= 0 &&
      state.selected_index < (int)state.results.size()) {

    const auto &selected = state.results[state.selected_index];

    double preview_y = results_end_y + cfg.preview_padding.top;
    double preview_center_x = width / 2.0;
    double icon_x = preview_center_x - (cfg.preview_icon_size / 2.0);

    if (cfg.preview_background_color.a > 0) {
      double bg_x = cfg.window_border_width + cfg.preview_padding.left;
      double bg_w = width - (cfg.window_border_width * 2) -
                    cfg.preview_padding.left - cfg.preview_padding.right;
      ctx.set_fill_style(ToBLColor(cfg.preview_background_color));
      ctx.fill_rect(bg_x, results_end_y, bg_w, preview_h);
    }

    Icons::Manager::Instance().render_icon(ctx, selected.icon, icon_x,
                                           preview_y, cfg.preview_icon_size);
  }
}

int Renderer::get_visible_count(int height, const Config::Config &cfg) {
  if (!cached_metrics.valid)
    update_metrics(cfg);
  if (cached_metrics.item_height <= 0)
    return 1;

  double input_h_approx = (cfg.input_font_size * 1.5) +
                          (cfg.input_padding.top + cfg.input_padding.bottom);

  double count_h = 0;
  if (cfg.results_count_enable) {
    count_h = cfg.results_count_font_size * 1.5 +
              cfg.results_count_padding.top + cfg.results_count_padding.bottom;
  }

  double preview_h = 0;
  if (cfg.preview_enable) {
    preview_h = cfg.preview_icon_size + cfg.preview_padding.top +
                cfg.preview_padding.bottom;
  }

  double start_y = cfg.window_border_width + input_h_approx + count_h;
  double results_start_y = start_y + cfg.results_margin.top;
  double available_h = height - results_start_y - cfg.results_margin.bottom -
                       preview_h - cfg.results_padding.top -
                       cfg.results_padding.bottom;

  return std::max(1, (int)std::floor(available_h / cached_metrics.item_height));
}

void Renderer::update_metrics(const Config::Config &cfg) {
  BLFont font = Lawnch::Gfx::get_font(cfg.result_item_font_family,
                                      cfg.result_item_font_size,
                                      cfg.result_item_font_weight);
  BLFontMetrics metrics = font.metrics();
  double name_h = metrics.ascent + metrics.descent;

  double comment_h = 0;
  if (cfg.result_item_enable_comment) {
    BLFont cfont = Lawnch::Gfx::get_font(cfg.result_item_font_family,
                                         cfg.result_item_comment_font_size,
                                         cfg.result_item_comment_font_weight);
    BLFontMetrics cmetrics = cfont.metrics();
    comment_h = cmetrics.ascent + cmetrics.descent;
  }

  double text_gap = 4.0;
  double item_inner_h = name_h;
  if (cfg.result_item_enable_comment) {
    item_inner_h += text_gap + comment_h;
  }

  cached_metrics.item_height =
      (cfg.result_item_padding.top + cfg.result_item_padding.bottom) +
      item_inner_h + cfg.results_spacing;
  cached_metrics.valid = true;
}

} // namespace Lawnch::Core::Window::Render
