#include "clock.hpp"
#include "../../../../helpers/gfx.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Lawnch::Core::Window::Render::Components {

ComponentResult Clock::draw(ComponentContext &context) {
  auto &ctx = context.ctx;
  auto &cfg = context.cfg;

  if (!cfg.clock_enable) {
    return {0, 0};
  }

  auto now = std::chrono::system_clock::now();
  std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
  std::tm *local_tm = std::localtime(&time_t_now);

  char time_buffer[128];
  std::strftime(time_buffer, sizeof(time_buffer), cfg.clock_format.c_str(),
                local_tm);
  std::string time_text(time_buffer);

  BLFont font = Lawnch::Gfx::get_font(
      cfg.clock_font_family, cfg.clock_font_size, cfg.clock_font_weight);
  BLFontMetrics fm = font.metrics();

  double clock_h =
      fm.ascent + fm.descent + cfg.clock_padding.top + cfg.clock_padding.bottom;

  double text_y = context.y + cfg.clock_padding.top + fm.ascent;
  double text_x = context.x + cfg.clock_padding.left;

  BLGlyphBuffer gb;
  gb.set_utf8_text(time_text.c_str(), time_text.size());
  font.shape(gb);
  BLTextMetrics tm;
  font.get_text_metrics(gb, tm);

  double avail_w =
      context.available_w - cfg.clock_padding.left - cfg.clock_padding.right;

  if (cfg.clock_text_align == "center") {
    text_x =
        context.x + cfg.clock_padding.left + (avail_w - tm.advance.x) / 2.0;
  } else if (cfg.clock_text_align == "right") {
    text_x = context.x + context.available_w - cfg.clock_padding.right -
             tm.advance.x;
  }

  ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.clock_text_color));
  ctx.fill_utf8_text(BLPoint(text_x, text_y), font, time_text.c_str());

  return {context.available_w, clock_h};
}

} // namespace Lawnch::Core::Window::Render::Components
