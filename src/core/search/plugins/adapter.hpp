#pragma once

#include "../interface.hpp"
#include "lawnch_plugin_api.h"

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

  bool allow_history() const override;
  bool is_custom_sorted() const override;

private:
  LawnchPluginVTable *vtable;
  uint32_t flags = 0;
  mutable bool triggers_cached = false;
  mutable std::vector<std::string> cached_triggers;
  mutable bool help_cached = false;
  mutable SearchResult cached_help;
};

} // namespace Lawnch::Core::Search::Plugins
