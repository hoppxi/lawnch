#pragma once

#include "config_types.hpp"
#include <memory>
#include <string>

// use the Pimpl idiom to hide internal details from the public API.

class ConfigManager {
public:
  static ConfigManager &Instance();

  ConfigManager(const ConfigManager &) = delete;
  ConfigManager &operator=(const ConfigManager &) = delete;
  ConfigManager(ConfigManager &&) = delete;
  ConfigManager &operator=(ConfigManager &&) = delete;

  void Load(const std::string &path);

  [[nodiscard]] const Config &Get() const;

private:
  ConfigManager();
  ~ConfigManager();

  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
