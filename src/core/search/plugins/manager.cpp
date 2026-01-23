#include "manager.hpp"
#include "../../../helpers/fs.hpp"
#include "../../../helpers/logger.hpp"
#include "../../../helpers/string.hpp"
#include "adapter.hpp"
#include <algorithm>
#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
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
  static thread_local std::string data_dir_cache;
  data_dir_cache = context->manager->get_plugin_data_dir(context->plugin_name);
  return data_dir_cache.c_str();
}

Manager::Manager(const Config::Config &config) : m_config(config) {
  find_plugin_dirs();
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

std::string Manager::get_plugin_data_dir(const std::string &plugin_name) const {
  auto it = m_plugin_data_dirs.find(plugin_name);
  if (it != m_plugin_data_dirs.end()) {
    return it->second;
  }
  std::string data_dir = find_plugin_data_dir(plugin_name);
  m_plugin_data_dirs[plugin_name] = data_dir;
  return data_dir;
}

std::string
Manager::find_plugin_data_dir(const std::string &plugin_name) const {
  for (const auto &dir : m_plugin_dirs) {
    fs::path assets_path = fs::path(dir) / plugin_name / "assets";
    if (fs::is_directory(assets_path)) {
      return assets_path.string();
    }
  }
  return "";
}

void Manager::find_plugin_dirs() {
  const char *env_path = std::getenv("LAWNCH_PLUGIN_PATH");
  if (env_path) {
    m_plugin_dirs.push_back(env_path);
  }

  fs::path user_plugins = Lawnch::Fs::get_data_home() / "lawnch" / "plugins";
  m_plugin_dirs.push_back(user_plugins.string());

  for (const auto &data_dir : Lawnch::Fs::get_data_dirs()) {
    fs::path system_plugins = fs::path(data_dir) / "lawnch" / "plugins";
    m_plugin_dirs.push_back(system_plugins.string());
  }
}

void Manager::ensure_plugins_loaded() {
  if (!plugins_loaded) {
    load_plugins();
  }
}

void Manager::load_plugins() {
  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                      "Initializing plugins...");

  fs::path cache_dir = Lawnch::Fs::get_cache_home() / "lawnch";
  if (!fs::exists(cache_dir)) {
    fs::create_directories(cache_dir);
  }
  fs::path cache_file = cache_dir / "plugin-triggers.cache";

  if (fs::exists(cache_file)) {
    load_triggers_cache();
    if (!m_lazy_triggers.empty()) {
      Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                          "Loaded plugin triggers from cache.");
      plugins_loaded = true;
      return;
    }
  }

  generate_triggers_cache();
  plugins_loaded = true;
}

void Manager::generate_triggers_cache() {
  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                      "Generating plugin triggers cache...");
  for (const auto &plugin_name : m_config.enabled_plugins) {
    load_plugin(plugin_name);
  }
  save_triggers_cache();
}

void Manager::save_triggers_cache() {
  fs::path cache_dir = fs::path(Lawnch::Fs::get_cache_home()) / "lawnch";
  if (!fs::exists(cache_dir)) {
    fs::create_directories(cache_dir);
  }
  fs::path cache_file = cache_dir / "plugin-triggers.cache";

  std::ofstream file(cache_file);
  if (!file.is_open()) {
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                        "Failed to open cache file for writing.");
    return;
  }

  if (m_plugins.size() != m_api_contexts.size()) {
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                        "Plugin/Context mismatch, cannot save cache.");
    return;
  }

  std::vector<std::string> sorted_plugins = m_config.enabled_plugins;
  std::sort(sorted_plugins.begin(), sorted_plugins.end());
  file << "CONFIG_SIGNATURE:";
  for (const auto &p : sorted_plugins) {
    file << p << ",";
  }
  file << "\n";

  for (size_t i = 0; i < m_plugins.size(); ++i) {
    std::string name = m_api_contexts[i]->plugin_name;
    auto triggers = m_plugins[i]->get_triggers();
    auto help = m_plugins[i]->get_help();

    file << "PLUGIN:" << name << "\n";
    for (const auto &t : triggers) {
      file << "TRIGGER:" << t << "\n";
    }
    file << "HELP_NAME:" << help.name << "\n";
    file << "HELP_COMMENT:" << help.comment << "\n";
    file << "HELP_ICON:" << help.icon << "\n";
    file << "HELP_COMMAND:" << help.command << "\n";
    file << "HELP_TYPE:" << help.type << "\n";
    file << "HELP_PREVIEW_IMAGE_PATH:" << help.preview_image_path << "\n";
    file << "END_PLUGIN\n";
  }
}

