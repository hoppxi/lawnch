#pragma once

#include "../interface.hpp"
#include <map>
#include <string>
#include <vector>

namespace Lawnch::Core::Search::Plugins {

struct TriggerCacheData {
  std::map<std::string, std::string> lazy_triggers;
  std::vector<SearchResult> cached_help;
};

class TriggerCache {
public:
  bool load(const std::string &config_signature);

  void
  save(const std::string &config_signature,
       const std::map<std::string, std::vector<std::string>> &loaded_triggers,
       const std::map<std::string, SearchResult> &loaded_help,
       const std::vector<std::string> &enabled_plugins);

  const TriggerCacheData &data() const { return m_data; }

private:
  TriggerCacheData m_data;
  std::string get_cache_path() const;
};

} // namespace Lawnch::Core::Search::Plugins
