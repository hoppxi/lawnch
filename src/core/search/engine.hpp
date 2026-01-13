#pragma once

#include "interface.hpp"
#include "plugins/manager.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Lawnch::Core::Search {

class Engine {
public:
  Engine(Plugins::Manager &plugin_manager);

  void set_async_callback(ResultsCallback callback);
  std::vector<SearchResult> query(const std::string &term);

private:
  Plugins::Manager &plugin_manager;
  std::vector<std::unique_ptr<SearchMode>> modes;
  ResultsCallback async_callback = nullptr;
  bool check_trigger(const std::string &term,
                     const std::vector<std::string> &triggers,
                     std::string &out_query);
};

} // namespace Lawnch::Core::Search
