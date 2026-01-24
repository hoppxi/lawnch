#include "history.hpp"
#include "../../helpers/fs.hpp"
#include "../../helpers/logger.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

#include "../config/manager.hpp"

namespace Lawnch::Core::Search {

namespace fs = std::filesystem;

HistoryManager::HistoryManager() {
  find_cache_path();
  load();
}

void HistoryManager::find_cache_path() {
  fs::path cache_dir = Lawnch::Fs::get_cache_home() / "lawnch";
  if (!fs::exists(cache_dir)) {
    fs::create_directories(cache_dir);
  }
  cache_path = (cache_dir / "history.cache").string();
}

void HistoryManager::load() {
  if (!Config::Manager::Instance().Get().general_history)
    return;

  std::ifstream file(cache_path);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  std::string current_command;
  int current_count = 0;

  while (std::getline(file, line)) {
    if (line.rfind("COMMAND:", 0) == 0) {
      if (!current_command.empty() && current_count > 0) {
        history[current_command] = current_count;
      }
      current_command = line.substr(8);
      current_count = 0;
    } else if (line.rfind("COUNT:", 0) == 0) {
      try {
        current_count = std::stoi(line.substr(6));
      } catch (...) {
        current_count = 0;
      }
    } else if (line == "END_ENTRY") {
      if (!current_command.empty() && current_count > 0) {
        history[current_command] = current_count;
      }
      current_command = "";
      current_count = 0;
    }
  }

  if (!current_command.empty() && current_count > 0) {
    history[current_command] = current_count;
  }
}

void HistoryManager::save() {
  if (!Config::Manager::Instance().Get().general_history)
    return;

  std::ofstream file(cache_path);
  if (!file.is_open()) {
    Lawnch::Logger::log("HistoryManager", Lawnch::Logger::LogLevel::ERROR,
                        "Failed to open history cache file for writing.");
    return;
  }

  std::vector<std::pair<std::string, int>> sorted_history(history.begin(),
                                                          history.end());
  std::sort(sorted_history.begin(), sorted_history.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  for (const auto &[cmd, count] : sorted_history) {
    if (count <= 0)
      continue;
    file << "COMMAND:" << cmd << "\n";
    file << "COUNT:" << count << "\n";
    file << "END_ENTRY\n";
  }
}

void HistoryManager::increment(const std::string &command) {
  if (!Config::Manager::Instance().Get().general_history)
    return;

  if (command.empty())
    return;
  history[command]++;
  save();
}

int HistoryManager::get_score(const std::string &command) const {
  if (!Config::Manager::Instance().Get().general_history)
    return 0;

  auto it = history.find(command);
  if (it != history.end()) {
    return it->second;
  }
  return 0;
}

} // namespace Lawnch::Core::Search
