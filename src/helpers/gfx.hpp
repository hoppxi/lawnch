#pragma once
#include <cairo/cairo.h>

namespace Lawnch::Gfx {

void path_rounded_rect(cairo_t *cr, double x, double y, double width,
                       double height, double radius);

} // namespace Lawnch::Gfx
