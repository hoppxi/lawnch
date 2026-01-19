#pragma once

#include "config.hpp"
#include <memory>
#include <string>

namespace Lawnch::Core::Config {

class Manager {
public:
  static Manager &Instance();

  Manager(const Manager &) = delete;
  Manager &operator=(const Manager &) = delete;
  Manager(Manager &&) = delete;
  Manager &operator=(Manager &&) = delete;

  void Load(const std::string &path);
  void Merge(const std::string &path);

  [[nodiscard]] const Config &Get() const;

private:
  Manager();
  ~Manager();

  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace Lawnch::Core::Config
