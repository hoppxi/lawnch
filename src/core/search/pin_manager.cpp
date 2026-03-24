#include "pin_manager.hpp"
#include "../../helpers/fs.hpp"
#include "../../helpers/logger.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace Lawnch::Core::Search {

namespace fs = std::filesystem;

PinManager::PinManager() {
  find_cache_path();
  load();
}

void PinManager::find_cache_path() {
  fs::path cache_dir = Lawnch::Fs::get_cache_home() / "lawnch";
  if (!fs::exists(cache_dir)) {
    fs::create_directories(cache_dir);
  }
  cache_path = (cache_dir / "pins.cache").string();
}

void PinManager::load() {
  std::ifstream file(cache_path);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  PinEntry current;
  bool in_entry = false;

  while (std::getline(file, line)) {
    if (line.rfind("COMMAND:", 0) == 0) {
      current = PinEntry{};
      current.command = line.substr(8);
      in_entry = true;
    } else if (line.rfind("NAME:", 0) == 0) {
      current.name = line.substr(5);
    } else if (line.rfind("TYPE:", 0) == 0) {
      current.type = line.substr(5);
    } else if (line.rfind("ICON:", 0) == 0) {
      current.icon = line.substr(5);
    } else if (line == "END_ENTRY") {
      if (in_entry && !current.command.empty()) {
        pins.push_back(current);
      }
      current = PinEntry{};
      in_entry = false;
    }
  }
}

void PinManager::save() {
  std::ofstream file(cache_path);
  if (!file.is_open()) {
    Lawnch::Logger::log("PinManager", Lawnch::Logger::LogLevel::ERROR,
                        "Failed to open pins cache file for writing.");
    return;
  }

  for (const auto &entry : pins) {
    if (entry.command.empty())
      continue;
    file << "COMMAND:" << entry.command << "\n";
    file << "NAME:" << entry.name << "\n";
    file << "TYPE:" << entry.type << "\n";
    file << "ICON:" << entry.icon << "\n";
    file << "END_ENTRY\n";
  }
}

void PinManager::pin(const std::string &command, const std::string &name,
                     const std::string &type, const std::string &icon) {
  if (is_pinned(command)) {
    return;
  }
  PinEntry entry;
  entry.command = command;
  entry.name = name;
  entry.type = type;
  entry.icon = icon;
  pins.insert(pins.begin(), entry);
  save();
}

void PinManager::unpin(const std::string &command) {
  auto it = std::remove_if(pins.begin(), pins.end(),
                           [&](const PinEntry &e) { return e.command == command; });
  if (it != pins.end()) {
    pins.erase(it, pins.end());
    save();
  }
}

bool PinManager::is_pinned(const std::string &command) const {
  for (const auto &entry : pins) {
    if (entry.command == command) return true;
  }
  return false;
}

std::string PinManager::get_pin_type(const std::string &command) const {
  for (const auto &entry : pins) {
    if (entry.command == command) return entry.type;
  }
  return "";
}

const std::vector<PinEntry> &PinManager::get_all() const {
  return pins;
}

} // namespace Lawnch::Core::Search
