#pragma once

#include "../helpers/config_parse.hpp"
#include <map>
#include <string>

namespace Lawnch::Config {

bool isColorFunction(const std::string &value);
std::string
processColorFunction(const std::string &value,
                     const std::map<std::string, Color> &theme_colors);

struct HSL {
  double h, s, l, a;
};

HSL rgbToHsl(double r, double g, double b, double a = 1.0);
Color hslToRgb(double h, double s, double l, double a = 1.0);
std::string colorToHex(const Color &c);

Color parseColorArg(const std::string &arg,
                    const std::map<std::string, Color> &theme_colors);

} // namespace Lawnch::Config
