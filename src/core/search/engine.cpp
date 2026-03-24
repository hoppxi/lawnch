#include "engine.hpp"
#include "../../helpers/logger.hpp"
#include "../../helpers/string.hpp"
#include "../config/manager.hpp"
#include "pin_manager.hpp"
#include "providers/modes.hpp"
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

void Engine::set_initial_mode(const std::string &trigger) {
  if (trigger.empty()) {
    initial_trigger = std::nullopt;
  } else {
    initial_trigger = trigger;
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

static void
promote_pinned_items(std::vector<SearchResult> &res,
                     const std::vector<std::string> &active_triggers) {
  auto &pin_mgr = PinManager::Instance();
  const auto &all_pins = pin_mgr.get_all();
  if (all_pins.empty())
    return;

  std::vector<SearchResult> promoted;
  std::vector<bool> is_promoted(res.size(), false);

  for (const auto &pin : all_pins) {
    bool type_matches = false;
    for (const auto &trigger : active_triggers) {
      if (pin.type == trigger) {
        type_matches = true;
        break;
      }
    }
    if (!type_matches)
      continue;

    for (size_t i = 0; i < res.size(); ++i) {
      if (!is_promoted[i] && res[i].command == pin.command) {
        res[i].is_pinned = true;
        promoted.push_back(res[i]);
        is_promoted[i] = true;
        break;
      }
    }
  }

  if (promoted.empty())
    return;

  std::vector<SearchResult> final_res;
  final_res.reserve(res.size());
  for (auto &p : promoted) {
    final_res.push_back(std::move(p));
  }
  for (size_t i = 0; i < res.size(); ++i) {
    if (!is_promoted[i]) {
      final_res.push_back(std::move(res[i]));
    }
  }
  res = std::move(final_res);
}

std::vector<SearchResult> Engine::query(const std::string &term) {
  std::vector<SearchResult> results;
  std::vector<std::string> active_triggers;

  auto sort_results = [this, &active_triggers](std::vector<SearchResult> &res) {
    for (auto &r : res) {
      if (r.use_custom_sort)
        continue;
      if (r.track_history) {
        r.score += history_manager.get_score(r.command) * 1000;
      }
    }
    std::stable_sort(res.begin(), res.end(),
                     [](const SearchResult &a, const SearchResult &b) {
                       return a.score > b.score;
                     });

    promote_pinned_items(res, active_triggers);

    int max_results = Config::Manager::Instance().Get().results_limit;
    if (max_results > 0 && res.size() > max_results) {
      res.resize(max_results);
    }
  };

  auto apply_type = [](std::vector<SearchResult> &res,
                       const std::string &type) {
    if (!type.empty()) {
      for (auto &r : res) {
        r.type = type;
      }
    }
  };

  std::string sub_query;

  if (forced_trigger.has_value()) {
    plugin_manager.ensure_plugin_for_trigger(forced_trigger.value());
    active_triggers.push_back(forced_trigger.value());

    if (auto *plugin = plugin_manager.find_plugin_for_query(
            forced_trigger.value(), sub_query)) {
      results = plugin->query(term);
      apply_type(results, plugin->get_triggers().empty()
                              ? ""
                              : plugin->get_triggers().front());
      sort_results(results);
      return results;
    }

    for (auto &mode : modes) {
      if (check_trigger(forced_trigger.value(), mode->get_triggers(),
                        sub_query)) {
        results = mode->query(term);
        apply_type(results, mode->get_triggers().empty()
                                ? ""
                                : mode->get_triggers().front());
        sort_results(results);
        return results;
      }
    }

    Lawnch::Logger::log(
        "Engine", Lawnch::Logger::LogLevel::ERROR,
        "CRITICAL: Forced context trigger '" + forced_trigger.value() +
            "' configured but no matching plugin or provider found.");
    return {};
  }

  if (term.empty() && initial_trigger.has_value()) {
    plugin_manager.ensure_plugin_for_trigger(initial_trigger.value());
    active_triggers.push_back(initial_trigger.value());

    if (auto *plugin = plugin_manager.find_plugin_for_query(
            initial_trigger.value(), sub_query)) {
      results = plugin->query("");
      apply_type(results, plugin->get_triggers().empty()
                              ? ""
                              : plugin->get_triggers().front());
      sort_results(results);
      return results;
    }

    for (auto &mode : modes) {
      if (check_trigger(initial_trigger.value(), mode->get_triggers(),
                        sub_query)) {
        results = mode->query("");
        apply_type(results, mode->get_triggers().empty()
                                ? ""
                                : mode->get_triggers().front());
        sort_results(results);
        return results;
      }
    }

    Lawnch::Logger::log("Engine", Lawnch::Logger::LogLevel::WARNING,
                        "Initial context trigger '" + initial_trigger.value() +
                            "' not found, falling back to default.");
  }

  if (term.empty()) {
    active_triggers.push_back(":app");
    active_triggers.push_back(":bin");
    sort_results(results);
    return results;
  }

  if (auto *plugin = plugin_manager.find_plugin_for_query(term, sub_query)) {
    active_triggers.push_back(
        plugin->get_triggers().empty() ? "" : plugin->get_triggers().front());
    results = plugin->query(sub_query);
    apply_type(results, plugin->get_triggers().empty()
                            ? ""
                            : plugin->get_triggers().front());
    sort_results(results);
    return results;
  }

  if (check_trigger(term, {":help", ":h", "?"}, sub_query)) {
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
      active_triggers.push_back(
          mode->get_triggers().empty() ? "" : mode->get_triggers().front());
      results = mode->query(sub_query);
      apply_type(results, mode->get_triggers().empty()
                              ? ""
                              : mode->get_triggers().front());
      sort_results(results);
      return results;
    }
  }

  if (initial_trigger.has_value()) {
    plugin_manager.ensure_plugin_for_trigger(initial_trigger.value());
    active_triggers.push_back(initial_trigger.value());

    if (auto *plugin = plugin_manager.find_plugin_for_query(
            initial_trigger.value(), sub_query)) {
      results = plugin->query(term);
      apply_type(results, plugin->get_triggers().empty()
                              ? ""
                              : plugin->get_triggers().front());
      sort_results(results);
      return results;
    }

    for (auto &mode : modes) {
      if (check_trigger(initial_trigger.value(), mode->get_triggers(),
                        sub_query)) {
        results = mode->query(term);
        apply_type(results, mode->get_triggers().empty()
                                ? ""
                                : mode->get_triggers().front());
        sort_results(results);
        return results;
      }
    }

    Lawnch::Logger::log(
        "Engine", Lawnch::Logger::LogLevel::WARNING,
        "Initial context trigger '" + initial_trigger.value() +
            "' not found for query, falling back to default search.");
  }

  active_triggers.push_back(":app");
  active_triggers.push_back(":bin");

  for (auto &mode : modes) {
    if (dynamic_cast<Providers::AppMode *>(mode.get())) {
      auto r = mode->query(term);
      apply_type(
          r, mode->get_triggers().empty() ? "" : mode->get_triggers().front());
      results.insert(results.end(), r.begin(), r.end());
    }
  }

  if (results.empty()) {
    for (auto &mode : modes) {
      if (dynamic_cast<Providers::BinMode *>(mode.get())) {
        auto r = mode->query(term);
        apply_type(r, mode->get_triggers().empty()
                          ? ""
                          : mode->get_triggers().front());
        results.insert(results.end(), r.begin(), r.end());
      }
    }
  }

  sort_results(results);
  return results;
}

std::vector<SearchResult>
Engine::query_submenu(const std::string &result_command,
                      const std::string &term) {
  std::vector<SearchResult> final_sub;
  std::string active_type;

  for (auto &mode : modes) {
    auto sub = mode->query_submenu(result_command, term);
    if (!sub.empty()) {
      final_sub = sub;
      active_type =
          mode->get_triggers().empty() ? "" : mode->get_triggers().front();
      break;
    }
  }

  if (final_sub.empty()) {
    for (auto &plugin : plugin_manager.get_plugins()) {
      auto sub = plugin->query_submenu(result_command, term);
      if (!sub.empty()) {
        final_sub = sub;
        active_type = plugin->get_triggers().empty()
                          ? ""
                          : plugin->get_triggers().front();
        break;
      }
    }
  }

  if (!active_type.empty()) {
    for (auto &r : final_sub) {
      r.type = active_type;
    }
  }

  if (!active_type.empty()) {
    std::vector<std::string> triggers = {active_type};
    promote_pinned_items(final_sub, triggers);
  }

  return final_sub;
}

} // namespace Lawnch::Core::Search
