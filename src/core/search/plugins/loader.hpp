#pragma once

#include "../interface.hpp"
#include "lawnch_plugin_api.h"
#include <filesystem>
#include <string>
#include <vector>

namespace Lawnch::Core::Search::Plugins {

struct LoadedPlugin {
  void *handle = nullptr;
  LawnchPluginVTable *vtable = nullptr;
  std::string found_path;
};

std::vector<std::string> find_plugin_dirs();

std::string find_plugin_data_dir(const std::string &plugin_name,
                                 const std::vector<std::string> &plugin_dirs);

LoadedPlugin load_plugin_vtable(const std::string &name,
                                const std::vector<std::string> &plugin_dirs);

} // namespace Lawnch::Core::Search::Plugins
