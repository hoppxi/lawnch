#include "engine.hpp"
#include "../../helpers/string.hpp"
#include "providers/modes.hpp"

#include "../../helpers/logger.hpp"
#include <algorithm>

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

void Engine::record_usage(const std::string &command) {
  history_manager.increment(command);
}

void Engine::set_forced_mode(const std::string &trigger) {
  if (trigger.empty()) {
    forced_trigger = std::nullopt;
  } else {
    forced_trigger = trigger;
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
  std::vector<SearchResult> results;

  auto sort_results = [this](std::vector<SearchResult> &res) {
    for (auto &r : res) {
      r.score = history_manager.get_score(r.command);
    }
    std::sort(res.begin(), res.end(),
              [](const SearchResult &a, const SearchResult &b) {
                if (a.score != b.score) {
                  return a.score > b.score;
                }
                return a.name < b.name;
              });
  };

  if (term.empty() && !forced_trigger.has_value())
    return {};

  plugin_manager.ensure_plugin_for_trigger(term);

  std::string sub_query;

  if (forced_trigger.has_value()) {
    plugin_manager.ensure_plugin_for_trigger(forced_trigger.value());
    int p_count = 0;
    for (const auto &plugin : plugin_manager.get_plugins()) {
      p_count++;
      if (check_trigger(forced_trigger.value(), plugin->get_triggers(),
                        sub_query)) {
        results = plugin->query(term);
        sort_results(results);
        return results;
      }
    }

    Lawnch::Logger::log("Engine", Lawnch::Logger::LogLevel::ERROR,
                        "CRITICAL: Forced plugin trigger '" +
                            forced_trigger.value() +
                            "' configured but no matching plugin found among " +
                            std::to_string(p_count) + " loaded plugins.");

    return {};
  }

  if (term.empty())
    return {};

  for (const auto &plugin : plugin_manager.get_plugins()) {
    if (check_trigger(term, plugin->get_triggers(), sub_query)) {
      results = plugin->query(sub_query);
      sort_results(results);
      return results;
    }
  }

  if (check_trigger(term, {":help", ":h"}, sub_query)) {
    std::vector<SearchResult> help;
    for (const auto &mode : modes) {
      help.push_back(mode->get_help());
    }
    const auto &cached_help = plugin_manager.get_all_help();
    help.insert(help.end(), cached_help.begin(), cached_help.end());
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
      results = mode->query(sub_query);
      sort_results(results);
      return results;
    }
  }

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

  if (term.rfind(":", 0) == 0 && term.length() > 1) {
    Lawnch::Logger::log("Engine", Lawnch::Logger::LogLevel::WARNING,
                        "Potential plugin trigger '" + term +
                            "' detected but no plugin handled it.");
  }

  sort_results(results);
  return results;
}

} // namespace Lawnch::Core::Search
