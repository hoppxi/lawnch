#define _USE_MATH_DEFINES
#include "gfx.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lawnch::Gfx {

void path_rounded_rect(cairo_t *cr, double x, double y, double width,
                       double height, double radius) {

  if (radius > width / 2.0)
    radius = width / 2.0;
  if (radius > height / 2.0)
    radius = height / 2.0;

  cairo_new_sub_path(cr);
  cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI / 2, 0);
  cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI / 2);
  cairo_arc(cr, x + radius, y + height - radius, radius, M_PI / 2, M_PI);
  cairo_arc(cr, x + radius, y + radius, radius, M_PI, M_PI * 3 / 2);
  cairo_close_path(cr);
}

} // namespace Lawnch::Gfx
