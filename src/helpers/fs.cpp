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

std::filesystem::path get_cache_dir() {
  const char *xdg = std::getenv("XDG_CACHE_HOME");
  if (xdg && *xdg)
    return std::filesystem::path(xdg);
  return get_home_path() / ".cache";
}

std::filesystem::path get_log_path(const std::string &app_name) {
  auto path = get_cache_dir() / app_name;
  std::filesystem::create_directories(path);
  return path / (app_name + ".log");
}

std::filesystem::path get_socket_path(const std::string &filename) {
  const char *xdg = std::getenv("XDG_RUNTIME_DIR");
  std::filesystem::path base = (xdg && *xdg) ? xdg : "/tmp";
  return base / filename;
}

} // namespace Lawnch::Fs
