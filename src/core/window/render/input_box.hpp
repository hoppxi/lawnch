#pragma once

#include "../../config/config.hpp"
#include <blend2d.h>
#include <string>

namespace Lawnch::Core::Window::Render {

class InputBox {
public:
  static void draw(BLContext &ctx, int width, const Config::Config &cfg,
                   const std::string &text, int caret_pos, bool selected,
                   double &out_height);
};

} // namespace Lawnch::Core::Window::Render
