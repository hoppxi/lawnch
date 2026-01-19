#include "input_prompt.hpp"
#include "../../../../helpers/gfx.hpp"

namespace Lawnch::Core::Window::Render::Components {

double InputPrompt::calculate_width(const Config::Config &cfg) {
  if (!cfg.input_prompt_enable || cfg.input_prompt_content.empty()) {
    return 0;
  }

  BLFont font = Lawnch::Gfx::get_font(cfg.input_prompt_font_family,
                                      cfg.input_prompt_font_size,
                                      cfg.input_prompt_font_weight);

  BLGlyphBuffer gb;
  gb.set_utf8_text(cfg.input_prompt_content.c_str());
  font.shape(gb);

  BLTextMetrics tm;
  font.get_text_metrics(gb, tm);

  return tm.advance.x + cfg.input_prompt_padding.left +
         cfg.input_prompt_padding.right;
}

ComponentResult InputPrompt::draw(ComponentContext &context) {
  auto &ctx = context.ctx;
  auto &cfg = context.cfg;

  if (!cfg.input_prompt_enable || cfg.input_prompt_content.empty()) {
    return {0, 0};
  }

  BLRoundRect rect = Lawnch::Gfx::rounded_rect(
      context.x, context.y, context.available_w, context.available_h,
      cfg.input_prompt_border_radius);
  if (cfg.input_prompt_background_color.a > 0) {
    ctx.set_fill_style(
        Lawnch::Gfx::toBLColor(cfg.input_prompt_background_color));
    ctx.fill_round_rect(rect);
  }
  if (cfg.input_prompt_border_width > 0) {
    ctx.set_stroke_style(Lawnch::Gfx::toBLColor(cfg.input_prompt_border_color));
    ctx.set_stroke_width(cfg.input_prompt_border_width);
    ctx.stroke_round_rect(rect);
  }

  BLFont font = Lawnch::Gfx::get_font(cfg.input_prompt_font_family,
                                      cfg.input_prompt_font_size,
                                      cfg.input_prompt_font_weight);

  BLFontMetrics fm = font.metrics();

  double content_h = context.available_h - cfg.input_prompt_padding.top -
                     cfg.input_prompt_padding.bottom;
  double content_y = context.y + cfg.input_prompt_padding.top;

  double text_x = context.x + cfg.input_prompt_padding.left;
  double text_y =
      content_y + (content_h - (fm.ascent + fm.descent)) / 2.0 + fm.ascent;

  ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.input_prompt_text_color));
  ctx.fill_utf8_text(BLPoint(text_x, text_y), font,
                     cfg.input_prompt_content.c_str());

  return {context.available_w, context.available_h};
}

} // namespace Lawnch::Core::Window::Render::Components
