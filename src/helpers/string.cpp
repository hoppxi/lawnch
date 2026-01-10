#include "string.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cctype>

namespace Lawnch::Str {

void to_lower(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

std::vector<std::string> tokenize(std::string_view str, char delimiter) {
  std::vector<std::string> tokens;
  // Pre-reserve a reasonable amount to avoid reallocations
  tokens.reserve(4);

  size_t start = 0;
  size_t end = str.find(delimiter);

  while (end != std::string_view::npos) {
    std::string token(str.substr(start, end - start));
    to_lower(token);
    if (!token.empty())
      tokens.push_back(std::move(token));

    start = end + 1;
    end = str.find(delimiter, start);
  }

  std::string last_token(str.substr(start));
  to_lower(last_token);
  if (!last_token.empty())
    tokens.push_back(std::move(last_token));

  return tokens;
}

bool contains_ic(std::string_view haystack, std::string_view needle) {
  auto it =
      std::search(haystack.begin(), haystack.end(), needle.begin(),
                  needle.end(), [](char ch1, char ch2) {
                    return std::toupper(static_cast<unsigned char>(ch1)) ==
                           std::toupper(static_cast<unsigned char>(ch2));
                  });
  return (it != haystack.end());
}

int match_score(std::string_view input, std::string_view target) {
  if (input.empty())
    return 1;

  std::string in_lower(input);
  std::string tg_lower(target);
  to_lower(in_lower);
  to_lower(tg_lower);

  if (tg_lower == in_lower)
    return 100;
  if (tg_lower.find(in_lower) == 0)
    return 80;
  if (tg_lower.find(in_lower) != std::string::npos)
    return 50;

  return 0;
}

std::string trim(const std::string &str) {
  const auto strBegin = str.find_first_not_of(" \t");
  if (strBegin == std::string::npos)
    return ""; // empty

  const auto strEnd = str.find_last_not_of(" \t");
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}

bool parseBool(const std::string &s) {
  std::string v = s;
  std::transform(v.begin(), v.end(), v.begin(), ::tolower);
  return (v == "true" || v == "1" || v == "yes" || v == "on");
}

Color parseColor(const std::string &s) {
  Color c = {0, 0, 0, 1};
  std::string clean = trim(s);

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
}
} // namespace Lawnch::Str
