#include "../../../helpers/logger.hpp"
#include "../../../helpers/process.hpp"
#include "../../../helpers/string.hpp"
#include "../../config/manager.hpp"
#include "modes.hpp"
#include <filesystem>
#include <mutex>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace Lawnch::Core::Search::Providers {

namespace {
std::vector<fs::path> cached_paths;
std::once_flag path_init_flag;

void init_paths() {
  const char *path_env = getenv("PATH");
  if (!path_env) {
    Logger::log("Bins", Logger::LogLevel::WARNING,
                "PATH environment variable not set");
    return;
  }

  std::stringstream ss(path_env);
  std::string dir;
  while (std::getline(ss, dir, ':')) {
    if (fs::exists(dir))
      cached_paths.emplace_back(dir);
  }

  Logger::log("Bins", Logger::LogLevel::INFO,
              "Indexed " + std::to_string(cached_paths.size()) +
                  " PATH directories");
}
} // namespace

std::vector<SearchResult> BinMode::query(const std::string &term) {
  std::call_once(path_init_flag, init_paths);

  const auto &cfg = Config::Manager::Instance().Get();
  bool track_history = cfg.providers_bins_history;
  bool terminal_exec = cfg.providers_bins_terminal_exec;
  std::string exec_mode = cfg.providers_bins_exec;

  std::string terminal_cmd = cfg.general_terminal;
  std::string terminal_flag = cfg.general_terminal_flag;

  if (terminal_cmd == "auto" || terminal_cmd.empty()) {
    terminal_cmd = ::Lawnch::Proc::get_default_terminal();
  }

  std::vector<SearchResult> results;

  for (const auto &dir : cached_paths) {
    for (const auto &entry : fs::directory_iterator(dir)) {

      const std::string name = entry.path().filename().string();
      if (!Lawnch::Str::contains_ic(name, term))
        continue;

      if (!Lawnch::Proc::is_executable(entry.path()))
        continue;

      std::string cmd = entry.path().string();

      if (terminal_exec) {
        cmd = terminal_cmd + " " + terminal_flag + " " + cmd;
      }

      results.push_back({name, entry.path().string(), "utilities-terminal", cmd,
                         "bin", "", 0, track_history, false, false});
    }
  }

  Logger::log("Bins", Logger::LogLevel::DEBUG,
              "Query '" + term + "' returned " +
                  std::to_string(results.size()) + " results");

  return results;
}

} // namespace Lawnch::Core::Search::Providers
