#include "manager.hpp"
#include "../../../helpers/fs.hpp"
#include "../../../helpers/logger.hpp"
#include "adapter.hpp"
#include "host_api.hpp"
#include "loader.hpp"
#include <algorithm>
#include <dlfcn.h>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace Lawnch::Core::Search::Plugins {

Manager::Manager(const Config::Config &config) : m_config(config) {
  m_plugin_dirs = find_plugin_dirs();
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
  std::string data_dir = find_plugin_data_dir(plugin_name, m_plugin_dirs);
  m_plugin_data_dirs[plugin_name] = data_dir;
  return data_dir;
}

std::string Manager::build_config_signature() const {
  std::vector<std::string> sorted_plugins = m_config.enabled_plugins;
  std::sort(sorted_plugins.begin(), sorted_plugins.end());
  std::stringstream ss;
  for (const auto &p : sorted_plugins) {
    ss << p << ",";
  }
  return ss.str();
}

void Manager::ensure_plugins_loaded() {
  if (!plugins_loaded) {
    load_plugins();
  }
}

void Manager::load_plugins() {
  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::DEBUG,
                      "Initializing plugins...");

  fs::path cache_dir = Lawnch::Fs::get_cache_home() / "lawnch";
  if (!fs::exists(cache_dir)) {
    fs::create_directories(cache_dir);
  }
  fs::path cache_file = cache_dir / "plugin-triggers.cache";

  if (fs::exists(cache_file)) {
    if (m_trigger_cache.load(build_config_signature())) {
      Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::DEBUG,
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
  m_loaded_triggers.clear();
  m_loaded_help.clear();

  for (const auto &plugin_name : m_config.enabled_plugins) {
    auto loaded = load_plugin_vtable(plugin_name, m_plugin_dirs);
    if (!loaded.handle || !loaded.vtable) {
      continue;
    }

    std::vector<std::string> triggers;
    if (loaded.vtable->get_triggers) {
      const char **plugin_triggers = loaded.vtable->get_triggers();
      while (plugin_triggers && *plugin_triggers) {
        triggers.emplace_back(*plugin_triggers);
        plugin_triggers++;
      }
    }
    m_loaded_triggers[plugin_name] = triggers;

    SearchResult help;
    if (loaded.vtable->get_help) {
      if (LawnchResult *r_ptr = loaded.vtable->get_help()) {
        LawnchResult r = *r_ptr;
        help = SearchResult{
            r.name ? r.name : "",
            r.comment ? r.comment : "",
            r.icon ? r.icon : "",
            r.command ? r.command : "",
            r.type ? r.type : "",
            r.preview_image_path ? r.preview_image_path : "",
            0,
        };
      }
    }
    m_loaded_help[plugin_name] = help;

    dlclose(loaded.handle);
  }

  m_trigger_cache.save(build_config_signature(), m_loaded_triggers,
                       m_loaded_help, m_config.enabled_plugins);

  m_trigger_cache.load(build_config_signature());
}

void Manager::ensure_plugin_for_trigger(const std::string &query) {
  if (!plugins_loaded)
    ensure_plugins_loaded();

  const auto &lazy_triggers = m_trigger_cache.data().lazy_triggers;

  auto it = lazy_triggers.find(query);
  if (it != lazy_triggers.end()) {
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

  for (const auto &[trigger, plugin_name] : lazy_triggers) {
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

  auto loaded = load_plugin_vtable(name, m_plugin_dirs);
  if (!loaded.handle || !loaded.vtable) {
    return;
  }

  auto adapter = std::make_unique<Adapter>(loaded.vtable);

  auto context = std::make_unique<PluginApiContext>(
      PluginApiContext{name, m_config, this, {}});

  context->host_api = {
      .host_api_version = LAWNCH_PLUGIN_API_VERSION,
      .userdata = context.get(),
      .get_config_value = &host_get_config_value,
      .get_data_dir = &host_get_data_dir,
      .log_api = &g_log_api,
      .fs_api = &g_fs_api,
      .str_api = &g_str_api,
      .search_api = &g_search_api,
      .request_results_update = &host_request_results_update,
  };

  adapter->init_with_api(&context->host_api);
  auto triggers = adapter->get_triggers();
  m_loaded_triggers[name] = triggers;
  m_loaded_help[name] = adapter->get_help();
  m_plugin_triggers[adapter.get()] = triggers;

  for (const auto &trigger : triggers) {
    m_trigger_map[trigger] = adapter.get();
  }

  m_handles.push_back(loaded.handle);
  m_api_contexts.push_back(std::move(context));
  m_plugins.push_back(std::move(adapter));

  std::stringstream info_ss;
  info_ss << "Loaded plugin: " << name << " from " << loaded.found_path;
  Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                      info_ss.str());
}

const std::vector<SearchResult> &Manager::get_all_help() const {
  if (!m_cached_help.empty()) {
    return m_cached_help;
  }

  // Use trigger cache help if available (lazy loading mode)
  const auto &cache_help = m_trigger_cache.data().cached_help;
  if (!cache_help.empty()) {
    m_cached_help = cache_help;
    return m_cached_help;
  }

  // Fallback to loaded plugin help
  if (plugins_loaded) {
    for (size_t i = 0; i < m_plugins.size(); ++i) {
      const auto &name = m_api_contexts[i]->plugin_name;
      auto help_it = m_loaded_help.find(name);
      if (help_it != m_loaded_help.end()) {
        m_cached_help.push_back(help_it->second);
      } else {
        m_cached_help.push_back(m_plugins[i]->get_help());
      }
    }
  }
  return m_cached_help;
}

const std::vector<std::string> &
Manager::get_triggers_for(const SearchMode *plugin) const {
  static const std::vector<std::string> empty;
  auto it = m_plugin_triggers.find(plugin);
  if (it != m_plugin_triggers.end()) {
    return it->second;
  }
  return empty;
}

SearchMode *Manager::find_plugin_for_query(const std::string &term,
                                           std::string &out_query) {
  ensure_plugins_loaded();

  const auto &lazy_triggers = m_trigger_cache.data().lazy_triggers;

  for (const auto &[trigger, plugin_name] : lazy_triggers) {
    if (term == trigger) {
      out_query.clear();
      load_plugin(plugin_name);
      for (size_t i = 0; i < m_api_contexts.size(); ++i) {
        if (m_api_contexts[i]->plugin_name == plugin_name) {
          return m_plugins[i].get();
        }
      }
      return nullptr;
    }
    if (term.rfind(trigger + " ", 0) == 0) {
      out_query = term.substr(trigger.length() + 1);
      load_plugin(plugin_name);
      for (size_t i = 0; i < m_api_contexts.size(); ++i) {
        if (m_api_contexts[i]->plugin_name == plugin_name) {
          return m_plugins[i].get();
        }
      }
      return nullptr;
    }
  }
  return nullptr;
}

} // namespace Lawnch::Core::Search::Plugins
