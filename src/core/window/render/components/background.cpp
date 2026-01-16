#include "background.hpp"
#include "../../../../helpers/gfx.hpp"

namespace Lawnch::Core::Window::Render::Components {

ComponentResult Background::draw(ComponentContext &context) {
  auto &ctx = context.ctx;
  auto &cfg = context.cfg;
  int width = context.window_width;
  int height = context.window_height;

  // Clear with transparency
  ctx.set_comp_op(BL_COMP_OP_SRC_COPY);
  ctx.fill_all(BLRgba32(0, 0, 0, 0));
  ctx.set_comp_op(BL_COMP_OP_SRC_OVER);

  BLRoundRect rect =
      Lawnch::Gfx::rounded_rect(0, 0, width, height, cfg.window_border_radius);

  auto bg = cfg.window_background_color;
  ctx.set_fill_style(Lawnch::Gfx::toBLColor(bg));
  ctx.fill_round_rect(rect);

  auto border = cfg.window_border_color;
  ctx.set_stroke_style(Lawnch::Gfx::toBLColor(border));
  ctx.set_stroke_width(cfg.window_border_width);
  ctx.stroke_round_rect(rect);

  return {static_cast<double>(width), static_cast<double>(height)};
}

} // namespace Lawnch::Core::Window::Render::Components
