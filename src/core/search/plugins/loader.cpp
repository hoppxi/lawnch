#include "loader.hpp"
#include "../../../helpers/fs.hpp"
#include "../../../helpers/logger.hpp"
#include <dlfcn.h>
#include <sstream>

namespace fs = std::filesystem;

namespace Lawnch::Core::Search::Plugins {

std::vector<std::string> find_plugin_dirs() {
  std::vector<std::string> dirs;

  const char *env_path = std::getenv("LAWNCH_PLUGIN_PATH");
  if (env_path) {
    dirs.push_back(env_path);
  }

  const char *env_dir = std::getenv("LAWNCH_PLUGINS_DIR");
  if (env_dir) {
    dirs.push_back(env_dir);
  }

  fs::path user_plugins = Lawnch::Fs::get_data_home() / "lawnch" / "plugins";
  dirs.push_back(user_plugins.string());

  for (const auto &data_dir : Lawnch::Fs::get_data_dirs()) {
    fs::path system_plugins = fs::path(data_dir) / "lawnch" / "plugins";
    dirs.push_back(system_plugins.string());
  }

  return dirs;
}

std::string find_plugin_data_dir(const std::string &plugin_name,
                                 const std::vector<std::string> &plugin_dirs) {
  for (const auto &dir : plugin_dirs) {
    fs::path assets_path = fs::path(dir) / plugin_name / "assets";
    if (fs::is_directory(assets_path)) {
      return assets_path.string();
    }
  }
  return "";
}

LoadedPlugin load_plugin_vtable(const std::string &name,
                                const std::vector<std::string> &plugin_dirs) {
  LoadedPlugin result;

  for (const auto &dir : plugin_dirs) {
    std::string path = (fs::path(dir) / name / (name + ".so")).string();
    if (fs::exists(path)) {
      result.handle = dlopen(path.c_str(), RTLD_LAZY);
      if (result.handle) {
        result.found_path = path;
        break;
      } else {
        std::stringstream err_ss;
        err_ss << "dlopen failed for '" << path << "': " << dlerror();
        Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                            err_ss.str());
      }
    }
  }

  if (!result.handle) {
    std::stringstream err_ss;
    err_ss << "Cannot find or load plugin " << name << ".so";
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                        err_ss.str());
    return result;
  }

  using entry_func = LawnchPluginVTable *(*)();
  entry_func entry = (entry_func)dlsym(result.handle, "lawnch_plugin_entry");
  if (!entry) {
    std::stringstream err_ss;
    err_ss << "Cannot find symbol 'lawnch_plugin_entry' in "
           << result.found_path << ": " << dlerror();
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                        err_ss.str());
    dlclose(result.handle);
    result.handle = nullptr;
    return result;
  }

  result.vtable = entry();
  if (!result.vtable) {
    std::stringstream err_ss;
    err_ss << "'lawnch_plugin_entry' in " << result.found_path
           << " returned null.";
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                        err_ss.str());
    dlclose(result.handle);
    result.handle = nullptr;
    return result;
  }

  return result;
}

} // namespace Lawnch::Core::Search::Plugins
