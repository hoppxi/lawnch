#include "pm.hpp"
#include "../helpers/fs.hpp"
#include "../helpers/string.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#include <toml++/toml.hpp>

namespace Lawnch::CLI {

void PluginManager::print_help() {
  std::cout << "Usage: lawnch pm <command> [arguments]\n\n"
            << "Commands:\n"
            << "  build <dir>                 Build a plugin from source\n"
            << "  install <dir|url>           Build and install a plugin\n"
            << "  uninstall <name>            Uninstall a plugin\n"
            << "  list [--enabled|--disabled] List plugins\n"
            << "  enable <name>               Enable a plugin\n"
            << "  disable <name>              Disable a plugin\n"
            << "  info <name>                 Show plugin info\n"
            << "  help                        Show this help\n\n"
            << "URL Install:\n"
            << "  lawnch pm install https://github.com/user/plugin-repo\n"
            << "    Clone a plugin repository and install it.\n\n"
            << "  lawnch pm install "
               "https://github.com/user/plugins-repo/plugin-name\n"
            << "    Clone a monorepo and install a specific plugin from\n"
            << "    plugins/<plugin-name>/.\n";
}

int PluginManager::handle_command(const std::vector<std::string> &args) {
  if (args.empty() || args[0] == "help" || args[0] == "--help") {
    print_help();
    return 0;
  }

  std::string command = args[0];

  try {
    if (command == "build") {
      if (args.size() < 2)
        throw std::runtime_error("Missing plugin directory");
      std::string temp_build = build(args[1]);
      std::cout << "Build output in: " << temp_build << "\n";
      std::cout << "Note: clean up with 'rm -rf " << temp_build << "'\n";
    } else if (command == "install") {
      if (args.size() < 2)
        throw std::runtime_error("Missing plugin directory or URL");
      if (Str::is_url(args[1])) {
        install_from_url(args[1]);
      } else {
        install(args[1]);
      }
    } else if (command == "uninstall") {
      if (args.size() < 2)
        throw std::runtime_error("Missing plugin name");
      uninstall(args[1]);
    } else if (command == "list") {
      std::string filter = (args.size() > 1) ? args[1] : "";
      list(filter);
    } else if (command == "enable") {
      if (args.size() < 2)
        throw std::runtime_error("Missing plugin name");
      enable(args[1]);
    } else if (command == "disable") {
      if (args.size() < 2)
        throw std::runtime_error("Missing plugin name");
      disable(args[1]);
    } else if (command == "info") {
      if (args.size() < 2)
        throw std::runtime_error("Missing plugin name");
      info(args[1]);
    } else {
      std::cerr << "Unknown command: " << command << std::endl;
      print_help();
      return 1;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

PluginInfo
PluginManager::parse_plugin_info(const std::string &plugin_dir_or_file) {
  std::filesystem::path path(plugin_dir_or_file);
  std::filesystem::path info_path;

  if (std::filesystem::is_directory(path)) {
    info_path = path / "pinfo";
  } else {
    info_path = path;
  }

  if (!std::filesystem::exists(info_path)) {
    throw std::runtime_error("pinfo file not found at " + info_path.string());
  }

  PluginInfo info;
  info.src_dir = ".";

  std::ifstream file(info_path);
  std::string line;
  std::string current_section;

  while (std::getline(file, line)) {
    if (line.empty() || line[0] == ';' || line[0] == '#')
      continue;

    if (line[0] == '[' && line.back() == ']') {
      current_section = line.substr(1, line.size() - 2);
      continue;
    }

    auto delimiterPos = line.find('=');
    if (delimiterPos == std::string::npos)
      continue;

    std::string key = line.substr(0, delimiterPos);
    std::string value = line.substr(delimiterPos + 1);

    if (current_section == "plugin") {
      if (key == "name")
        info.name = value;
      else if (key == "version")
        info.version = value;
      else if (key == "author")
        info.author = value;
      else if (key == "description")
        info.description = value;
      else if (key == "url")
        info.url = value;
    } else if (current_section == "build") {
      if (key == "src")
        info.src_dir = value;
    } else if (current_section == "dependencies") {
      if (!key.empty())
        info.dependencies.push_back(key);
    }
  }

  if (info.name.empty()) {
    throw std::runtime_error("Plugin name is required in pinfo");
  }

  return info;
}

void PluginManager::validate_pinfo(const PluginInfo &info,
                                   const std::string &base_path) {
  if (info.name.empty())
    throw std::runtime_error("Missing 'name' in [plugin] section");
  if (info.version.empty())
    throw std::runtime_error("Missing 'version' in [plugin] section");
  if (info.description.empty())
    std::cerr << "Warning: Missing 'description' in [plugin] section\n";
  if (info.author.empty())
    std::cerr << "Warning: Missing 'author' in [plugin] section\n";

  std::filesystem::path src_path =
      std::filesystem::path(base_path) / info.src_dir;
  if (!std::filesystem::exists(src_path)) {
    throw std::runtime_error("Source directory not found: " +
                             src_path.string());
  }
}

std::string PluginManager::build(const std::string &plugin_dir) {
  if (!std::filesystem::exists(plugin_dir)) {
    throw std::runtime_error("Directory not found: " + plugin_dir);
  }

  PluginInfo info = parse_plugin_info(plugin_dir);
  validate_pinfo(info, plugin_dir);

  std::filesystem::path abs_path = std::filesystem::absolute(plugin_dir);
  std::filesystem::path source_path = abs_path / info.src_dir;

  std::string temp_dir = Fs::make_temp_dir("lawnch-pm-build");
  std::filesystem::path build_dir = std::filesystem::path(temp_dir);

  std::cout << "--- Configuring plugin '" << info.name << "' v" << info.version
            << " ---\n";
  if (!info.author.empty())
    std::cout << "    Author: " << info.author << "\n";
  if (!info.dependencies.empty()) {
    std::cout << "    Dependencies: ";
    for (const auto &dep : info.dependencies)
      std::cout << dep << " ";
    std::cout << "\n";
  }

  std::string cmd_config = "cmake -S \"" + source_path.string() + "\" -B \"" +
                           build_dir.string() + "\"";
  if (std::system(cmd_config.c_str()) != 0) {
    std::filesystem::remove_all(temp_dir);
    throw std::runtime_error("CMake configuration failed");
  }

  std::cout << "--- Building plugin ---\n";
  std::string cmd_build = "cmake --build \"" + build_dir.string() + "\"";
  if (std::system(cmd_build.c_str()) != 0) {
    std::filesystem::remove_all(temp_dir);
    throw std::runtime_error("Build failed");
  }

  return temp_dir;
}

void PluginManager::install(const std::string &plugin_dir) {
  std::string temp_build_dir = build(plugin_dir);

  PluginInfo info = parse_plugin_info(plugin_dir);
  std::string plugin_name = info.name;

  std::filesystem::path data_home = Lawnch::Fs::get_data_home();
  std::filesystem::path plugin_install_dir =
      data_home / "lawnch" / "plugins" / plugin_name;

  std::cout << "--- Installing plugin '" << plugin_name << "' ---\n";
  std::filesystem::create_directories(plugin_install_dir);

  std::string cmd_install = "cmake --install \"" + temp_build_dir +
                            "\" --prefix \"" + plugin_install_dir.string() +
                            "\"";
  if (std::system(cmd_install.c_str()) != 0) {
    std::filesystem::remove_all(temp_build_dir);
    throw std::runtime_error("cmake --install failed");
  }

  std::cout << "--- Cleaning up build ---\n";
  std::filesystem::remove_all(temp_build_dir);

  std::cout << "Plugin '" << plugin_name << "' installed successfully.\n";
  std::cout
      << "WARNING: Installing third-party plugins can be a security risk.\n";
}

void PluginManager::install_from_url(const std::string &url) {
  std::string repo_url = url;
  std::string plugin_name;

  while (!repo_url.empty() && repo_url.back() == '/') {
    repo_url.pop_back();
  }

  if (repo_url.size() > 4 && repo_url.substr(repo_url.size() - 4) == ".git") {
    repo_url = repo_url.substr(0, repo_url.size() - 4);
  }

  // Count path segments after the host
  // https://github.com/user/repo/plugin-name
  // segments: user, repo, plugin-name
  size_t host_end = repo_url.find("://");
  if (host_end == std::string::npos) {
    throw std::runtime_error("Invalid URL: " + url);
  }
  host_end += 3;
  size_t first_slash = repo_url.find('/', host_end);
  if (first_slash == std::string::npos) {
    throw std::runtime_error("Invalid URL: " + url);
  }

  std::string path_part = repo_url.substr(first_slash + 1);
  std::vector<std::string> segments;
  std::istringstream ss(path_part);
  std::string segment;
  while (std::getline(ss, segment, '/')) {
    if (!segment.empty()) {
      segments.push_back(segment);
    }
  }

  if (segments.size() < 2) {
    throw std::runtime_error("URL must contain at least user/repo: " + url);
  }

  if (segments.size() > 2) {
    plugin_name = segments.back();
    repo_url =
        repo_url.substr(0, host_end) +
        repo_url.substr(
            host_end, repo_url.find('/', first_slash + 1 + segments[0].size() +
                                             1 + segments[1].size()) -
                          host_end);
    std::string host = repo_url.substr(0, first_slash + 1);
    repo_url = host + segments[0] + "/" + segments[1];
  }

  std::cout << "--- Fetching from " << repo_url << " ---\n";
  if (!plugin_name.empty()) {
    std::cout << "    Plugin: " << plugin_name << "\n";
  }

  std::string temp_dir = Fs::make_temp_dir("lawnch-pm");

  std::string clone_cmd =
      "git clone --depth 1 \"" + repo_url + "\" \"" + temp_dir + "/repo\"";
  std::cout << "--- Cloning repository ---\n";
  if (std::system(clone_cmd.c_str()) != 0) {
    std::filesystem::remove_all(temp_dir);
    throw std::runtime_error("Failed to clone repository: " + repo_url);
  }

  std::filesystem::path repo_dir = std::filesystem::path(temp_dir) / "repo";

  try {
    std::filesystem::path install_dir;

    if (!plugin_name.empty()) {
      install_dir = repo_dir / "plugins" / plugin_name;
      if (!std::filesystem::exists(install_dir)) {
        throw std::runtime_error("Plugin '" + plugin_name + "' not found in " +
                                 install_dir.string());
      }
      if (!std::filesystem::exists(install_dir / "pinfo")) {
        throw std::runtime_error("No pinfo found in " + install_dir.string());
      }
    } else {
      if (std::filesystem::exists(repo_dir / "pinfo")) {
        install_dir = repo_dir;
      } else {
        throw std::runtime_error("No pinfo found at repository root: " +
                                 repo_dir.string());
      }
    }

    install(install_dir.string());
  } catch (...) {
    std::filesystem::remove_all(temp_dir);
    throw;
  }

  std::cout << "--- Cleaning up ---\n";
  std::filesystem::remove_all(temp_dir);
}

void PluginManager::uninstall(const std::string &plugin_name) {
  bool uninstalled = false;

  for (const auto &data_dir : Lawnch::Fs::get_data_dirs()) {
    std::filesystem::path plugin_dir =
        std::filesystem::path(data_dir) / "lawnch" / "plugins" / plugin_name;

    if (std::filesystem::exists(plugin_dir)) {
      std::filesystem::remove_all(plugin_dir);
      std::cout << "Plugin '" << plugin_name
                << "' uninstalled from: " << data_dir << "\n";
      uninstalled = true;
      // we could break but we want to uninstall every appearance of the
      // specified plugin
    }
  }

  if (!uninstalled) {
    std::cerr << "Plugin '" << plugin_name
              << "' not found in any data directory.\n";
  }
}

void PluginManager::list(const std::string &filter) {
  std::set<std::string> unique_plugins;
  bool found_any = false;

  for (const auto &data_dir : Lawnch::Fs::get_data_dirs()) {
    std::filesystem::path install_dir =
        std::filesystem::path(data_dir) / "lawnch" / "plugins";

    if (std::filesystem::exists(install_dir) &&
        std::filesystem::is_directory(install_dir)) {
      for (const auto &entry :
           std::filesystem::directory_iterator(install_dir)) {
        if (entry.is_directory()) {
          std::string name = entry.path().filename().string();

          if (filter.empty() || name.find(filter) != std::string::npos) {
            unique_plugins.insert(name);
            found_any = true;
          }
        }
      }
    }
  }

  if (!found_any) {
    std::cout << "No plugins found"
              << (filter.empty() ? "." : " matching filter: " + filter) << "\n";
    return;
  }

  std::cout << "Installed plugins:\n";
  for (const auto &plugin_name : unique_plugins) {
    std::cout << "  - " << plugin_name << "\n";
  }
}

static bool plugin_exists(const std::string &plugin_name) {
  for (const auto &data_dir : Lawnch::Fs::get_data_dirs()) {
    std::filesystem::path plugin_dir =
        std::filesystem::path(data_dir) / "lawnch" / "plugins" / plugin_name;
    if (std::filesystem::exists(plugin_dir) &&
        std::filesystem::is_directory(plugin_dir)) {
      return true;
    }
  }
  return false;
}

static void update_config(const std::string &plugin_name, bool enabled) {
  if (!plugin_exists(plugin_name)) {
    throw std::runtime_error("Cannot update config: Plugin '" + plugin_name +
                             "' is not installed.");
  }

  std::filesystem::path config_path =
      Lawnch::Fs::get_config_home() / "lawnch" / "config.toml";
  std::filesystem::create_directories(config_path.parent_path());

  toml::table tbl;
  if (std::filesystem::exists(config_path)) {
    try {
      tbl = toml::parse_file(config_path.string());
    } catch (const toml::parse_error &e) {
      throw std::runtime_error("Failed to parse config: " +
                               std::string(e.what()));
    }
  }

  if (!tbl.contains("plugin")) {
    tbl.insert_or_assign("plugin", toml::table{});
  }
  auto *plugin_tbl = tbl["plugin"].as_table();

  if (!plugin_tbl->contains(plugin_name)) {
    plugin_tbl->insert_or_assign(plugin_name, toml::table{});
  }
  auto *entry = (*plugin_tbl)[plugin_name].as_table();
  entry->insert_or_assign("enable", enabled);

  std::ofstream out(config_path);
  out << tbl;
}

void PluginManager::enable(const std::string &plugin_name) {
  try {
    update_config(plugin_name, true);
    std::cout << "Plugin '" << plugin_name << "' enabled.\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void PluginManager::disable(const std::string &plugin_name) {
  try {
    update_config(plugin_name, false);
    std::cout << "Plugin '" << plugin_name << "' disabled.\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void PluginManager::info(const std::string &plugin_name) {
  std::filesystem::path pinfo_path;
  bool found = false;

  for (const auto &data_dir : Lawnch::Fs::get_data_dirs()) {
    std::filesystem::path candidate_path = std::filesystem::path(data_dir) /
                                           "lawnch" / "plugins" / plugin_name /
                                           "pinfo";

    if (std::filesystem::exists(candidate_path)) {
      pinfo_path = candidate_path;
      found = true;
      break;
    }
  }

  if (!found && std::filesystem::is_directory(plugin_name)) {
    std::filesystem::path candidate_path =
        std::filesystem::path(plugin_name) / "pinfo";
    if (std::filesystem::exists(candidate_path)) {
      pinfo_path = candidate_path;
      found = true;
    }
  }

  if (!found) {
    throw std::runtime_error("Plugin info not found for: " + plugin_name);
  }

  PluginInfo info = parse_plugin_info(pinfo_path.string());
  std::cout << "Name: " << info.name << "\n"
            << "Version: " << info.version << "\n"
            << "Author: " << info.author << "\n"
            << "Description: " << info.description << "\n"
            << "URL: " << info.url << "\n";
}

} // namespace Lawnch::CLI
