#include "PluginBase.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

bool is_image_ext(const std::string &ext) {
  std::string e = ext;
  std::transform(e.begin(), e.end(), e.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".webp" ||
         e == ".bmp";
}

std::string build_command(const std::string &tpl, const std::string &path) {
  std::string cmd = tpl;
  size_t pos = cmd.find("{}");
  if (pos != std::string::npos) {
    cmd.replace(pos, 2, path);
  }
  return cmd;
}

} // namespace

class WallpapersPlugin : public lawnch::Plugin {
private:
  std::string wallpaper_dir_;
  std::string command_template_;
  size_t max_results_ = 50;

  std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
  }

  std::string expand_home(const std::string &path) {
    if (!path.empty() && path[0] == '~') {
      const char *home = std::getenv("HOME");
      if (home)
        return std::string(home) + path.substr(1);
    }
    return path;
  }

public:
  void init(const LawnchHostApi *host) override {
    Plugin::init(host);

    const char *dir = host->get_config_value(host, "dir");
    if (dir && *dir) {
      wallpaper_dir_ = expand_home(dir);
    } else {
      wallpaper_dir_ = expand_home("~/Pictures/Wallpapers");
    }

    const char *max_res = host->get_config_value(host, "max_results");
    if (max_res) {
      try {
        max_results_ = std::stoul(max_res);
      } catch (...) {
        max_results_ = 50;
      }
    }

    // Command template
    const char *cmd = host->get_config_value(host, "command");
    if (cmd && *cmd) {
      command_template_ = cmd;
    } else {
      command_template_ = "";
    }
  }

  std::vector<std::string> get_triggers() override { return {":wall", ":wp"}; }

  lawnch::Result get_help() override {
    lawnch::Result r;
    r.name = ":wall / :wp";
    r.comment = "Set wallpaper";
    r.icon = "preferences-desktop-wallpaper";
    r.type = "help";
    return r;
  }

  std::vector<lawnch::Result> query(const std::string &term) override {
    if (!fs::exists(wallpaper_dir_) || !fs::is_directory(wallpaper_dir_)) {
      return {};
    }

    std::string search = to_lower(term);
    std::vector<lawnch::Result> results;
    results.reserve(max_results_);

    fs::recursive_directory_iterator it(
        wallpaper_dir_, fs::directory_options::skip_permission_denied);

    for (const auto &entry : it) {
      if (results.size() >= max_results_)
        break;

      if (!entry.is_regular_file())
        continue;

      const fs::path &p = entry.path();
      if (!is_image_ext(p.extension().string()))
        continue;

      std::string fname = to_lower(p.filename().string());
      if (!search.empty() && fname.find(search) == std::string::npos)
        continue;

      std::string abs_path = p.string();

      lawnch::Result r;
      r.name = p.filename().string();
      r.comment = p.parent_path().string();
      r.icon = "preferences-desktop-wallpaper";
      r.command = build_command(command_template_, abs_path);
      r.type = "wallpaper-plugin";
      r.preview_image_path = (p.parent_path() / p.filename()).string();

      results.push_back(r);
    }
    return results;
  }
};

LAWNCH_PLUGIN_DEFINE(WallpapersPlugin)
