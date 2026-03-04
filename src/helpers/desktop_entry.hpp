#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Lawnch::Desktop {

struct DesktopAction {
  std::string id;
  std::string name;
  std::string exec;
  std::string icon;
};

struct Entry {
  std::string name;
  std::string comment;
  std::string icon;
  std::string exec;
  bool terminal = false;
  bool no_display = false;
  std::vector<std::string> action_ids;
  std::vector<DesktopAction> desktop_actions;
};

std::optional<Entry> parse(const std::filesystem::path &path);

} // namespace Lawnch::Desktop
