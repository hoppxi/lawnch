#pragma once
#include <string>
#include <string_view>

namespace Lawnch::Locale {

struct Info {
  std::string lang;
  std::string country;
  std::string modifier;

  bool empty() const { return lang.empty(); }
};

static Info system_locale;
static bool locale_initialized = false;

Info parse(std::string_view locale_str);
const Info &get_system();
void set_override(const std::string &locale_override);
int calculate_score(std::string_view key_locale);

} // namespace Lawnch::Locale
