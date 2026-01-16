#include "input_box.hpp"
#include "../../../../helpers/gfx.hpp"
#include "../render_state.hpp"
#include <algorithm>

namespace Lawnch::Core::Window::Render::Components {

ComponentResult InputBox::draw(ComponentContext &context) {
  auto &ctx = context.ctx;
  auto &cfg = context.cfg;
  auto &state = context.state;
  int width = context.window_width;

  BLFont font = Lawnch::Gfx::get_font(
      cfg.input_font_family, cfg.input_font_size, cfg.input_font_weight);

  BLFontMetrics metrics = font.metrics();
  double font_h = metrics.ascent + metrics.descent;

  double input_box_x = context.x;
  double input_box_y = context.y;
  double input_box_w = context.available_w;

  double input_box_h =
      cfg.input_padding.top + font_h + cfg.input_padding.bottom;

  BLRoundRect rect =
      Lawnch::Gfx::rounded_rect(input_box_x, input_box_y, input_box_w,
                                input_box_h, cfg.input_border_radius);

  auto bg = cfg.input_background_color;
  ctx.set_fill_style(Lawnch::Gfx::toBLColor(bg));
  ctx.fill_round_rect(rect);

  if (cfg.input_border_width > 0) {
    auto border = cfg.input_border_color;
    ctx.set_stroke_style(Lawnch::Gfx::toBLColor(border));
    ctx.set_stroke_width(cfg.input_border_width);
    ctx.stroke_round_rect(rect);
  }

  bool is_empty = state.search_text.empty();
  std::string display_text =
      is_empty ? cfg.input_placeholder_text : state.search_text;

  BLGlyphBuffer gb;
  gb.set_utf8_text(display_text.c_str());
  font.shape(gb);

  BLTextMetrics text_metrics;
  font.get_text_metrics(gb, text_metrics);
  double text_width = text_metrics.advance.x;

  double text_avail_w =
      input_box_w - (cfg.input_padding.left + cfg.input_padding.right);
  double x_offset = 0;

  if (cfg.input_horizontal_align == "center") {
    x_offset = (text_avail_w - text_width) / 2.0;
  } else if (cfg.input_horizontal_align == "right") {
    x_offset = text_avail_w - text_width;
  }

  double draw_x = input_box_x + cfg.input_padding.left + x_offset;
  double draw_y = input_box_y + cfg.input_padding.top + metrics.ascent;

  ctx.save();
  ctx.clip_to_rect(BLRect(input_box_x, input_box_y, input_box_w, input_box_h));

  if (state.input_selected && !is_empty) {
    auto sel_col = cfg.input_selection_color;
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(sel_col));
    ctx.fill_rect(BLRect(draw_x, input_box_y + cfg.input_padding.top,
                         text_width, font_h));

    auto txt_col = cfg.input_selection_text_color;
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(txt_col));
  } else {
    Lawnch::Config::Color c =
        is_empty ? cfg.input_placeholder_color : cfg.input_text_color;
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(c));
  }

  if (!display_text.empty()) {
    ctx.fill_utf8_text(BLPoint(draw_x, draw_y), font, display_text.c_str());
  }

  if (!state.input_selected) {
    double caret_x_offset = 0;

    if (!is_empty && state.caret_position > 0) {
      BLGlyphBuffer gb_caret;
      size_t safe_pos =
          std::min((size_t)state.caret_position, state.search_text.length());
      gb_caret.set_utf8_text(state.search_text.c_str(), safe_pos);
      font.shape(gb_caret);

      BLTextMetrics cm;
      font.get_text_metrics(gb_caret, cm);
      caret_x_offset = cm.advance.x;
    } else if (is_empty) {
      if (cfg.input_horizontal_align == "center") {
        draw_x = input_box_x + cfg.input_padding.left + (text_avail_w / 2.0);
      } else if (cfg.input_horizontal_align == "right") {
        draw_x = input_box_x + cfg.input_padding.left + text_avail_w;
      }
    }

    double caret_x = draw_x + caret_x_offset;
    double caret_y = input_box_y + cfg.input_padding.top;

    auto caret_col = cfg.input_caret_color;
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(caret_col));
    ctx.fill_rect(BLRect(caret_x, caret_y, cfg.input_caret_width, font_h));
  }

  ctx.restore();

  return {input_box_w, input_box_h};
}

} // namespace Lawnch::Core::Window::Render::Components
