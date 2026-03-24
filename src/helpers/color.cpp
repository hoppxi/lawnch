#include "color.hpp"
#include "config_parse.hpp"
#include "logger.hpp"
#include "string.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <regex>
#include <string>

namespace Lawnch::Config {

HSL rgbToHsl(double r, double g, double b, double a) {
  double mx = std::max({r, g, b});
  double mn = std::min({r, g, b});
  double h = 0, s = 0, l = (mx + mn) / 2.0;

  if (mx != mn) {
    double d = mx - mn;
    s = l > 0.5 ? d / (2.0 - mx - mn) : d / (mx + mn);

    if (mx == r) {
      h = (g - b) / d + (g < b ? 6.0 : 0.0);
    } else if (mx == g) {
      h = (b - r) / d + 2.0;
    } else {
      h = (r - g) / d + 4.0;
    }
    h /= 6.0;
  }

  return {h * 360.0, s * 100.0, l * 100.0, a};
}

static double hueToRgb(double p, double q, double t) {
  if (t < 0.0)
    t += 1.0;
  if (t > 1.0)
    t -= 1.0;
  if (t < 1.0 / 6.0)
    return p + (q - p) * 6.0 * t;
  if (t < 1.0 / 2.0)
    return q;
  if (t < 2.0 / 3.0)
    return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
  return p;
}

Color hslToRgb(double h, double s, double l, double a) {
  h = std::fmod(h, 360.0);
  if (h < 0)
    h += 360.0;
  double hh = h / 360.0;
  double ss = std::clamp(s / 100.0, 0.0, 1.0);
  double ll = std::clamp(l / 100.0, 0.0, 1.0);

  Color c;
  c.a = a;

  if (ss == 0.0) {
    c.r = c.g = c.b = ll;
  } else {
    double q = ll < 0.5 ? ll * (1.0 + ss) : ll + ss - ll * ss;
    double p = 2.0 * ll - q;
    c.r = hueToRgb(p, q, hh + 1.0 / 3.0);
    c.g = hueToRgb(p, q, hh);
    c.b = hueToRgb(p, q, hh - 1.0 / 3.0);
  }

  return c;
}

std::string colorToHex(const Color &c) {
  auto to8 = [](double v) -> int {
    return static_cast<int>(std::clamp(v, 0.0, 1.0) * 255.0 + 0.5);
  };
  char buf[10];
  std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", to8(c.r), to8(c.g),
                to8(c.b), to8(c.a));
  return std::string(buf);
}

Color parseColorArg(const std::string &arg,
                    const std::map<std::string, Color> &theme_colors) {
  std::string trimmed = Lawnch::Str::trim(arg);

  if (!trimmed.empty() && trimmed[0] == '$') {
    std::string var_name = trimmed.substr(1);
    auto it = theme_colors.find(var_name);
    if (it != theme_colors.end()) {
      return it->second;
    }
    Logger::log("Color", Logger::LogLevel::WARNING,
                "Unknown theme variable: " + trimmed);
    return {0, 0, 0, 1};
  }

  if (!trimmed.empty() && trimmed[0] == '#') {
    return parseHexColor(trimmed);
  }

  if (trimmed.rfind("rgba(", 0) == 0) {
    return parseRgbaColor(trimmed);
  }

  if (trimmed.rfind("rgb(", 0) == 0) {
    return parseRgbColor(trimmed);
  }

  Logger::log("Color", Logger::LogLevel::WARNING,
              "Unrecognized color argument: " + trimmed);
  return {0, 0, 0, 1};
}

static const std::regex COLOR_FUNC_REGEX(
    R"(^\s*(lighten|darken|opacity|saturate|desaturate|mix|invert|complement|grayscale|adjust-hue)\s*\()",
    std::regex::icase);

bool isColorFunction(const std::string &value) {
  return std::regex_search(value, COLOR_FUNC_REGEX);
}

static std::vector<std::string> splitFuncArgs(const std::string &inner) {
  std::vector<std::string> args;
  int depth = 0;
  std::string current;

  for (char ch : inner) {
    if (ch == '(') {
      depth++;
      current += ch;
    } else if (ch == ')') {
      depth--;
      current += ch;
    } else if (ch == ',' && depth == 0) {
      args.push_back(Lawnch::Str::trim(current));
      current.clear();
    } else {
      current += ch;
    }
  }
  if (!current.empty()) {
    args.push_back(Lawnch::Str::trim(current));
  }
  return args;
}

static double parsePercent(const std::string &s) {
  std::string trimmed = Lawnch::Str::trim(s);
  if (!trimmed.empty() && trimmed.back() == '%') {
    trimmed.pop_back();
    try {
      return std::stod(trimmed);
    } catch (...) {
      return 0.0;
    }
  }
  try {
    return std::stod(trimmed);
  } catch (...) {
    return 0.0;
  }
}

static Color lightenColor(const Color &c, double amount) {
  HSL hsl = rgbToHsl(c.r, c.g, c.b, c.a);
  hsl.l = std::clamp(hsl.l + amount, 0.0, 100.0);
  return hslToRgb(hsl.h, hsl.s, hsl.l, hsl.a);
}

