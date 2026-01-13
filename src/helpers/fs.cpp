#include "fs.hpp"
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>

namespace Lawnch::Fs {

std::filesystem::path get_home_path() {
  const char *home = std::getenv("HOME");
  if (home) {
    return std::filesystem::path(home);
  }

  struct passwd *pwd = getpwuid(getuid());
  if (pwd) {
    return std::filesystem::path(pwd->pw_dir);
  }

  return std::filesystem::path("/");
}

std::filesystem::path expand_tilde(const std::string &path_str) {
  if (path_str.empty())
    return "";

  if (path_str[0] == '~') {
    std::filesystem::path home = get_home_path();
    if (path_str.length() == 1)
      return home;
    if (path_str[1] == '/') {
      return home / path_str.substr(2);
    }
  }
  return std::filesystem::path(path_str);
}

std::filesystem::path get_config_home() {
  const char *xdg = std::getenv("XDG_CONFIG_HOME");
  if (xdg && *xdg)
    return std::filesystem::path(xdg);
  return get_home_path() / ".config";
}

std::filesystem::path get_data_home() {
  const char *xdg = std::getenv("XDG_DATA_HOME");
  if (xdg && *xdg)
    return std::filesystem::path(xdg);
  return get_home_path() / ".local" / "share";
}

std::filesystem::path get_cache_home() {
  const char *xdg = std::getenv("XDG_CACHE_HOME");
  if (xdg && *xdg)
    return std::filesystem::path(xdg);
  return get_home_path() / ".cache";
}

std::filesystem::path get_log_path(const std::string &app_name) {
  auto path = get_cache_home() / app_name;
  std::filesystem::create_directories(path);
  return path / (app_name + ".log");
}

std::filesystem::path get_socket_path(const std::string &filename) {
  const char *xdg = std::getenv("XDG_RUNTIME_DIR");
  std::filesystem::path base = (xdg && *xdg) ? xdg : "/tmp";
  return base / filename;
}

std::vector<std::string> get_data_dirs() {
  std::vector<std::string> dirs;
  const char *xdg_data = std::getenv("XDG_DATA_DIRS");

  std::string xdg_str = xdg_data ? xdg_data : "/usr/local/share:/usr/share";

  std::stringstream ss(xdg_str);
  std::string item;
  while (std::getline(ss, item, ':')) {
    if (!item.empty())
      dirs.push_back(item);
  }

  dirs.push_back((Fs::get_home_path() / ".local/share").string());

  return dirs;
}

std::vector<std::string> get_icon_dirs() {
  std::vector<std::string> icon_dirs;

  icon_dirs.push_back((Fs::get_home_path() / ".icons").string());
  auto data_dirs = get_data_dirs(); // This includes .local/share

  const char *home = std::getenv("HOME");
  if (home) {
    icon_dirs.push_back(std::string(home) + "/.icons");
    icon_dirs.push_back(std::string(home) + "/.local/share/icons");
  }

  const char *xdg_data = std::getenv("XDG_DATA_DIRS");
  if (xdg_data) {
    std::stringstream ss(xdg_data);
    std::string item;
    while (std::getline(ss, item, ':')) {
      if (!item.empty())
        icon_dirs.push_back(item + "/icons");
    }
  } else {
    icon_dirs.push_back("/usr/local/share/icons");
    icon_dirs.push_back("/usr/share/icons");
  }

  icon_dirs.push_back("/var/lib/flatpak/exports/share/icons");

  return icon_dirs;
}

} // namespace Lawnch::Fs
