#include "search.hpp"
#include "string.hpp"
#include <algorithm>
#include <vector>

namespace Lawnch::Helpers::Search {

int fuzzy_match(const std::string &pattern, const std::string &str) {
  if (pattern.empty()) return 100;
  if (str.empty()) return 0;

  std::string lower_pattern = ::Lawnch::Str::to_lower_copy(pattern);
  std::string lower_str = ::Lawnch::Str::to_lower_copy(str);

  int score = 0;
  size_t p_idx = 0;
  size_t s_idx = 0;

  bool consecutive_match = false;

  while (p_idx < lower_pattern.length() && s_idx < lower_str.length()) {
    if (lower_pattern[p_idx] == lower_str[s_idx]) {
      score += 10;
      if (consecutive_match) {
        score += 5;
      }
      if (s_idx == 0 || lower_str[s_idx - 1] == ' ') {
        score += 15;
      }
      consecutive_match = true;
      p_idx++;
    } else {
      consecutive_match = false;
    }
    s_idx++;
  }

  if (p_idx == lower_pattern.length()) {
    if (lower_pattern.length() == lower_str.length()) {
      score += 50;
    }
    return score;
  }

  return 0;
}

int multi_word_match(const std::string &pattern, const std::string &str) {
  if (pattern.empty()) return 100;
  if (str.empty()) return 0;

  std::string lower_pattern = ::Lawnch::Str::to_lower_copy(pattern);
  std::string lower_str = ::Lawnch::Str::to_lower_copy(str);

  std::vector<std::string> words = ::Lawnch::Str::tokenize(lower_pattern, ' ');

  if (words.empty()) return 100;

  int score = 0;
  for (const auto &word : words) {
    if (word.empty()) continue;
    size_t pos = lower_str.find(word);
    if (pos == std::string::npos) {
      return 0;
    }
    score += 10;
    if (pos == 0 || lower_str[pos - 1] == ' ') {
      score += 5;
    }
  }

  return score;
}

int levenshtein_distance(const std::string &s1, const std::string &s2) {
  const size_t len1 = s1.size(), len2 = s2.size();
  std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

  d[0][0] = 0;
  for(size_t i = 1; i <= len1; ++i) d[i][0] = i;
  for(size_t i = 1; i <= len2; ++i) d[0][i] = i;

  for(size_t i = 1; i <= len1; ++i) {
    for(size_t j = 1; j <= len2; ++j) {
      int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
      d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost });
    }
  }

  return d[len1][len2];
}

double jaro_winkler_similarity(const std::string &s1, const std::string &s2) {
  if (s1 == s2) return 1.0;

  const int len1 = s1.length(), len2 = s2.length();
  if (len1 == 0 || len2 == 0) return 0.0;

  const int match_distance = std::max(len1, len2) / 2 - 1;

  std::vector<bool> s1_matches(len1, false);
  std::vector<bool> s2_matches(len2, false);

  int matches = 0;
  for (int i = 0; i < len1; i++) {
    int start = std::max(0, i - match_distance);
    int end = std::min(i + match_distance + 1, len2);

    for (int j = start; j < end; j++) {
      if (s2_matches[j]) continue;
      if (s1[i] != s2[j]) continue;
      s1_matches[i] = true;
      s2_matches[j] = true;
      matches++;
      break;
    }
  }

  if (matches == 0) return 0.0;

  int transpositions = 0;
  int k = 0;
  for (int i = 0; i < len1; i++) {
    if (!s1_matches[i]) continue;
    while (!s2_matches[k]) k++;
    if (s1[i] != s2[k]) transpositions++;
    k++;
  }
  transpositions /= 2;

  double m = static_cast<double>(matches);
  double jaro = ((m / len1) + (m / len2) + ((m - transpositions) / m)) / 3.0;

  int prefix = 0;
  for (int i = 0; i < std::min(4, std::min(len1, len2)); i++) {
    if (s1[i] == s2[i]) prefix++;
    else break;
  }

  double jaro_winkler = jaro + (prefix * 0.1 * (1.0 - jaro));
  return std::min(1.0, std::max(0.0, jaro_winkler));
}

int calculate_advanced_score(const std::string &query, const std::string &target) {
  if (query.empty()) return 100;
  if (target.empty()) return 0;

  std::string lower_query = ::Lawnch::Str::to_lower_copy(query);
  std::string lower_target = ::Lawnch::Str::to_lower_copy(target);

  if (lower_query == lower_target) return 10000;

  int score = 0;

  int fm = fuzzy_match(lower_query, lower_target);
  int mw = multi_word_match(lower_query, lower_target);

  // If both failed, we can consider Jaro-Winkler for slight typos
  if (fm == 0 && mw == 0) {
    if (lower_query.length() >= 3) {
      double jw = jaro_winkler_similarity(lower_query, lower_target);
      if (jw > 0.85) {
        return static_cast<int>(jw * 100);
      }
    }
    return 0;
  }

  if (lower_target.rfind(lower_query, 0) == 0) {
    score += 5000;
  } else if (lower_target.find(" " + lower_query) != std::string::npos ||
             lower_target.find("-" + lower_query) != std::string::npos ||
             lower_target.find("_" + lower_query) != std::string::npos) {
    score += 3000;
  } else if (lower_target.find(lower_query) != std::string::npos) {
    score += 1000;
  }

  if (mw > 0) score += 500 + mw;
  if (fm > 0) score += fm;

  score -= lower_target.length();

  return std::max(0, score);
}

} // namespace Lawnch::Helpers::Search