static Color darkenColor(const Color &c, double amount) {
  HSL hsl = rgbToHsl(c.r, c.g, c.b, c.a);
  hsl.l = std::clamp(hsl.l - amount, 0.0, 100.0);
  return hslToRgb(hsl.h, hsl.s, hsl.l, hsl.a);
}

static Color opacityColor(const Color &c, double amount) {
  Color result = c;
  result.a = std::clamp(amount / 100.0, 0.0, 1.0);
  return result;
}

static Color saturateColor(const Color &c, double amount) {
  HSL hsl = rgbToHsl(c.r, c.g, c.b, c.a);
  hsl.s = std::clamp(hsl.s + amount, 0.0, 100.0);
  return hslToRgb(hsl.h, hsl.s, hsl.l, hsl.a);
}

static Color desaturateColor(const Color &c, double amount) {
  HSL hsl = rgbToHsl(c.r, c.g, c.b, c.a);
  hsl.s = std::clamp(hsl.s - amount, 0.0, 100.0);
  return hslToRgb(hsl.h, hsl.s, hsl.l, hsl.a);
}

static Color invertColor(const Color &c) {
  return {1.0 - c.r, 1.0 - c.g, 1.0 - c.b, c.a};
}

static Color complementColor(const Color &c) {
  HSL hsl = rgbToHsl(c.r, c.g, c.b, c.a);
  hsl.h = std::fmod(hsl.h + 180.0, 360.0);
  return hslToRgb(hsl.h, hsl.s, hsl.l, hsl.a);
}

static Color grayscaleColor(const Color &c) {
  HSL hsl = rgbToHsl(c.r, c.g, c.b, c.a);
  hsl.s = 0.0;
  return hslToRgb(hsl.h, hsl.s, hsl.l, hsl.a);
}

static Color adjustHue(const Color &c, double degrees) {
  HSL hsl = rgbToHsl(c.r, c.g, c.b, c.a);
  hsl.h = std::fmod(hsl.h + degrees, 360.0);
  if (hsl.h < 0)
    hsl.h += 360.0;
  return hslToRgb(hsl.h, hsl.s, hsl.l, hsl.a);
}

static Color mixColors(const Color &c1, const Color &c2, double weight) {
  double w = std::clamp(weight / 100.0, 0.0, 1.0);
  return {c1.r * w + c2.r * (1.0 - w), c1.g * w + c2.g * (1.0 - w),
          c1.b * w + c2.b * (1.0 - w), c1.a * w + c2.a * (1.0 - w)};
}

std::string
processColorFunction(const std::string &value,
                     const std::map<std::string, Color> &theme_colors) {
  std::string trimmed = Lawnch::Str::trim(value);

  auto paren_open = trimmed.find('(');
  if (paren_open == std::string::npos || trimmed.back() != ')') {
    Logger::log("Color", Logger::LogLevel::WARNING,
                "Malformed color function: " + value);
    return value;
  }

  std::string func_name = Lawnch::Str::trim(trimmed.substr(0, paren_open));
  std::transform(func_name.begin(), func_name.end(), func_name.begin(),
                 ::tolower);

  std::string inner =
      trimmed.substr(paren_open + 1, trimmed.size() - paren_open - 2);
  auto args = splitFuncArgs(inner);

  auto resolveArg = [&](const std::string &arg) -> Color {
    std::string a = Lawnch::Str::trim(arg);
    if (isColorFunction(a)) {
      std::string resolved = processColorFunction(a, theme_colors);
      return parseColorArg(resolved, theme_colors);
    }
    return parseColorArg(a, theme_colors);
  };

  Color result = {0, 0, 0, 1};

  if (func_name == "lighten" && args.size() == 2) {
    result = lightenColor(resolveArg(args[0]), parsePercent(args[1]));

  } else if (func_name == "darken" && args.size() == 2) {
    result = darkenColor(resolveArg(args[0]), parsePercent(args[1]));

  } else if (func_name == "opacity" && args.size() == 2) {
    result = opacityColor(resolveArg(args[0]), parsePercent(args[1]));

  } else if (func_name == "saturate" && args.size() == 2) {
    result = saturateColor(resolveArg(args[0]), parsePercent(args[1]));

  } else if (func_name == "desaturate" && args.size() == 2) {
    result = desaturateColor(resolveArg(args[0]), parsePercent(args[1]));

  } else if (func_name == "mix" && args.size() == 3) {
    result = mixColors(resolveArg(args[0]), resolveArg(args[1]),
                       parsePercent(args[2]));

  } else if (func_name == "invert" && args.size() == 1) {
    result = invertColor(resolveArg(args[0]));

  } else if (func_name == "complement" && args.size() == 1) {
    result = complementColor(resolveArg(args[0]));

  } else if (func_name == "grayscale" && args.size() == 1) {
    result = grayscaleColor(resolveArg(args[0]));

  } else if (func_name == "adjust-hue" && args.size() == 2) {
    result = adjustHue(resolveArg(args[0]), parsePercent(args[1]));

  } else {
    Logger::log("Color", Logger::LogLevel::WARNING,
                "Unknown color function or wrong argument count: " + func_name +
                    " (" + std::to_string(args.size()) + " args)");
    return value;
  }

  return colorToHex(result);
}

} // namespace Lawnch::Config
