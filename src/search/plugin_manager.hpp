#pragma once

#include "../config.hpp"
#include "interface.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

class PluginAdapter;
struct PluginApiContext;

class PluginManager {
public:
  explicit PluginManager(Config &config);
  ~PluginManager();

  void load_plugins();
  SearchMode *find_plugin(const std::string &trigger);
  const std::vector<std::unique_ptr<SearchMode>> &get_plugins() const;
  const std::string &get_data_dir() const;

private:
  void load_plugin(const std::string &name);
  void find_plugin_dirs();
  void find_data_dir();

  Config &config;
  std::string data_dir;
  std::vector<std::string> plugin_dirs;
  std::vector<void *> handles;
  std::vector<std::unique_ptr<SearchMode>> plugins;
  std::vector<std::unique_ptr<PluginApiContext>> api_contexts;
  std::map<std::string, SearchMode *> trigger_map;
};
