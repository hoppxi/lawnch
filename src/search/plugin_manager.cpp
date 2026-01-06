#include "plugin_manager.hpp"
#include "../utils.hpp"
#include "plugin_adapter.hpp"
#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

// Context for the host API callbacks.
// Each loaded plugin gets one of these.
struct PluginApiContext {
  std::string plugin_name;
  Config &config;
  PluginManager *manager;
};

// C-style callback function to get a config value
static const char *s_get_config_value(const LawnchHostApi *host,
                                      const char *key) {
  if (!host || !host->userdata || !key) {
    return nullptr;
  }
  auto *context = static_cast<PluginApiContext *>(host->userdata);

  std::string full_key = context->plugin_name + "." + key;

  auto &plugin_configs_map = context->config.plugin_configs;
  auto it = plugin_configs_map.find(full_key);
  if (it != plugin_configs_map.end()) {
    return it->second.c_str();
  }

  return nullptr;
}

static const char *s_get_data_dir(const LawnchHostApi *host) {
  if (!host || !host->userdata) {
    return nullptr;
  }
  auto *context = static_cast<PluginApiContext *>(host->userdata);
  return context->manager->get_data_dir().c_str();
}

PluginManager::PluginManager(Config &config) : config(config) {
  find_plugin_dirs();
  find_data_dir();
}

PluginManager::~PluginManager() {
  trigger_map.clear();
  plugins.clear();
  api_contexts.clear(); // Frees the context objects
  for (void *handle : handles) {
    dlclose(handle);
  }
}

const std::vector<std::unique_ptr<SearchMode>> &
PluginManager::get_plugins() const {
  return plugins;
}

const std::string &PluginManager::get_data_dir() const { return data_dir; }

void PluginManager::find_data_dir() {

#ifdef LAWNCH_DATA_DIR
  if (fs::is_directory(fs::path(LAWNCH_DATA_DIR) / "assets")) {
    data_dir = LAWNCH_DATA_DIR;
    return;
  }
#endif

  const char *flatpak_dir = std::getenv("XDG_DATA_HOME");
  std::string xdg_data_home = Utils::get_xdg_data_home();
  if (!xdg_data_home.empty()) {
    fs::path user_data = fs::path(xdg_data_home) / "lawnch";
    if (fs::is_directory(user_data / "assets")) {
      data_dir = user_data.string();
      return;
    }
  }

  // either XDG_DATA_HOME or LAWNCH_DATA_DIR should be enough
  // Check for a flatpak install
  if (flatpak_dir) {
    fs::path flatpak_path = fs::path(flatpak_dir).parent_path() / "app" /
                            "com.github.hoppxi.lawnch" / "share" / "lawnch";
    if (fs::is_directory(flatpak_path)) {
      data_dir = flatpak_path.string();
      return;
    }
  }

  // Check relative to executable (for development)
  fs::path exe_path = fs::canonical("/proc/self/exe");
  fs::path dev_path1 = exe_path.parent_path().parent_path() / "assets";
  if (fs::is_directory(dev_path1)) {
    data_dir = dev_path1.parent_path().string();
    return;
  }
  fs::path dev_path2 =
      exe_path.parent_path().parent_path().parent_path() / "assets";
  if (fs::is_directory(dev_path2)) {
    data_dir = dev_path2.parent_path().string();
    return;
  }

  // Check standard system install paths
  if (fs::is_directory("/usr/share/lawnch/assets")) {
    data_dir = "/usr/share/lawnch";
    return;
  }
  if (fs::is_directory("/usr/local/share/lawnch/assets")) {
    data_dir = "/usr/local/share/lawnch";
    return;
  }

  // Fallback to a relative path for local development builds
  if (fs::is_directory("./assets")) {
    data_dir = ".";
    return;
  }

  Utils::log("PluginManager", Utils::LogLevel::WARNING,
             "Could not determine data directory.");
}

