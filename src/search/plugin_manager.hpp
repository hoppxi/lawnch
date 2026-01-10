#pragma once

#include "../config/config_types.hpp"
#include "interface.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

class PluginAdapter;
struct PluginApiContext;

class PluginManager {
public:
  explicit PluginManager(const Config &config);
  ~PluginManager();

  PluginManager(const PluginManager &) = delete;
  PluginManager &operator=(const PluginManager &) = delete;

  void load_plugins();
  SearchMode *find_plugin(const std::string &trigger);
  const std::vector<std::unique_ptr<SearchMode>> &get_plugins() const;
  const std::string &get_data_dir() const;

private:
  void load_plugin(const std::string &name);
  void find_plugin_dirs();
  void find_data_dir();

  const Config &m_config;
  std::string m_data_dir;
  std::vector<std::string> m_plugin_dirs;
  std::vector<void *> m_handles;

  std::vector<std::unique_ptr<SearchMode>> m_plugins;
  std::vector<std::unique_ptr<PluginApiContext>> m_api_contexts;
  std::map<std::string, SearchMode *> m_trigger_map;
};
