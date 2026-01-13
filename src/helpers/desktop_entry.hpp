#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace Lawnch::Desktop {

struct Entry {
  std::string name;
  std::string comment;
  std::string icon;
  std::string exec;
  bool terminal = false;
  bool no_display = false;
};

std::optional<Entry> parse(const std::filesystem::path &path);

} // namespace Lawnch::Desktop
