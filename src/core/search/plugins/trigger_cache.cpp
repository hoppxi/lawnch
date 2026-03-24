#include "trigger_cache.hpp"
#include "../../../helpers/fs.hpp"
#include "../../../helpers/logger.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace Lawnch::Core::Search::Plugins {

std::string TriggerCache::get_cache_path() const {
  fs::path cache_dir = fs::path(Lawnch::Fs::get_cache_home()) / "lawnch";
  return (cache_dir / "plugin-triggers.cache").string();
}

bool TriggerCache::load(const std::string &config_signature) {
  std::string cache_file = get_cache_path();
  std::ifstream file(cache_file);
  if (!file.is_open()) {
    return false;
  }

  std::string line;
  std::string current_plugin;
  std::vector<std::string> current_triggers;
  SearchResult current_help;

  m_data.lazy_triggers.clear();
  m_data.cached_help.clear();

  if (std::getline(file, line)) {
    if (line.rfind("CONFIG_SIGNATURE:", 0) == 0) {
      std::string cached_sig = line.substr(17);
      if (cached_sig != config_signature) {
        Lawnch::Logger::log(
            "PluginManager", Lawnch::Logger::LogLevel::INFO,
            "Plugin configuration changed, invalidating cache.");
        return false;
      }
    } else {
      Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::INFO,
                          "Old cache format detected, invalidating cache.");
      return false;
    }
  } else {
    return false;
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
          m_data.lazy_triggers[t] = current_plugin;
        }
        if (current_help.name.empty() && !current_triggers.empty()) {
          current_help.name = current_triggers[0];
        }
        m_data.cached_help.push_back(current_help);
      }
    }
  }

  return !m_data.lazy_triggers.empty();
}

void TriggerCache::save(
    const std::string &config_signature,
    const std::map<std::string, std::vector<std::string>> &loaded_triggers,
    const std::map<std::string, SearchResult> &loaded_help,
    const std::vector<std::string> &enabled_plugins) {

  fs::path cache_dir = fs::path(Lawnch::Fs::get_cache_home()) / "lawnch";
  if (!fs::exists(cache_dir)) {
    fs::create_directories(cache_dir);
  }
  std::string cache_file = get_cache_path();

  std::ofstream file(cache_file);
  if (!file.is_open()) {
    Lawnch::Logger::log("PluginManager", Lawnch::Logger::LogLevel::ERROR,
                        "Failed to open cache file for writing.");
    return;
  }

  std::vector<std::string> sorted_plugins = enabled_plugins;
  std::sort(sorted_plugins.begin(), sorted_plugins.end());

  file << "CONFIG_SIGNATURE:" << config_signature << "\n";

  for (const auto &name : sorted_plugins) {
    std::vector<std::string> triggers;
    SearchResult help;
    auto triggers_it = loaded_triggers.find(name);
    auto help_it = loaded_help.find(name);
    if (triggers_it != loaded_triggers.end()) {
      triggers = triggers_it->second;
    }
    if (help_it != loaded_help.end()) {
      help = help_it->second;
    }

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

} // namespace Lawnch::Core::Search::Plugins
