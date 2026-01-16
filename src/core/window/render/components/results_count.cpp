#include "results_count.hpp"
#include "../../../../helpers/gfx.hpp"
#include "../../../../helpers/string.hpp"
#include "../render_state.hpp"

namespace Lawnch::Core::Window::Render::Components {

ComponentResult ResultsCount::draw(ComponentContext &context) {
  auto &ctx = context.ctx;
  auto &cfg = context.cfg;
  auto &state = context.state;

  if (!cfg.results_count_enable) {
    return {0, 0};
  }

  BLFont count_font = Lawnch::Gfx::get_font(cfg.results_count_font_family,
                                            cfg.results_count_font_size,
                                            cfg.results_count_font_weight);
  BLFontMetrics fm = count_font.metrics();
  double count_h = fm.ascent + fm.descent + cfg.results_count_padding.top +
                   cfg.results_count_padding.bottom;

  std::string count_text =
      Lawnch::Str::replace_all(cfg.results_count_format, "{count}",
                               std::to_string(state.results.size()));

  double text_y = context.y + cfg.results_count_padding.top + fm.ascent;
  double text_x = context.x + cfg.results_count_padding.left;

  BLGlyphBuffer gb;
  gb.set_utf8_text(count_text.c_str(), count_text.size());
  count_font.shape(gb);
  BLTextMetrics tm;
  count_font.get_text_metrics(gb, tm);

  double avail_w = context.available_w - cfg.results_count_padding.left -
                   cfg.results_count_padding.right;

  if (cfg.results_count_text_align == "center") {
    text_x = context.x + cfg.results_count_padding.left +
             (avail_w - tm.advance.x) / 2.0;
  } else if (cfg.results_count_text_align == "right") {
    text_x = context.x + context.available_w - cfg.results_count_padding.right -
             tm.advance.x;
  }

  ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.results_count_text_color));
  ctx.fill_utf8_text(BLPoint(text_x, text_y), count_font, count_text.c_str());

  return {context.available_w, count_h};
}

} // namespace Lawnch::Core::Window::Render::Components
