#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace Lawnch::Str {

std::string trim(const std::string &str);
std::string to_lower_copy(std::string_view str);
std::string unescape(std::string_view str);
std::string replace_all(std::string str, const std::string &from,
                        const std::string &to);

std::vector<std::string> tokenize(std::string_view str, char delimiter = ' ');

bool iequals(const std::string &a, const std::string &b);
bool contains_ic(std::string_view haystack, std::string_view needle);
int match_score(std::string_view input, std::string_view target);
size_t hash(std::string_view str);

void to_lower(std::string &str);

} // namespace Lawnch::Str
