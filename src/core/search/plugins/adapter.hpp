#pragma once

#include "../../../../plugins/lawnch_plugin_api.h"
#include "../interface.hpp"

#include <string>
#include <vector>

namespace Lawnch::Core::Search::Plugins {

class Adapter : public SearchMode {
public:
  explicit Adapter(LawnchPluginVTable *vtable);
  ~Adapter() override;

  void init_with_api(const LawnchHostApi *host_api);

  std::vector<std::string> get_triggers() const override;
  SearchResult get_help() const override;
  std::vector<SearchResult> query(const std::string &term) override;

private:
  LawnchPluginVTable *vtable;
};

} // namespace Lawnch::Core::Search::Plugins