void Manager::load_triggers_cache() {
  fs::path cache_file = fs::path(Lawnch::Fs::get_cache_home()) / "lawnch" /
                        "plugin-triggers.cache";
  std::ifstream file(cache_file);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  std::string current_plugin;
  std::vector<std::string> current_triggers;
  SearchResult current_help;

  if (std::getline(file, line)) {
    if (line.rfind("CONFIG_SIGNATURE:", 0) == 0) {
      std::string cached_sig = line.substr(17);

      std::vector<std::string> sorted_plugins = m_config.enabled_plugins;
      std::sort(sorted_plugins.begin(), sorted_plugins.end());
      std::stringstream current_sig_ss;
      for (const auto &p : sorted_plugins) {
        current_sig_ss << p << ",";
      }

      if (cached_sig != current_sig_ss.str()) {
        Lawnch::Logger::log(
            "PluginManager", Lawnch::Logger::LogLevel::INFO,
            "Plugin configuration changed, invalidating cache.");
        return;
      }
    } else {
      Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                          "Old cache format detected, invalidating cache.");
      return;
    }
  } else {
    return;
  }

  while (std::getline(file, line)) {
    if (line.rfind("PLUGIN:", 0) == 0) {
      current_plugin = line.substr(7);
      current_triggers.clear();
      current_help = SearchResult{};
    } else if (line.rfind("TRIGGER:", 0) == 0) {
      current_triggers.push_back(line.substr(8));
    } else if (line.rfind("HELP_NAME:", 0) == 0) {
      current_help.name = line.substr(10);
    } else if (line.rfind("HELP_COMMENT:", 0) == 0) {
      current_help.comment = line.substr(13);
    } else if (line.rfind("HELP_ICON:", 0) == 0) {
      current_help.icon = line.substr(10);
    } else if (line.rfind("HELP_COMMAND:", 0) == 0) {
      current_help.command = line.substr(13);
    } else if (line.rfind("HELP_TYPE:", 0) == 0) {
      current_help.type = line.substr(10);
    } else if (line.rfind("HELP_PREVIEW_IMAGE_PATH:", 0) == 0) {
      current_help.preview_image_path = line.substr(24);
    } else if (line == "END_PLUGIN") {
      if (!current_plugin.empty()) {
        for (const auto &t : current_triggers) {
          m_lazy_triggers[t] = current_plugin;
        }
        if (current_help.name.empty() && !current_triggers.empty()) {
          current_help.name = current_triggers[0];
        }
        m_cached_help.push_back(current_help);
      }
    }
  }
}

void Manager::ensure_plugin_for_trigger(const std::string &query) {
  if (!plugins_loaded)
    ensure_plugins_loaded();

  auto it = m_lazy_triggers.find(query);
  if (it != m_lazy_triggers.end()) {
    if (m_trigger_map.find(query) == m_trigger_map.end()) {
      load_plugin(it->second);
      if (m_trigger_map.find(query) == m_trigger_map.end()) {
        Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::WARNING,
                            "Lazy loaded plugin '" + it->second +
                                "' for trigger '" + query +
                                "' but it did not register that trigger!");
      }
    }
    return;
  }

  for (const auto &[trigger, plugin_name] : m_lazy_triggers) {
    if (query.rfind(trigger + " ", 0) == 0) {
      if (m_trigger_map.find(trigger) == m_trigger_map.end()) {
        load_plugin(plugin_name);
      }
      return;
    }
  }
}

SearchMode *Manager::find_plugin(const std::string &trigger) {
  ensure_plugin_for_trigger(trigger);
  auto it = m_trigger_map.find(trigger);
  if (it != m_trigger_map.end()) {
    return it->second;
  }
  return nullptr;
}

void Manager::load_plugin(const std::string &name) {
  for (const auto &ctx : m_api_contexts) {
    if (ctx->plugin_name == name)
      return;
  }

  if (m_plugin_dirs.empty()) {
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::WARNING,
                        "Attempting to load plugin '" + name +
                            "' but no plugin directories are configured!");
  }

  std::stringstream ss;
  ss << "Loading plugin '" << name << "'";
  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                      ss.str());

  void *handle = nullptr;
  std::string found_path;

  for (const auto &dir : m_plugin_dirs) {
    std::string path = (fs::path(dir) / name / (name + ".so")).string();
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

const std::vector<SearchResult> &Manager::get_all_help() const {
  if (m_cached_help.empty() && plugins_loaded) {
    auto &mutable_this = const_cast<Manager &>(*this);
    for (const auto &plugin : m_plugins) {
      mutable_this.m_cached_help.push_back(plugin->get_help());
    }
  }
  return m_cached_help;
}

} // namespace Lawnch::Core::Search::Plugins
