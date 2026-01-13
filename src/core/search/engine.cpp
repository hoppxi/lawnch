#include "engine.hpp"
#include "../../helpers/string.hpp"
#include "providers/modes.hpp"

namespace Lawnch::Core::Search {

Engine::Engine(Plugins::Manager &pm) : plugin_manager(pm) {
  modes.push_back(std::make_unique<Providers::AppMode>());
  modes.push_back(std::make_unique<Providers::BinMode>());

  for (auto &mode : modes) {
    mode->init();
  }
}

void Engine::set_async_callback(ResultsCallback callback) {
  async_callback = callback;
  for (auto &mode : modes) {
    mode->set_async_callback(callback);
  }
}

bool Engine::check_trigger(const std::string &term,
                           const std::vector<std::string> &triggers,
                           std::string &out_query) {
  for (const auto &t : triggers) {
    if (t.empty())
      continue;
    if (term == t) {
      out_query = "";
      return true;
    }
    if (term.rfind(t + " ", 0) == 0) {
      out_query = term.substr(t.length() + 1);
      return true;
    }
  }
  return false;
}

std::vector<SearchResult> Engine::query(const std::string &term) {
  if (term.empty())
    return {};

  std::string sub_query;

  for (const auto &plugin : plugin_manager.get_plugins()) {
    if (check_trigger(term, plugin->get_triggers(), sub_query)) {
      return plugin->query(sub_query);
    }
  }

  if (check_trigger(term, {":help", ":h"}, sub_query)) {
    std::vector<SearchResult> help;
    for (const auto &mode : modes) {
      help.push_back(mode->get_help());
    }
    for (const auto &plugin : plugin_manager.get_plugins()) {
      help.push_back(plugin->get_help());
    }
    if (!sub_query.empty()) {
      std::vector<SearchResult> filtered;
      for (const auto &h : help) {
        if (Lawnch::Str::contains_ic(h.name, sub_query) ||
            Lawnch::Str::contains_ic(h.comment, sub_query)) {
          filtered.push_back(h);
        }
      }
      return filtered;
    }
    return help;
  }

  for (auto &mode : modes) {
    if (check_trigger(term, mode->get_triggers(), sub_query)) {
      return mode->query(sub_query);
    }
  }

  std::vector<SearchResult> results;

  for (auto &mode : modes) {
    if (dynamic_cast<Providers::AppMode *>(mode.get())) {
      auto r = mode->query(term);
      results.insert(results.end(), r.begin(), r.end());
    }
  }

  if (results.empty()) {
    for (auto &mode : modes) {
      if (dynamic_cast<Providers::BinMode *>(mode.get())) {
        auto r = mode->query(term);
        results.insert(results.end(), r.begin(), r.end());
      }
    }
  }

  return results;
}

} // namespace Lawnch::Core::Search
