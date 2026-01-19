#pragma once

#include <string>
#include <unordered_map>

namespace Lawnch::Core::Search {

class HistoryManager {
public:
  HistoryManager();
  ~HistoryManager() = default;

  void load();
  void save();
  void increment(const std::string &command);
  int get_score(const std::string &command) const;

private:
  std::unordered_map<std::string, int> history;
  std::string cache_path;

  void find_cache_path();
};

} // namespace Lawnch::Core::Search
