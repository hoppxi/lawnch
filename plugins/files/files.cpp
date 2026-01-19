#include "lawnch_plugin_api.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

struct FileEntry {
  fs::path path;
  std::string name;
  std::string name_lower;
  bool is_dir;
};

struct DirIndex {
  std::vector<FileEntry> files;
  std::shared_mutex mutex;
  std::atomic<bool> scanning{false};
  std::atomic<bool> stop{false};
  std::once_flag once;
};

static std::unordered_map<std::string, DirIndex> g_indexes;
static std::mutex g_index_map_mutex;

static size_t g_max_results = 50;
static int g_max_depth = 4;

static char *c_strdup(const std::string &s) {
  char *c = new char[s.size() + 1];
  std::memcpy(c, s.c_str(), s.size() + 1);
  return c;
}

static std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

static std::string expand_home(const std::string &p) {
  if (!p.empty() && p[0] == '~') {
    const char *home = std::getenv("HOME");
    if (home)
      return std::string(home) + p.substr(1);
  }
  return p;
}

static bool is_hidden(const fs::path &p) {
  auto name = p.filename().string();
  return !name.empty() && name[0] == '.';
}

static void scan_dir(DirIndex *index, fs::path root, int max_depth) {
  index->scanning = true;

  std::vector<std::pair<fs::path, int>> stack;
  stack.emplace_back(root, 0);

  const auto slice = std::chrono::milliseconds(6);

  while (!stack.empty() && !index->stop.load()) {
    auto start = std::chrono::steady_clock::now();

    while (!stack.empty()) {
      auto [dir, depth] = stack.back();
      stack.pop_back();

      if (depth > max_depth || !fs::exists(dir))
        continue;

      std::error_code ec;
      for (const auto &e : fs::directory_iterator(dir, ec)) {
        if (ec || is_hidden(e.path()))
          continue;

        bool is_dir = e.is_directory(ec);
        if (ec)
          continue;

        FileEntry fe;
        fe.path = e.path();
        fe.name = fe.path.filename().string();
        fe.name_lower = to_lower(fe.name);
        fe.is_dir = is_dir;

        {
          std::unique_lock lock(index->mutex);
          index->files.emplace_back(std::move(fe));
        }

        if (is_dir)
          stack.emplace_back(e.path(), depth + 1);
      }

      if (std::chrono::steady_clock::now() - start > slice)
        break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  index->scanning = false;
}

static void parse_query(const std::string &term, fs::path &out_dir,
                        std::string &out_query) {
  out_dir = expand_home("~");
  out_query = term;

  auto pos = term.find(":dir");
  if (pos == std::string::npos)
    return;

  size_t start = term.find_first_not_of(" ", pos + 4);
  if (start == std::string::npos)
    return;

  if (term[start] == '\'' || term[start] == '"') {
    char q = term[start];
    size_t end = term.find(q, start + 1);
    if (end != std::string::npos) {
      out_dir = expand_home(term.substr(start + 1, end - start - 1));
      out_query = term.substr(end + 1);
    }
  } else {
    size_t end = term.find(' ', start);
    out_dir = expand_home(term.substr(start, end - start));
    out_query = end == std::string::npos ? "" : term.substr(end);
  }

  out_query.erase(0, out_query.find_first_not_of(" "));
}

static std::string get_icon_for_file(const FileEntry &f) {
  if (f.is_dir) {
    return "folder-symbolic";
  }

  std::string ext = f.path.extension().string();
  if (ext.empty())
    return "text-x-generic";

  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  static const std::unordered_map<std::string, std::string> icon_map = {
      // Programming & Scripts
      {".c", "text-x-script"},
      {".cpp", "text-x-script"},
      {".h", "text-x-script"},
      {".hpp", "text-x-script"},
      {".py", "text-x-script"},
      {".js", "text-x-script"},
      {".ts", "text-x-script"},
      {".go", "text-x-script"},
      {".rs", "text-x-script"},
      {".java", "text-x-script"},
      {".sh", "text-x-script"},
      {".bash", "text-x-script"},
      {".php", "text-x-script"},
      {".rb", "text-x-script"},
      {".pl", "text-x-script"},
      {".lua", "text-x-script"},
      {".sql", "text-x-script"},
      {".html", "text-html"},
      {".css", "text-css"},
      {".json", "text-x-script"},
      {".xml", "text-xml"},

      // Documents
      {".pdf", "document-pdf"},
      {".doc", "x-office-document"},
      {".docx", "x-office-document"},
      {".odt", "x-office-document"},
      {".rtf", "x-office-document"},
      {".xls", "x-office-spreadsheet"},
      {".xlsx", "x-office-spreadsheet"},
      {".ods", "x-office-spreadsheet"},
      {".csv", "x-office-spreadsheet"},
      {".ppt", "x-office-presentation"},
      {".pptx", "x-office-presentation"},
      {".odp", "x-office-presentation"},
      {".txt", "text-x-generic"},
      {".md", "text-markdown"},
      {".tex", "text-x-tex"},

      // Archives
      {".zip", "package-x-generic"},
      {".tar", "package-x-generic"},
      {".gz", "package-x-generic"},
      {".bz2", "package-x-generic"},
      {".xz", "package-x-generic"},
      {".7z", "package-x-generic"},
      {".rar", "package-x-generic"},
      {".deb", "package-x-generic"},
      {".rpm", "package-x-generic"},
      {".iso", "drive-optical"},

      // Images
      {".png", "image-x-generic"},
      {".jpg", "image-x-generic"},
      {".jpeg", "image-x-generic"},
      {".gif", "image-x-generic"},
      {".svg", "image-x-generic"},
      {".bmp", "image-x-generic"},
      {".webp", "image-x-generic"},
      {".ico", "image-x-generic"},
      {".tiff", "image-x-generic"},

      // Audio
      {".mp3", "audio-x-generic"},
      {".wav", "audio-x-generic"},
      {".ogg", "audio-x-generic"},
      {".flac", "audio-x-generic"},
      {".m4a", "audio-x-generic"},
      {".aac", "audio-x-generic"},
      {".opus", "audio-x-generic"},

      // Video
      {".mp4", "video-x-generic"},
      {".mkv", "video-x-generic"},
      {".avi", "video-x-generic"},
      {".mov", "video-x-generic"},
      {".wmv", "video-x-generic"},
      {".webm", "video-x-generic"},
      {".flv", "video-x-generic"},

      // ohers
      {".bin", "application-x-executable"},
      {".appimage", "application-x-executable"},
      {".flatpak", "package-x-generic"},
      {".conf", "emblem-system"},
      {".cfg", "emblem-system"},
      {".ini", "emblem-system"},
      {".log", "text-x-generic-template"}};

  auto it = icon_map.find(ext);
  if (it != icon_map.end()) {
    return it->second;
  }

  return "text-x-generic"; // Final fallback
}

void plugin_init(const LawnchHostApi *host) {
  if (const char *v = host->get_config_value(host, "max_results")) {
    try {
      g_max_results = std::stoul(v);
    } catch (...) {
    }
  }

  if (const char *v = host->get_config_value(host, "max_depth")) {
    try {
      g_max_depth = std::stoi(v);
    } catch (...) {
    }
  }
}

void plugin_destroy(void) {
  std::lock_guard lock(g_index_map_mutex);
  g_indexes.clear();
}

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":file", ":f", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  LawnchResult *r = new LawnchResult;
  r->name = c_strdup(":file / :f");
  r->comment = c_strdup("Search files (:dir <path>)");
  r->icon = c_strdup("folder-symbolic");
  r->command = c_strdup("");
  r->type = c_strdup("help");
  r->preview_image_path = c_strdup("");
  return r;
}

LawnchResult *plugin_query(const char *term, int *num_results) {
  fs::path dir;
  std::string query;
  parse_query(term, dir, query);

  if (!fs::exists(dir)) {
    *num_results = 0;
    return nullptr;
  }

  DirIndex *index;
  {
    std::lock_guard lock(g_index_map_mutex);
    index = &g_indexes[dir.string()];
  }

  std::call_once(index->once, [&]() {
    std::thread(scan_dir, index, dir, g_max_depth).detach();
  });

  std::string q = to_lower(query);
  std::vector<LawnchResult> out;
  out.reserve(g_max_results);

  const char *editor = std::getenv("EDITOR") ?: "nano";
  const char *terminal = std::getenv("TERMINAL") ?: "xterm";

  {
    std::shared_lock lock(index->mutex);

    for (const auto &f : index->files) {
      if (!q.empty() && f.name_lower.find(q) == std::string::npos)
        continue;

      std::string cmd = f.is_dir ? std::string(terminal) + " -e " + editor +
                                       " \"" + f.path.string() + "\""
                                 : "xdg-open \"" + f.path.string() + "\"";

      LawnchResult r;
      r.name = c_strdup(f.name);
      r.comment = c_strdup(f.path.string());

      std::string icon_name = get_icon_for_file(f);
      r.icon = c_strdup(icon_name.c_str());

      r.command = c_strdup(cmd);
      r.type = c_strdup("plugin");

      if (icon_name == "image-x-generic") {
        r.preview_image_path = c_strdup(f.path.string());
      } else {
        r.preview_image_path = c_strdup("");
      }

      out.push_back(r);
      if (out.size() >= g_max_results)
        break;
    }
  }

  if (out.empty()) {
    *num_results = 0;
    return nullptr;
  }

  LawnchResult *arr = new LawnchResult[out.size()];
  std::copy(out.begin(), out.end(), arr);
  *num_results = out.size();
  return arr;
}

void plugin_free_results(LawnchResult *results, int n) {
  for (int i = 0; i < n; ++i) {
    delete[] results[i].name;
    delete[] results[i].comment;
    delete[] results[i].icon;
    delete[] results[i].command;
    delete[] results[i].type;
    delete[] results[i].preview_image_path;
  }
  delete[] results;
}

static LawnchPluginVTable g_vtable = {.plugin_api_version =
                                          LAWNCH_PLUGIN_API_VERSION,
                                      .init = plugin_init,
                                      .destroy = plugin_destroy,
                                      .get_triggers = plugin_get_triggers,
                                      .get_help = plugin_get_help,
                                      .query = plugin_query,
                                      .free_results = plugin_free_results};

PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void) { return &g_vtable; }
