#pragma once
#include "config_parse.hpp"
#include <blend2d.h>
#include <string>

namespace Lawnch::Gfx {
BLRgba32 toBLColor(const Config::Color &c);

BLRoundRect rounded_rect(double x, double y, double width, double height,
                         double radius);

BLFont get_font(const std::string &family, double size,
                const std::string &weight = "normal",
                const std::string &font_path_override = "");

std::string truncate_text(const std::string &text, BLFont &font,
                          double max_width);

} // namespace Lawnch::Gfx
