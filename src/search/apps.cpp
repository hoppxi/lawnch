#include "../utils.hpp"
#include "modes.hpp"
#include <algorithm>
#include <atomic>
#include <execution>
#include <filesystem>
#include <glib.h>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class KeyFileRAII {
public:
  KeyFileRAII() : file_(g_key_file_new()) {}
  ~KeyFileRAII() { g_key_file_free(file_); }
  GKeyFile *get() const { return file_; }

private:
  GKeyFile *file_;
};

struct DesktopEntry {
  std::string name;
  std::string comment;
  std::string icon;
  std::string exec;
  std::string name_lower;
  bool terminal;
};

static std::vector<DesktopEntry> g_index;
static std::once_flag g_index_once;
static std::shared_mutex g_index_mutex;

static inline std::string to_lower(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s)
    out.push_back(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

static void build_index() {
  std::vector<std::string> data_dirs;

  const char *xdg = getenv("XDG_DATA_DIRS");
  std::stringstream ss(xdg ? xdg : "/usr/share:/usr/local/share");

  for (std::string seg; std::getline(ss, seg, ':');)
    data_dirs.push_back(seg + "/applications");

  data_dirs.push_back(Utils::get_home_dir() + "/.local/share/applications");

  std::vector<DesktopEntry> local_index;
  std::mutex push_mutex;

  std::for_each(
      std::execution::par, data_dirs.begin(), data_dirs.end(),
      [&](const std::string &dir) {
        if (!fs::exists(dir))
          return;

        for (const auto &entry : fs::directory_iterator(dir)) {
          if (entry.path().extension() != ".desktop")
            continue;

          KeyFileRAII key;
          if (!g_key_file_load_from_file(key.get(), entry.path().c_str(),
                                         G_KEY_FILE_NONE, nullptr))
            continue;

          if (g_key_file_get_boolean(key.get(), "Desktop Entry", "NoDisplay",
                                     nullptr))
            continue;

          char *name = g_key_file_get_string(key.get(), "Desktop Entry", "Name",
                                             nullptr);
          char *exec = g_key_file_get_string(key.get(), "Desktop Entry", "Exec",
                                             nullptr);

          if (!name || !exec) {
            g_free(name);
            g_free(exec);
            continue;
          }

          char *comment = g_key_file_get_string(key.get(), "Desktop Entry",
                                                "Comment", nullptr);
          char *icon = g_key_file_get_string(key.get(), "Desktop Entry", "Icon",
                                             nullptr);

          bool terminal = g_key_file_get_boolean(key.get(), "Desktop Entry",
                                                 "Terminal", nullptr);

          std::string cmd(exec);
          if (auto pct = cmd.find('%'); pct != std::string::npos)
            cmd.erase(pct);

          DesktopEntry e{.name = name,
                         .comment = comment ? comment : "",
                         .icon = icon ? icon : "application-x-executable",
                         .exec = cmd,
                         .name_lower = to_lower(name),
                         .terminal = terminal};

          {
            std::lock_guard lock(push_mutex);
            local_index.emplace_back(std::move(e));
          }

          g_free(name);
          g_free(exec);
          g_free(comment);
          g_free(icon);
        }
      });

  std::unique_lock lock(g_index_mutex);
  g_index = std::move(local_index);
}

std::vector<SearchResult> AppMode::query(const std::string &term) {
  std::call_once(g_index_once, build_index);

  std::shared_lock lock(g_index_mutex);

  const std::string term_lower = to_lower(term);
  const bool empty = term_lower.empty();

  static const std::string terminal_cmd = Utils::get_default_terminal();

  std::vector<SearchResult> results;
  results.reserve(64);

  for (const auto &app : g_index) {
    int score = empty ? 1 : Utils::match_score(term_lower, app.name_lower);
    if (score <= 0)
      continue;

    std::string cmd =
        app.terminal ? terminal_cmd + " -e " + app.exec : app.exec;

    results.push_back({app.name, app.comment, app.icon, cmd, "app", score});
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
