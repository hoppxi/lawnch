#include "input_box.hpp"
#include "../../../helpers/gfx.hpp"
#include "../../config/config.hpp"
#include <chrono>

namespace Lawnch::Core::Window::Render {

void InputBox::draw(BLContext &ctx, int width, const Config::Config &cfg,
                    const std::string &text, int caret_pos, bool selected,
                    double &out_height) {

  BLFont font = Lawnch::Gfx::get_font(
      cfg.input_font_family, cfg.input_font_size, cfg.input_font_weight);

  BLFontMetrics metrics = font.metrics();
  double font_h = metrics.ascent + metrics.descent;

  double input_box_x = cfg.window_border_width;
  double input_box_y = cfg.window_border_width;
  double input_box_w = width - (cfg.window_border_width * 2);

  double input_box_h =
      cfg.input_padding.top + font_h + cfg.input_padding.bottom;
  out_height = input_box_h;

  BLRoundRect rect =
      Lawnch::Gfx::rounded_rect(input_box_x, input_box_y, input_box_w,
                                input_box_h, cfg.input_border_radius);

  auto bg = cfg.input_background_color;
  ctx.set_fill_style(BLRgba32(bg.r * 255, bg.g * 255, bg.b * 255, bg.a * 255));
  ctx.fill_round_rect(rect);

  if (cfg.input_border_width > 0) {
    auto border = cfg.input_border_color;
    ctx.set_stroke_style(BLRgba32(border.r * 255, border.g * 255,
                                  border.b * 255, border.a * 255));
    ctx.set_stroke_width(cfg.input_border_width);
    ctx.stroke_round_rect(rect);
  }

  bool is_empty = text.empty();
  std::string display_text = is_empty ? cfg.input_placeholder_text : text;

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

  if (selected && !is_empty) {
    auto sel_col = cfg.input_selection_color;
    ctx.set_fill_style(BLRgba32(sel_col.r * 255, sel_col.g * 255,
                                sel_col.b * 255, sel_col.a * 255));
    ctx.fill_rect(BLRect(draw_x, input_box_y + cfg.input_padding.top,
                         text_width, font_h));

    auto txt_col = cfg.input_selection_text_color;
    ctx.set_fill_style(BLRgba32(txt_col.r * 255, txt_col.g * 255,
                                txt_col.b * 255, txt_col.a * 255));
  } else {
    Lawnch::Config::Color c =
        is_empty ? cfg.input_placeholder_color : cfg.input_text_color;
    ctx.set_fill_style(BLRgba32(c.r * 255, c.g * 255, c.b * 255, c.a * 255));
  }

  if (!display_text.empty()) {
    ctx.fill_utf8_text(BLPoint(draw_x, draw_y), font, display_text.c_str());
  }

  if (!selected) {
    double caret_x_offset = 0;

    if (!is_empty && caret_pos > 0) {
      BLGlyphBuffer gb_caret;
      size_t safe_pos = std::min((size_t)caret_pos, text.length());
      gb_caret.set_utf8_text(text.c_str(), safe_pos);
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
    ctx.set_fill_style(BLRgba32(caret_col.r * 255, caret_col.g * 255,
                                caret_col.b * 255, caret_col.a * 255));

    ctx.fill_rect(BLRect(caret_x, caret_y, cfg.input_caret_width, font_h));
  }

  ctx.restore();
}

} // namespace Lawnch::Core::Window::Render
