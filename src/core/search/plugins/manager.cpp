#include "manager.hpp"
#include "../../../helpers/fs.hpp"
#include "../../../helpers/logger.hpp"
#include "adapter.hpp"
#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace Lawnch::Core::Search::Plugins {

// Context for host API
struct PluginApiContext {
  std::string plugin_name;
  const Config::Config &config;
  Manager *manager;
};

// C callback wrappers
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

Manager::Manager(const Config::Config &config) : m_config(config) {
  find_plugin_dirs();
  find_data_dir();
}

Manager::~Manager() {
  m_trigger_map.clear();
  m_plugins.clear();
  m_api_contexts.clear();

  for (void *handle : m_handles) {
    if (handle)
      dlclose(handle);
  }
}

const std::vector<std::unique_ptr<SearchMode>> &Manager::get_plugins() const {
  const_cast<Manager *>(this)->ensure_plugins_loaded();
  return m_plugins;
}

const std::string &Manager::get_data_dir() const { return m_data_dir; }

void Manager::find_data_dir() {

#ifdef LAWNCH_DATA_DIR
  if (fs::is_directory(fs::path(LAWNCH_DATA_DIR) / "assets")) {
    m_data_dir = LAWNCH_DATA_DIR;
    return;
  }
#endif

  const char *flatpak_dir = std::getenv("XDG_DATA_HOME");
  std::filesystem::path xdg_data_home = Lawnch::Fs::get_data_home();
  if (!xdg_data_home.empty()) {
    fs::path user_data = fs::path(xdg_data_home) / "lawnch";
    if (fs::is_directory(user_data / "assets")) {
      m_data_dir = user_data.string();
      return;
    }
  }

  if (flatpak_dir) {
    fs::path flatpak_path = fs::path(flatpak_dir).parent_path() / "app" /
                            "com.github.hoppxi.lawnch" / "share" / "lawnch";
    if (fs::is_directory(flatpak_path)) {
      m_data_dir = flatpak_path.string();
      return;
    }
  }

  fs::path exe_path = fs::canonical("/proc/self/exe");
  fs::path dev_path1 = exe_path.parent_path().parent_path() / "assets";
  if (fs::is_directory(dev_path1)) {
    m_data_dir = dev_path1.parent_path().string();
    return;
  }
  fs::path dev_path2 =
      exe_path.parent_path().parent_path().parent_path() / "assets";
  if (fs::is_directory(dev_path2)) {
    m_data_dir = dev_path2.parent_path().string();
    return;
  }

  if (fs::is_directory("/usr/share/lawnch/assets")) {
    m_data_dir = "/usr/share/lawnch";
    return;
  }
  if (fs::is_directory("/usr/local/share/lawnch/assets")) {
    m_data_dir = "/usr/local/share/lawnch";
    return;
  }

  if (fs::is_directory("./assets")) {
    m_data_dir = ".";
    return;
  }

  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::WARNING,
                      "Could not determine data directory.");
}

void Manager::find_plugin_dirs() {
  const char *env_path = std::getenv("LAWNCH_PLUGIN_PATH");
  if (env_path) {
    m_plugin_dirs.push_back(env_path);
  }

  if (fs::exists("build/plugins")) {
    m_plugin_dirs.push_back("build/plugins");
  }

  std::filesystem::path xdg_data_home = Lawnch::Fs::get_data_home();
  if (!xdg_data_home.empty()) {
    fs::path user_plugins = fs::path(xdg_data_home) / "lawnch" / "plugins";
    if (fs::is_directory(user_plugins)) {
      m_plugin_dirs.push_back(user_plugins.string());
    }
  }

  fs::path system_plugins1 = "/usr/lib/lawnch/plugins";
  if (fs::is_directory(system_plugins1)) {
    m_plugin_dirs.push_back(system_plugins1.string());
  }

  fs::path system_plugins2 = "/usr/local/lib/lawnch/plugins";
  if (fs::is_directory(system_plugins2)) {
    m_plugin_dirs.push_back(system_plugins2.string());
  }
}

void Manager::ensure_plugins_loaded() {
  if (!plugins_loaded) {
    load_plugins();
  }
}

void Manager::load_plugins() {
  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                      "Loading plugins...");
  for (const auto &plugin_name : m_config.enabled_plugins) {
    load_plugin(plugin_name);
  }
  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                      "All plugins loaded.");
  plugins_loaded = true;
}

SearchMode *Manager::find_plugin(const std::string &trigger) {
  ensure_plugins_loaded();
  auto it = m_trigger_map.find(trigger);
  if (it != m_trigger_map.end()) {
    return it->second;
  }
  return nullptr;
}

void Manager::load_plugin(const std::string &name) {
  std::stringstream ss;
  ss << "Loading plugin '" << name << "'";
  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                      ss.str());

  void *handle = nullptr;
  std::string found_path;

  for (const auto &dir : m_plugin_dirs) {
    std::string path = fs::path(dir) / (name + ".so");
    if (fs::exists(path)) {
      handle = dlopen(path.c_str(), RTLD_LAZY);
      if (handle) {
        found_path = path;
        break;
      } else {
        std::stringstream err_ss;
        err_ss << "dlopen failed for '" << path << "': " << dlerror();
        Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                            err_ss.str());
      }
    }
  }

  if (!handle) {
    std::stringstream err_ss;
    err_ss << "Cannot find or load plugin " << name << ".so";
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                        err_ss.str());
    return;
  }

  using entry_func = LawnchPluginVTable *(*)();
  entry_func entry = (entry_func)dlsym(handle, "lawnch_plugin_entry");
  if (!entry) {
    std::stringstream err_ss;
    err_ss << "Cannot find symbol 'lawnch_plugin_entry' in " << found_path
           << ": " << dlerror();
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                        err_ss.str());
    dlclose(handle);
    return;
  }

  LawnchPluginVTable *vtable = entry();
  if (!vtable) {
    std::stringstream err_ss;
    err_ss << "'lawnch_plugin_entry' in " << found_path << " returned null.";
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                        err_ss.str());
    dlclose(handle);
    return;
  }

  auto adapter = std::make_unique<Adapter>(vtable);

  auto context = std::make_unique<PluginApiContext>(
      PluginApiContext{name, m_config, this});
  LawnchHostApi host_api = {
      .host_api_version = LAWNCH_PLUGIN_API_VERSION,
      .userdata = context.get(),
      .get_config_value = &s_get_config_value,
      .get_data_dir = &s_get_data_dir,
  };

  adapter->init_with_api(&host_api);

  for (const auto &trigger : adapter->get_triggers()) {
    m_trigger_map[trigger] = adapter.get();
  }

  m_handles.push_back(handle);
  m_api_contexts.push_back(std::move(context));
  m_plugins.push_back(std::move(adapter));

  std::stringstream info_ss;
  info_ss << "Loaded plugin: " << name << " from " << found_path;
  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                      info_ss.str());
}

} // namespace Lawnch::Core::Search::Plugins
