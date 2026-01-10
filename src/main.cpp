#include "config/config_manager.hpp"
#include "helpers/fs.hpp"
#include "helpers/logger.hpp"
#include "search/plugin_manager.hpp"
#include "search/search.hpp"
#include "window/window.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>

namespace fs = std::filesystem;

bool is_another_instance_running() {
  try {
    std::string lock_path =
        Lawnch::Fs::get_cache_dir() / "lawnch/running.status";

    fs::path p(lock_path);
    if (!fs::exists(p.parent_path())) {
      fs::create_directories(p.parent_path());
    }

    if (fs::exists(p)) {
      std::ifstream lock_file(lock_path);
      std::string status;
      lock_file >> status;
      if (status == "running") {
        return true;
      }
    }
  } catch (const std::exception &e) {
    std::stringstream ss;
    ss << "Error checking for another instance: " << e.what();
    Logger::log("main", Logger::LogLevel::ERROR, ss.str());
    return false;
  }
  return false;
}

void set_running_status(bool running) {
  try {
    std::string lock_path =
        Lawnch::Fs::get_cache_dir() / "lawnch/running.status";
    std::ofstream lock_file(lock_path, std::ios::trunc);
    if (lock_file.is_open()) {
      lock_file << (running ? "running" : "idle");
    }
  } catch (const std::exception &e) {
    std::stringstream ss;
    ss << "Error setting running status: " << e.what();
    Logger::log("main", Logger::LogLevel::ERROR, ss.str());
  }
}

int main(int argc, char *argv[]) {
  std::filesystem::path log_path = Lawnch::Fs::get_log_path("lawnch");
  Logger::init(log_path);
  Logger::log("main", Logger::LogLevel::INFO, "Application starting");

  if (is_another_instance_running()) {
    Logger::log("main", Logger::LogLevel::INFO,
                "Another instance is running, exiting");
    return 0;
  }

  try {
    set_running_status(true);

    std::string config_path =
        Lawnch::Fs::get_config_home() / "lawnch/config.ini";

    ConfigManager::Instance().Load(config_path);
    Logger::log("main", Logger::LogLevel::INFO, "Configuration loaded");

    const auto &cfg = ConfigManager::Instance().Get();

    PluginManager plugin_manager(cfg);
    Logger::log("main", Logger::LogLevel::INFO, "PluginManager initialized");

    plugin_manager.load_plugins();
    Logger::log("main", Logger::LogLevel::INFO, "Plugins loaded");

    Search search(plugin_manager);
    LauncherWindow launcher(search);

    launcher.run();
    Logger::log("main", Logger::LogLevel::INFO, "Launcher finished");

    set_running_status(false);
  } catch (const std::exception &e) {
    set_running_status(false);
    std::stringstream ss;
    ss << "Fatal Error: " << e.what();
    Logger::log("main", Logger::LogLevel::CRITICAL, ss.str());
    return 1;
  }

  return 0;
}
