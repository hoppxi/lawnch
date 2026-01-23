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

  std::string get_plugin_data_dir(const std::string &plugin_name) const;

private:
  void load_plugins();
  void ensure_plugins_loaded();
  void load_plugin(const std::string &name);
  void find_plugin_dirs();
  std::string find_plugin_data_dir(const std::string &plugin_name) const;

  bool plugins_loaded = false;
  const Config::Config &m_config;
  std::vector<std::string> m_plugin_dirs;
  mutable std::map<std::string, std::string> m_plugin_data_dirs;
  std::vector<void *> m_handles;

  std::vector<std::unique_ptr<SearchMode>> m_plugins;
  std::vector<std::unique_ptr<PluginApiContext>> m_api_contexts;
  std::map<std::string, SearchMode *> m_trigger_map;

  std::map<std::string, std::string> m_lazy_triggers;
  std::vector<SearchResult> m_cached_help;
  void load_triggers_cache();
  void save_triggers_cache();
  void generate_triggers_cache();

public:
  void ensure_plugin_for_trigger(const std::string &query);
  const std::vector<SearchResult> &get_all_help() const;
};

} // namespace Lawnch::Core::Search::Plugins
