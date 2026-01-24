#include "PluginBase.hpp"

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

namespace {

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

bool is_hidden(const fs::path &p) {
  auto name = p.filename().string();
  return !name.empty() && name[0] == '.';
}

void scan_dir(DirIndex *index, fs::path root, int max_depth) {
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
        std::string lower = fe.name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        fe.name_lower = lower;
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

std::string get_icon_for_file(const FileEntry &f) {
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

} // namespace

class FilesPlugin : public lawnch::Plugin {
private:
  std::unordered_map<std::string, DirIndex> indexes_;
  std::mutex index_map_mutex_;
  size_t max_results_ = 50;
  int max_depth_ = 4;

  std::string to_lower(const std::string &s) {
    if (host_ && host_->str_api && host_->str_api->to_lower_copy) {
      char *res = host_->str_api->to_lower_copy(s.c_str());
      if (res) {
        std::string ret(res);
        if (host_->str_api->free_str)
          host_->str_api->free_str(res);
        else
          free(res);
        return ret;
      }
    }
    // Fallback
    std::string ret = s;
    std::transform(ret.begin(), ret.end(), ret.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ret;
  }

  std::string expand_home(const std::string &p) {
    if (host_ && host_->fs_api && host_->fs_api->expand_tilde) {
      char *res = host_->fs_api->expand_tilde(p.c_str());
      if (res) {
        std::string s(res);
        if (host_->fs_api->free_path)
          host_->fs_api->free_path(res);
        else
          free(res);
        return s;
      }
    }
    // Fallback
    if (!p.empty() && p[0] == '~') {
      const char *home = std::getenv("HOME");
      if (home)
        return std::string(home) + p.substr(1);
    }
    return p;
  }

  void parse_query(const std::string &term, fs::path &out_dir,
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

  std::vector<lawnch::Result> do_file_query(const std::string &term) {
    fs::path dir;
    std::string query_str;
    parse_query(term, dir, query_str);

    if (!fs::exists(dir)) {
      return {};
    }

    DirIndex *index;
    {
      std::lock_guard lock(index_map_mutex_);
      index = &indexes_[dir.string()];
    }

    std::call_once(index->once, [&]() {
      std::thread(scan_dir, index, dir, max_depth_).detach();
    });

    std::string q = to_lower(query_str);
    std::vector<lawnch::Result> out;
    out.reserve(max_results_);

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

        lawnch::Result r;
        r.name = f.name;
        r.comment = f.path.string();

        std::string icon_name = get_icon_for_file(f);
        r.icon = icon_name;

        r.command = cmd;
        r.type = "plugin";

        if (icon_name == "image-x-generic") {
          r.preview_image_path = f.path.string();
        }

        out.push_back(r);
        if (out.size() >= max_results_)
          break;
      }
    }

    return out;
  }

public:
  void init(const LawnchHostApi *host) override {
    Plugin::init(host);
    if (const char *v = host->get_config_value(host, "max_results")) {
      try {
        max_results_ = std::stoul(v);
      } catch (...) {
      }
    }
    if (const char *v = host->get_config_value(host, "max_depth")) {
      try {
        max_depth_ = std::stoi(v);
      } catch (...) {
      }
    }
  }

  void destroy() override {
    std::lock_guard lock(index_map_mutex_);
    for (auto &[path, idx] : indexes_) {
      idx.stop = true;
    }
    indexes_.clear();
  }

  std::vector<std::string> get_triggers() override { return {":file", ":f"}; }

  lawnch::Result get_help() override {
    lawnch::Result r;
    r.name = ":file / :f";
    r.comment = "Search files (:dir <path>)";
    r.icon = "folder-symbolic";
    r.type = "help";
    return r;
  }

  std::vector<lawnch::Result> query(const std::string &term) override {
    return do_file_query(term);
  }
};

LAWNCH_PLUGIN_DEFINE(FilesPlugin)
