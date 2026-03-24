#pragma once

#include "../../config/config.hpp"
#include "../interface.hpp"
#include "host_api.hpp"
#include "trigger_cache.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Lawnch::Core::Search::Plugins {

class PluginAdapter;

class Manager {
public:
  explicit Manager(const Config::Config &config);
  ~Manager();

  Manager(const Manager &) = delete;
  Manager &operator=(const Manager &) = delete;

  const std::vector<std::unique_ptr<SearchMode>> &get_plugins() const;
  SearchMode *find_plugin(const std::string &trigger);

  using UpdateResultsCallback = std::function<void(const std::string& plugin_name, const std::string& new_query)>;
  void set_update_results_callback(UpdateResultsCallback cb) { update_callback = std::move(cb); }
  void fire_update_callback(const std::string& plugin_name, const std::string& new_query) const {
    if (update_callback) update_callback(plugin_name, new_query);
  }

  std::string get_plugin_data_dir(const std::string &plugin_name) const;

  void ensure_plugin_for_trigger(const std::string &query);
  const std::vector<SearchResult> &get_all_help() const;
  const std::vector<std::string> &
  get_triggers_for(const SearchMode *plugin) const;
  SearchMode *find_plugin_for_query(const std::string &term,
                                    std::string &out_query);

private:
  void load_plugins();
  void ensure_plugins_loaded();
  void load_plugin(const std::string &name);
  void generate_triggers_cache();
  std::string build_config_signature() const;

  bool plugins_loaded = false;
  UpdateResultsCallback update_callback;
  const Config::Config &m_config;
  std::vector<std::string> m_plugin_dirs;
  mutable std::map<std::string, std::string> m_plugin_data_dirs;
  std::vector<void *> m_handles;

  std::vector<std::unique_ptr<SearchMode>> m_plugins;
  std::vector<std::unique_ptr<PluginApiContext>> m_api_contexts;
  std::map<std::string, SearchMode *> m_trigger_map;
  std::map<std::string, std::vector<std::string>> m_loaded_triggers;
  std::map<std::string, SearchResult> m_loaded_help;
  std::map<const SearchMode *, std::vector<std::string>> m_plugin_triggers;

  TriggerCache m_trigger_cache;
  mutable std::vector<SearchResult> m_cached_help;
};

} // namespace Lawnch::Core::Search::Plugins
