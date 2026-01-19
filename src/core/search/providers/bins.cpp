#include "../../../helpers/process.hpp"
#include "../../../helpers/string.hpp"
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
  if (!path_env)
    return;

  std::stringstream ss(path_env);
  std::string dir;
  while (std::getline(ss, dir, ':')) {
    if (fs::exists(dir))
      cached_paths.emplace_back(dir);
  }
}
} // namespace

std::vector<SearchResult> BinMode::query(const std::string &term) {
  std::call_once(path_init_flag, init_paths);

  std::vector<SearchResult> results;
  results.reserve(20);

  if (term.empty())
    return results;

  for (const auto &dir : cached_paths) {
    for (const auto &entry : fs::directory_iterator(dir)) {
      if (results.size() >= 20)
        return results;

      const std::string name = entry.path().filename().string();
      if (!Lawnch::Str::contains_ic(name, term))
        continue;

      if (!Lawnch::Proc::is_executable(entry.path()))
        continue;

      results.push_back({name, entry.path().string(), "utilities-terminal",
                         entry.path().string(), "bin"});
    }
  }
  return results;
}

} // namespace Lawnch::Core::Search::Providers
