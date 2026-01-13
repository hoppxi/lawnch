#include "locale.hpp"
#include <cstdlib>

namespace Lawnch::Locale {

Info parse(std::string_view locale_str) {
  Info loc;
  if (locale_str.empty())
    return loc;

  std::string_view current = locale_str;

  auto at_pos = current.find('@');
  if (at_pos != std::string_view::npos) {
    loc.modifier = std::string(current.substr(at_pos + 1));
    current = current.substr(0, at_pos);
  }

  auto dot_pos = current.find('.');
  if (dot_pos != std::string_view::npos) {
    current = current.substr(0, dot_pos);
  }

  auto us_pos = current.find('_');
  if (us_pos != std::string_view::npos) {
    loc.country = std::string(current.substr(us_pos + 1));
    loc.lang = std::string(current.substr(0, us_pos));
  } else {
    loc.lang = std::string(current);
  }

  return loc;
}

const Info &get_system() {
  if (!locale_initialized) {
    const char *env = std::getenv("LC_MESSAGES");
    if (!env || !*env) {
      env = std::getenv("LANG");
    }
    if (env) {
      system_locale = parse(env);
    }
    locale_initialized = true;
  }
  return system_locale;
}

void set_override(const std::string &locale_override) {
  if (!locale_override.empty()) {
    system_locale = parse(locale_override);
    locale_initialized = true;
  }
}

int calculate_score(std::string_view key_locale) {
  if (key_locale.empty())
    return 1;

  const auto &sys = get_system();
  if (sys.empty())
    return 0;

  Info key = parse(key_locale);

  if (key.lang != sys.lang)
    return 0;

  int score = 2;

  if (!key.country.empty()) {
    if (key.country == sys.country) {
      score += 2;
    } else {
      return 0;
    }
  }

  if (!key.modifier.empty()) {
    if (key.modifier == sys.modifier) {
      score += 1;
    } else {
      return 0;
    }
  }

  return score;
}

} // namespace Lawnch::Locale
