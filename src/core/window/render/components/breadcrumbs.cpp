#include "breadcrumbs.hpp"
#include "../../../../helpers/gfx.hpp"
#include "../render_state.hpp"

namespace Lawnch::Core::Window::Render::Components {

ComponentResult Breadcrumbs::draw(ComponentContext &context) {
  auto &ctx = context.ctx;
  auto &cfg = context.cfg;
  auto &state = context.state;

  if (!cfg.breadcrumbs_enable || state.breadcrumb_trail.empty()) {
    return {0, 0};
  }

  BLFont font = Lawnch::Gfx::get_font(
      cfg.breadcrumbs_font_family, cfg.breadcrumbs_font_size,
      cfg.breadcrumbs_font_weight, cfg.breadcrumbs_font_path);
  BLFontMetrics fm = font.metrics();

  double crumb_h = fm.ascent + fm.descent + cfg.breadcrumbs_padding.top +
                   cfg.breadcrumbs_padding.bottom;

  std::string sep = " " + cfg.breadcrumbs_separator + " ";
  std::string full_text;
  for (size_t i = 0; i < state.breadcrumb_trail.size(); ++i) {
    if (i > 0)
      full_text += sep;
    full_text += state.breadcrumb_trail[i];
  }

  BLGlyphBuffer gb;
  gb.set_utf8_text(full_text.c_str(), full_text.size());
  font.shape(gb);
  BLTextMetrics tm;
  font.get_text_metrics(gb, tm);

  double avail_w = context.available_w - cfg.breadcrumbs_padding.left -
                   cfg.breadcrumbs_padding.right;
  double text_width = tm.advance.x;

  double text_x = context.x + cfg.breadcrumbs_padding.left;
  double text_y = context.y + cfg.breadcrumbs_padding.top + fm.ascent;

  if (text_width <= avail_w) {
    if (cfg.breadcrumbs_align == "center") {
      text_x += (avail_w - text_width) / 2.0;
    } else if (cfg.breadcrumbs_align == "right") {
      text_x += avail_w - text_width;
    }
  } else {
    double scroll_offset = text_width - avail_w;
    text_x -= scroll_offset;
  }

  ctx.save();
  ctx.clip_to_rect(BLRect(context.x + cfg.breadcrumbs_padding.left, context.y,
                          avail_w, crumb_h));

  ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.breadcrumbs_color));
  ctx.fill_utf8_text(BLPoint(text_x, text_y), font, full_text.c_str());

  ctx.restore();

  return {context.available_w, crumb_h};
}

} // namespace Lawnch::Core::Window::Render::Components
