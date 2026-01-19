#pragma once

#include "history.hpp"
#include "interface.hpp"
#include "plugins/manager.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Lawnch::Core::Search {

class Engine {
public:
  Engine(Plugins::Manager &plugin_manager);

  void set_async_callback(ResultsCallback callback);
  void set_forced_mode(const std::string &trigger);
  std::vector<SearchResult> query(const std::string &term);

  void record_usage(const std::string &command);

private:
  Plugins::Manager &plugin_manager;
  HistoryManager history_manager;
  std::vector<std::unique_ptr<SearchMode>> modes;
  ResultsCallback async_callback = nullptr;
  bool check_trigger(const std::string &term,
                     const std::vector<std::string> &triggers,
                     std::string &out_query);

  std::optional<std::string> forced_trigger;
};

} // namespace Lawnch::Core::Search
