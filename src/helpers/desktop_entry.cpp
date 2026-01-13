#include "desktop_entry.hpp"
#include "config_parse.hpp"
#include "locale.hpp"
#include "string.hpp"
#include <fstream>

namespace Lawnch::Desktop {

std::optional<Entry> parse(const std::filesystem::path &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return std::nullopt;
  }

  Entry entry;
  std::string line;
  bool in_desktop_entry = false;
  bool found_entry_group = false;
  bool has_name_any = false;
  bool has_exec = false;

  int score_name = 0;
  int score_comment = 0;
  int score_icon = 0;

  while (std::getline(file, line)) {
    std::string trimmed = Str::trim(line);

    if (trimmed.empty() || trimmed[0] == '#') {
      continue;
    }

    if (trimmed[0] == '[') {
      if (trimmed == "[Desktop Entry]") {
        in_desktop_entry = true;
        found_entry_group = true;
      } else {
        in_desktop_entry = false;
      }
      continue;
    }

    if (!in_desktop_entry) {
      continue;
    }

    size_t eq_pos = trimmed.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }

    std::string key_full = Str::trim(trimmed.substr(0, eq_pos));
    std::string value = Str::trim(trimmed.substr(eq_pos + 1));

    std::string_view key_base = key_full;
    std::string_view key_locale;

    size_t bracket = key_full.find('[');
    if (bracket != std::string::npos && key_full.back() == ']') {
      key_base = std::string_view(key_full).substr(0, bracket);
      key_locale = std::string_view(key_full).substr(
          bracket + 1, key_full.size() - bracket - 2);
    } else if (bracket != std::string::npos) {
      // malformed but has bracket, skip
      continue;
    }

    if (key_base == "Name") {
      int s = Locale::calculate_score(key_locale);
      if (s > score_name) {
        entry.name = Str::unescape(value);
        score_name = s;
        has_name_any = true;
      }
    } else if (key_base == "Comment") {
      int s = Locale::calculate_score(key_locale);
      if (s > score_comment) {
        entry.comment = Str::unescape(value);
        score_comment = s;
      }
    } else if (key_base == "Icon") {
      int s = Locale::calculate_score(key_locale);
      if (s > score_icon) {
        entry.icon = Str::unescape(value);
        score_icon = s;
      }
    } else if (key_base == "Exec" && key_locale.empty()) {
      entry.exec = Str::unescape(value);
      has_exec = true;
    } else if (key_base == "Terminal" && key_locale.empty()) {
      entry.terminal = Config::parseBool(value);
    } else if (key_base == "NoDisplay" && key_locale.empty()) {
      entry.no_display = Config::parseBool(value);
    }
  }

  if (!found_entry_group || !has_name_any || !has_exec) {
    return std::nullopt;
  }

  return entry;
}

} // namespace Lawnch::Desktop
