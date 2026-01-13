#pragma once

#include "../../config/config.hpp"
#include "../interface.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Lawnch::Core::Search::Plugins {

class PluginAdapter;
struct PluginApiContext;

class Manager {
public:
  explicit Manager(const Config::Config &config);
  ~Manager();

  Manager(const Manager &) = delete;
  Manager &operator=(const Manager &) = delete;

  const std::vector<std::unique_ptr<SearchMode>> &get_plugins() const;
  SearchMode *find_plugin(const std::string &trigger);

  const std::string &get_data_dir() const;

private:
  void load_plugins();
  void ensure_plugins_loaded();
  void load_plugin(const std::string &name);
  void find_plugin_dirs();
  void find_data_dir();

  bool plugins_loaded = false;
  const Config::Config &m_config;
  std::string m_data_dir;
  std::vector<std::string> m_plugin_dirs;
  std::vector<void *> m_handles;

  std::vector<std::unique_ptr<SearchMode>> m_plugins;
  std::vector<std::unique_ptr<PluginApiContext>> m_api_contexts;
  std::map<std::string, SearchMode *> m_trigger_map;
};

} // namespace Lawnch::Core::Search::Plugins
