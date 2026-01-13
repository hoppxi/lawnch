#pragma once

#include "../../config/config.hpp"
#include <blend2d.h>

namespace Lawnch::Core::Window::Render {

class Background {
public:
  static void draw(BLContext &ctx, int width, int height,
                   const Config::Config &cfg);
};

} // namespace Lawnch::Core::Window::Render
