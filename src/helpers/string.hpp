#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace Lawnch::Str {

struct Color {
  double r{0.0}, g{0.0}, b{0.0}, a{1.0};
};

std::string trim(const std::string &str);
std::vector<std::string> tokenize(std::string_view str, char delimiter = ' ');
int match_score(std::string_view input, std::string_view target);
void to_lower(std::string &str);
bool contains_ic(std::string_view haystack, std::string_view needle);
bool parseBool(const std::string &s);
Color parseColor(const std::string &s);

} // namespace Lawnch::Str
