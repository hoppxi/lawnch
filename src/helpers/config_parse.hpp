#pragma once

#include <string>
#include <vector>

namespace Lawnch::Config {

struct Padding {
  int top, right, bottom, left;

  Padding() : top(0), right(0), bottom(0), left(0) {}
  Padding(int all) : top(all), right(all), bottom(all), left(all) {}
  Padding(int t, int r, int b, int l) : top(t), right(r), bottom(b), left(l) {}
};

struct Color {
  double r{0.0}, g{0.0}, b{0.0}, a{1.0};
};

std::vector<int> parseIntList(const std::string &value);
Padding parsePadding(const std::string &value);
bool parseBool(const std::string &s);
Color parseColor(const std::string &s);

} // namespace Lawnch::Config
