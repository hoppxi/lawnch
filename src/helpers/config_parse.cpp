#include "config_parse.hpp"
#include "logger.hpp"
#include "string.hpp"
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace Lawnch::Config {

std::vector<int> parseIntList(const std::string &value) {
  std::vector<int> result;
  std::string trimmed = Lawnch::Str::trim(value);

  if (trimmed.empty()) {
    return result;
  }

  std::stringstream ss(trimmed);
  std::string token;

  while (std::getline(ss, token, ',')) {
    token = Lawnch::Str::trim(token);
    if (!token.empty()) {
      try {
        result.push_back(std::stoi(token));
      } catch (...) {
      }
    }
  }

  return result;
}

Padding parsePadding(const std::string &value) {
  auto values = parseIntList(value);

  if (values.empty()) {
    return Padding(0);
  }

  if (values.size() == 1) {
    // padding=10 - all sides
    return Padding(values[0]);
  } else if (values.size() == 2) {
    // padding=10,20 - vertical, horizontal
    return Padding(values[0], values[1], values[0], values[1]);
  } else if (values.size() == 3) {
    // padding=10,20,30 - top, horizontal, bottom
    return Padding(values[0], values[1], values[2], values[1]);
  } else {
    // padding=10,20,30,40 - top, right, bottom, left
    return Padding(values[0], values[1], values[2], values[3]);
  }
}

bool parseBool(const std::string &s) {
  std::string v = s;
  std::transform(v.begin(), v.end(), v.begin(), ::tolower);
  return (v == "true" || v == "1" || v == "yes" || v == "on");
}

Color parseColor(const std::string &s) {
  Color c = {0, 0, 0, 1};
  std::string clean = Str::trim(s);

  if (clean.rfind("rgba(", 0) == 0 && clean.back() == ')') {
    double r = 0, g = 0, b = 0, a = 1.0;
    if (std::sscanf(clean.c_str(), "rgba(%lf, %lf, %lf, %lf)", &r, &g, &b,
                    &a) == 4) {
      c.r = std::clamp(r / 255.0, 0.0, 1.0);
      c.g = std::clamp(g / 255.0, 0.0, 1.0);
      c.b = std::clamp(b / 255.0, 0.0, 1.0);
      c.a = std::clamp(a, 0.0, 1.0);
    } else {
      Logger::log("Config", Logger::LogLevel::WARNING,
                  "Malformed color format: " + s + ". Expected rgba(r,g,b,a).");
    }
  } else {
    // TODO: support for hex color format
    // currenly only supports rgba color format
    Logger::log("Config", Logger::LogLevel::WARNING,
                "Unsupported color format: " + s);
  }
  return c;
  return c;
}

} // namespace Lawnch::Config
