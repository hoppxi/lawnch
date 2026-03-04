#include "../../../helpers/desktop_entry.hpp"
#include "../../../helpers/fs.hpp"
#include "../../../helpers/process.hpp"
#include "../../../helpers/string.hpp"
#include "../../config/manager.hpp"
#include "modes.hpp"
#include <algorithm>
#include <execution>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace fs = std::filesystem;

namespace Lawnch::Core::Search::Providers {

struct DesktopEntry {
  std::string name;
  std::string comment;
  std::string icon;
  std::string exec;
  std::string name_lower;
  bool terminal;
  std::vector<Desktop::DesktopAction> desktop_actions;
};

static std::vector<DesktopEntry> g_index;
static std::once_flag g_index_once;
static std::shared_mutex g_index_mutex;

static void build_index() {
  auto data_dirs = ::Lawnch::Fs::get_data_dirs();

  std::vector<std::string> app_dirs;
  for (const auto &dir : data_dirs) {
    app_dirs.push_back(dir + "/applications");
  }

  std::vector<DesktopEntry> local_index;
  std::mutex push_mutex;

  std::for_each(std::execution::par, app_dirs.begin(), app_dirs.end(),
                [&](const std::string &dir) {
                  if (!fs::exists(dir))
                    return;

                  for (const auto &entry : fs::directory_iterator(dir)) {
                    if (entry.path().extension() != ".desktop")
                      continue;

                    auto parsed = Desktop::parse(entry.path());
                    if (!parsed)
                      continue;

                    if (parsed->no_display)
                      continue;

                    std::string cmd(parsed->exec);
                    if (auto pct = cmd.find('%'); pct != std::string::npos)
                      cmd.erase(pct);

                    DesktopEntry e{.name = parsed->name,
                                   .comment = parsed->comment,
                                   .icon = parsed->icon.empty()
                                               ? "application-x-executable"
                                               : parsed->icon,
                                   .exec = cmd,
                                   .name_lower = ::Lawnch::Str::to_lower_copy(
                                       parsed->name),
                                   .terminal = parsed->terminal,
                                   .desktop_actions = parsed->desktop_actions};

                    {
                      std::lock_guard lock(push_mutex);
                      local_index.emplace_back(std::move(e));
                    }
                  }
                });

  std::unique_lock lock(g_index_mutex);
  g_index = std::move(local_index);
}

std::vector<SearchResult> AppMode::query(const std::string &term) {
  std::call_once(g_index_once, build_index);

  std::shared_lock lock(g_index_mutex);

  const std::string term_lower = ::Lawnch::Str::to_lower_copy(term);

  const bool empty = term_lower.empty();

  const auto &cfg = Config::Manager::Instance().Get();
  std::string terminal_cmd = cfg.general_terminal;
  std::string terminal_flag = cfg.general_terminal_exec_flag;

  if (terminal_cmd == "auto" || terminal_cmd.empty()) {
    terminal_cmd = ::Lawnch::Proc::get_default_terminal();
  }

  std::string app_cmd_template = cfg.launch_app_cmd;
  std::string terminal_app_cmd_template = cfg.launch_terminal_app_cmd;

  std::vector<SearchResult> results;
  results.reserve(64);

  for (const auto &app : g_index) {
    int score =
        empty ? 1 : ::Lawnch::Str::match_score(term_lower, app.name_lower);
    if (score <= 0)
      continue;

    std::string cmd;
    if (app.terminal) {
      cmd = terminal_app_cmd_template;
      cmd = ::Lawnch::Str::replace_all(cmd, "{terminal}", terminal_cmd);
      cmd = ::Lawnch::Str::replace_all(cmd, "{terminal_exec_flag}",
                                       terminal_flag);
      cmd = ::Lawnch::Str::replace_all(cmd, "{}", app.exec);
    } else {
      cmd = ::Lawnch::Str::replace_all(app_cmd_template, "{}", app.exec);
    }

    results.push_back({app.name, app.comment, app.icon, cmd, "app", "", score,
                       true, false, !app.desktop_actions.empty()});
  }

  std::partial_sort(
      results.begin(), results.begin() + std::min<size_t>(50, results.size()),
      results.end(), [](const SearchResult &a, const SearchResult &b) {
        return (a.score != b.score) ? a.score > b.score : a.name < b.name;
      });

  if (results.size() > 50)
    results.resize(50);

  return results;
}

std::vector<SearchResult>
AppMode::query_submenu(const std::string &result_command,
                       const std::string &term) {
  std::call_once(g_index_once, build_index);
  std::shared_lock lock(g_index_mutex);

  const auto &cfg = Config::Manager::Instance().Get();
  std::string terminal_cmd = cfg.general_terminal;
  std::string terminal_flag = cfg.general_terminal_exec_flag;

  if (terminal_cmd == "auto" || terminal_cmd.empty()) {
    terminal_cmd = ::Lawnch::Proc::get_default_terminal();
  }

  std::string app_cmd_template = cfg.launch_app_cmd;
  std::string terminal_app_cmd_template = cfg.launch_terminal_app_cmd;

  const std::string term_lower = ::Lawnch::Str::to_lower_copy(term);

  for (const auto &app : g_index) {
    std::string app_full_cmd;
    if (app.terminal) {
      app_full_cmd = terminal_app_cmd_template;
      app_full_cmd =
          ::Lawnch::Str::replace_all(app_full_cmd, "{terminal}", terminal_cmd);
      app_full_cmd = ::Lawnch::Str::replace_all(
          app_full_cmd, "{terminal_exec_flag}", terminal_flag);
      app_full_cmd = ::Lawnch::Str::replace_all(app_full_cmd, "{}", app.exec);
    } else {
      app_full_cmd =
          ::Lawnch::Str::replace_all(app_cmd_template, "{}", app.exec);
    }

    if (app_full_cmd != result_command)
      continue;

    if (app.desktop_actions.empty())
      return {};

    std::vector<SearchResult> results;
    for (const auto &action : app.desktop_actions) {
      std::string action_name_lower = ::Lawnch::Str::to_lower_copy(action.name);
      if (!term_lower.empty() &&
          action_name_lower.find(term_lower) == std::string::npos)
        continue;

      std::string cmd;
      std::string action_exec = action.exec;
      if (auto pct = action_exec.find('%'); pct != std::string::npos)
        action_exec.erase(pct);

      if (app.terminal) {
        cmd = terminal_app_cmd_template;
        cmd = ::Lawnch::Str::replace_all(cmd, "{terminal}", terminal_cmd);
        cmd = ::Lawnch::Str::replace_all(cmd, "{terminal_exec_flag}",
                                         terminal_flag);
        cmd = ::Lawnch::Str::replace_all(cmd, "{}", action_exec);
      } else {
        cmd = ::Lawnch::Str::replace_all(app_cmd_template, "{}", action_exec);
      }

      std::string icon = action.icon.empty() ? app.icon : action.icon;
      results.push_back({action.name, app.name + " → " + action.name, icon, cmd,
                         "app", "", 0, true, false, false});
    }
    return results;
  }

  return {};
}

} // namespace Lawnch::Core::Search::Providers
