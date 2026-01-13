#include "background.hpp"
#include "../../../helpers/gfx.hpp"

namespace Lawnch::Core::Window::Render {

void Background::draw(BLContext &ctx, int width, int height,
                      const Config::Config &cfg) {
  // clear
  ctx.set_comp_op(BL_COMP_OP_SRC_COPY);
  ctx.fill_all(BLRgba32(0, 0, 0, 0));
  ctx.set_comp_op(BL_COMP_OP_SRC_OVER);

  BLRoundRect rect =
      Lawnch::Gfx::rounded_rect(0, 0, width, height, cfg.window_border_radius);

  auto bg = cfg.window_background_color;
  ctx.set_fill_style(BLRgba32(bg.r * 255, bg.g * 255, bg.b * 255, bg.a * 255));
  ctx.fill_round_rect(rect);

  auto border = cfg.window_border_color;
  ctx.set_stroke_style(
      BLRgba32(border.r * 255, border.g * 255, border.b * 255, border.a * 255));
  ctx.set_stroke_width(cfg.window_border_width);
  ctx.stroke_round_rect(rect);
}

} // namespace Lawnch::Core::Window::Render
