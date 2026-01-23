#pragma once

#include <string>

namespace Lawnch::Core::Config::Validator {
bool isValidConfigKey(const std::string &key);
bool isValidThemeKey(const std::string &key);
} // namespace Lawnch::Core::Config::Validator
