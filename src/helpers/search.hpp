#pragma once

#include <string>

namespace Lawnch::Helpers::Search {
int fuzzy_match(const std::string &pattern, const std::string &str);
int multi_word_match(const std::string &pattern, const std::string &str);
int levenshtein_distance(const std::string &s1, const std::string &s2);
double jaro_winkler_similarity(const std::string &s1, const std::string &s2);
int calculate_advanced_score(const std::string &query,
                             const std::string &target);
} // namespace Lawnch::Helpers::Search
