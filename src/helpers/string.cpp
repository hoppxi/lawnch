#include "string.hpp"
#include <algorithm>
#include <cctype>
#include <functional>

namespace Lawnch::Str {

std::string to_lower_copy(std::string_view str) {
  std::string out;
  out.reserve(str.size());
  for (char c : str)
    out.push_back(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

std::string trim(const std::string &str) {
  const auto strBegin = str.find_first_not_of(" \t");
  if (strBegin == std::string::npos)
    return ""; // empty

  const auto strEnd = str.find_last_not_of(" \t");
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}

std::string unescape(std::string_view str) {
  std::string result;
  result.reserve(str.size());

  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '\\' && i + 1 < str.size()) {
      switch (str[i + 1]) {
      case 's':
        result += ' ';
        break;
      case 'n':
        result += '\n';
        break;
      case 't':
        result += '\t';
        break;
      case 'r':
        result += '\r';
        break;
      case '\\':
        result += '\\';
        break;
      default:
        result += str[i + 1];
        break;
      }
      ++i;
    } else {
      result += str[i];
    }
  }
  return result;
}

std::string escape(std::string_view str) {
  std::string result;
  result.reserve(str.size() * 2);

  for (char c : str) {
    switch (c) {
    case ' ':
      result += "\\s";
      break;
    case '\n':
      result += "\\n";
      break;
    case '\t':
      result += "\\t";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\\':
      result += "\\\\";
      break;
    default:
      result += c;
      break;
    }
  }

  return result;
}

std::string replace_all(std::string str, const std::string &from,
                        const std::string &to) {
  if (from.empty())
    return str;
  size_t pos = 0;
  while ((pos = str.find(from, pos)) != std::string::npos) {
    str.replace(pos, from.length(), to);
    pos += to.length();
  }
  return str;
}

std::vector<std::string> tokenize(std::string_view str, char delimiter) {
  std::vector<std::string> tokens;
  // pre-reserve a reasonable amount to avoid reallocations
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

bool iequals(const std::string &a, const std::string &b) {
  if (a.size() != b.size())
    return false;
  return std::equal(a.begin(), a.end(), b.begin(),
                    [](char a, char b) { return tolower(a) == tolower(b); });
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

bool is_url(const std::string &str) {
  return str.rfind("https://", 0) == 0 || str.rfind("http://", 0) == 0;
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

void to_lower(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

size_t hash(std::string_view str) {
  std::hash<std::string_view> hasher;
  return hasher(str);
}

} // namespace Lawnch::Str
