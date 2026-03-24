#pragma once

#include <string>
#include <vector>

namespace Lawnch::Core::Search {

struct PinEntry {
  std::string command;
  std::string name;
  std::string type;
  std::string icon;
};

class PinManager {
public:
  static PinManager &Instance() {
    static PinManager instance;
    return instance;
  }

  void load();
  void save();

  void pin(const std::string &command, const std::string &name,
           const std::string &type, const std::string &icon);
  void unpin(const std::string &command);
  bool is_pinned(const std::string &command) const;
  std::string get_pin_type(const std::string &command) const;
  const std::vector<PinEntry> &get_all() const;

private:
  PinManager();
  ~PinManager() = default;

  PinManager(const PinManager &) = delete;
  PinManager &operator=(const PinManager &) = delete;

  std::vector<PinEntry> pins;
  std::string cache_path;

  void find_cache_path();
};

} // namespace Lawnch::Core::Search