void PluginManager::find_plugin_dirs() {
  const char *env_path = std::getenv("LAWNCH_PLUGIN_PATH");
  if (env_path) {
    plugin_dirs.push_back(env_path);
  }

  if (fs::exists("build/plugins")) {
    plugin_dirs.push_back("build/plugins");
  }

  std::string xdg_data_home = Utils::get_xdg_data_home();
  if (!xdg_data_home.empty()) {
    fs::path user_plugins = fs::path(xdg_data_home) / "lawnch" / "plugins";
    if (fs::is_directory(user_plugins)) {
      plugin_dirs.push_back(user_plugins.string());
    }
  }

  fs::path system_plugins1 = "/usr/lib/lawnch/plugins";
  if (fs::is_directory(system_plugins1)) {
    plugin_dirs.push_back(system_plugins1.string());
  }

  fs::path system_plugins2 = "/usr/local/lib/lawnch/plugins";
  if (fs::is_directory(system_plugins2)) {
    plugin_dirs.push_back(system_plugins2.string());
  }
}

void PluginManager::load_plugins() {
  Utils::log("PluginManager", Utils::LogLevel::INFO, "Loading plugins...");
  for (const auto &plugin_name : config.enabled_plugins) {
    load_plugin(plugin_name);
  }
  Utils::log("PluginManager", Utils::LogLevel::INFO, "All plugins loaded.");
}

SearchMode *PluginManager::find_plugin(const std::string &trigger) {
  auto it = trigger_map.find(trigger);
  if (it != trigger_map.end()) {
    return it->second;
  }
  return nullptr;
}

void PluginManager::load_plugin(const std::string &name) {
  std::stringstream ss;
  ss << "Loading plugin '" << name << "'";
  Utils::log("PluginManager", Utils::LogLevel::INFO, ss.str());

  void *handle = nullptr;
  std::string found_path;

  for (const auto &dir : plugin_dirs) {
    std::string path = fs::path(dir) / (name + ".so");
    if (fs::exists(path)) {
      handle = dlopen(path.c_str(), RTLD_LAZY);
      if (handle) {
        found_path = path;
        break;
      } else {
        std::stringstream err_ss;
        err_ss << "dlopen failed for '" << path << "': " << dlerror();
        Utils::log("PluginManager", Utils::LogLevel::ERROR, err_ss.str());
      }
    }
  }

  if (!handle) {
    std::stringstream err_ss;
    err_ss << "Cannot find or load plugin " << name << ".so";
    Utils::log("PluginManager", Utils::LogLevel::ERROR, err_ss.str());
    return;
  }

  using entry_func = LawnchPluginVTable *(*)();
  entry_func entry = (entry_func)dlsym(handle, "lawnch_plugin_entry");
  if (!entry) {
    std::stringstream err_ss;
    err_ss << "Cannot find symbol 'lawnch_plugin_entry' in " << found_path
           << ": " << dlerror();
    Utils::log("PluginManager", Utils::LogLevel::ERROR, err_ss.str());
    dlclose(handle);
    return;
  }

  LawnchPluginVTable *vtable = entry();
  if (!vtable) {
    std::stringstream err_ss;
    err_ss << "'lawnch_plugin_entry' in " << found_path << " returned null.";
    Utils::log("PluginManager", Utils::LogLevel::ERROR, err_ss.str());
    dlclose(handle);
    return;
  }

  auto adapter = std::make_unique<PluginAdapter>(vtable);

  auto context =
      std::make_unique<PluginApiContext>(PluginApiContext{name, config, this});
  LawnchHostApi host_api = {
      .host_api_version = LAWNCH_PLUGIN_API_VERSION,
      .userdata = context.get(),
      .get_config_value = &s_get_config_value,
      .get_data_dir = &s_get_data_dir,
  };

  adapter->init_with_api(&host_api);

  for (const auto &trigger : adapter->get_triggers()) {
    trigger_map[trigger] = adapter.get();
  }

  handles.push_back(handle);
  api_contexts.push_back(std::move(context));
  plugins.push_back(std::move(adapter));

  std::stringstream info_ss;
  info_ss << "Loaded plugin: " << name << " from " << found_path;
  Utils::log("PluginManager", Utils::LogLevel::INFO, info_ss.str());
}