#pragma once

#include "../../plugins/lawnch_plugin_api.h"
#include "interface.hpp"

#include <string>
#include <vector>

// Acts as a C++ wrapper around a loaded C plugin which
// adapts its C API to the C++ SearchMode interface.
class PluginAdapter : public SearchMode {
public:
  explicit PluginAdapter(LawnchPluginVTable *vtable);
  ~PluginAdapter() override;

  void init_with_api(const LawnchHostApi *host_api);

  std::vector<std::string> get_triggers() const override;
  SearchResult get_help() const override;
  std::vector<SearchResult> query(const std::string &term) override;

private:
  LawnchPluginVTable *vtable;
};
