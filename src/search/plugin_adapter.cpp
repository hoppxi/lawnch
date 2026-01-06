#include "plugin_adapter.hpp"

#include <iostream>

// function to convert C-style LawnchResult to C++ SearchResult
static SearchResult to_cpp_result(const LawnchResult &c_result) {
  SearchResult cpp_result;
  cpp_result.name = c_result.name ? c_result.name : "";
  cpp_result.comment = c_result.comment ? c_result.comment : "";
  cpp_result.icon = c_result.icon ? c_result.icon : "";
  cpp_result.command = c_result.command ? c_result.command : "";
  cpp_result.type = c_result.type ? c_result.type : "";
  cpp_result.score = 0; // default
  return cpp_result;
}

PluginAdapter::PluginAdapter(LawnchPluginVTable *vtable) : vtable(vtable) {
  if (vtable->plugin_api_version != LAWNCH_PLUGIN_API_VERSION) {
    throw std::runtime_error("Plugin has incompatible API version.");
  }
}

PluginAdapter::~PluginAdapter() {
  if (vtable && vtable->destroy) {
    vtable->destroy();
  }
}

void PluginAdapter::init_with_api(const LawnchHostApi *host_api) {
  if (vtable && vtable->init) {
    vtable->init(host_api);
  }
}

std::vector<std::string> PluginAdapter::get_triggers() const {
  std::vector<std::string> triggers;
  if (vtable && vtable->get_triggers) {
    const char **c_triggers = vtable->get_triggers();
    if (c_triggers) {
      for (int i = 0; c_triggers[i] != nullptr; ++i) {
        triggers.push_back(c_triggers[i]);
      }
    }
  }
  return triggers;
}

SearchResult PluginAdapter::get_help() const {
  if (vtable && vtable->get_help) {
    LawnchResult *c_result = vtable->get_help();
    if (c_result) {
      SearchResult help_result = to_cpp_result(*c_result);
      if (vtable->free_results) {
        vtable->free_results(c_result, 1);
      }
      return help_result;
    }
  }
  // Return a default if the plugin doesn't implement help
  return SearchMode::get_help();
}

std::vector<SearchResult> PluginAdapter::query(const std::string &term) {
  std::vector<SearchResult> results;
  if (vtable && vtable->query) {
    int num_results = 0;
    LawnchResult *c_results = vtable->query(term.c_str(), &num_results);
    if (c_results && num_results > 0) {
      for (int i = 0; i < num_results; ++i) {
        results.push_back(to_cpp_result(c_results[i]));
      }

      if (vtable->free_results) {
        vtable->free_results(c_results, num_results);
      }
    }
  }
  return results;
}
