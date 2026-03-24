#pragma once

#include <string>
#include <vector>
#include <toml++/toml.hpp>

namespace Lawnch::Core::Config::Validator {

struct ValidationResult {
  bool success;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
};

ValidationResult validateConfig(const toml::table &root, bool is_preset = false);
ValidationResult validateTheme(const toml::table &root);

} // namespace Lawnch::Core::Config::Validator
