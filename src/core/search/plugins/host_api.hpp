#pragma once

#include "../../config/config.hpp"
#include "../interface.hpp"
#include "lawnch_plugin_api.h"
#include <string>

namespace Lawnch::Core::Search::Plugins {

class Manager;

struct PluginApiContext {
  std::string plugin_name;
  const Config::Config &config;
  Manager *manager;
  LawnchHostApi host_api;
};

extern const LawnchLogApi g_log_api;
extern const LawnchFsApi g_fs_api;
extern const LawnchStrApi g_str_api;
extern const LawnchSearchApi g_search_api;

const char *host_get_config_value(const LawnchHostApi *host, const char *key);
const char *host_get_data_dir(const LawnchHostApi *host);
void host_request_results_update(const LawnchHostApi *host,
                                 const char *new_query);

} // namespace Lawnch::Core::Search::Plugins
