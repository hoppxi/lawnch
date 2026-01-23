#include "pm.hpp"
#include "../helpers/fs.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace Lawnch::CLI {

void PluginManager::print_help() {
  std::cout << "Usage: lawnch pm <command> [arguments]\n\n"
            << "Commands:\n"
            << "  build <dir>                 Build a plugin from source\n"
            << "  install <dir>               Build and install a plugin\n"
            << "  uninstall <name>            Uninstall a plugin\n"
            << "  list [--enabled|--disabled] List plugins\n"
            << "  enable <name>               Enable a plugin\n"
            << "  disable <name>              Disable a plugin\n"
            << "  info <name>                 Show plugin info\n"
            << "  help                        Show this help\n";
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
      build(args[1]);
    } else if (command == "install") {
      if (args.size() < 2)
        throw std::runtime_error("Missing plugin directory");
      install(args[1]);
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

    // Assets section: lines are paths, not key=value
    if (current_section == "assets") {
      // Trim whitespace
      auto start = line.find_first_not_of(" \t");
      auto end = line.find_last_not_of(" \t");
      if (start == std::string::npos)
        continue;
      std::string asset_line = line.substr(start, end - start + 1);

      std::string src, dest;
      auto arrow_pos = asset_line.find("->");
      if (arrow_pos != std::string::npos) {
        src = asset_line.substr(0, arrow_pos);
        dest = asset_line.substr(arrow_pos + 2);
        // Trim whitespace from src and dest
        src.erase(src.find_last_not_of(" \t") + 1);
        src.erase(0, src.find_first_not_of(" \t"));
        dest.erase(dest.find_last_not_of(" \t") + 1);
        dest.erase(0, dest.find_first_not_of(" \t"));
      } else {
        src = asset_line;
        // Use just the filename as destination
        dest = std::filesystem::path(asset_line).filename().string();
      }

      // Normalize src path: remove leading ./ if present
      if (src.rfind("./", 0) == 0) {
        src = src.substr(2);
      }
      // Handle leading / as relative to plugin root, not system root
      if (!src.empty() && src[0] == '/') {
        src = src.substr(1);
      }

      if (!src.empty()) {
        info.assets.push_back({src, dest});
      }
      continue;
    }

    // Other sections use key=value format
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

void PluginManager::build(const std::string &plugin_dir) {
  if (!std::filesystem::exists(plugin_dir)) {
    throw std::runtime_error("Directory not found: " + plugin_dir);
  }

  std::filesystem::path build_dir = std::filesystem::path(plugin_dir) / "build";
  std::filesystem::create_directories(build_dir);

  PluginInfo info = parse_plugin_info(plugin_dir);
  validate_pinfo(info, plugin_dir);
  std::filesystem::path source_path =
      std::filesystem::path(plugin_dir) / info.src_dir;

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
    throw std::runtime_error("CMake configuration failed");
  }

  std::cout << "--- Building plugin ---\n";
  std::string cmd_build = "cmake --build \"" + build_dir.string() + "\"";
  if (std::system(cmd_build.c_str()) != 0) {
    throw std::runtime_error("Build failed");
  }
}

void PluginManager::install(const std::string &plugin_dir) {
  build(plugin_dir);
  PluginInfo info = parse_plugin_info(plugin_dir);

  std::filesystem::path path(plugin_dir);
  std::string plugin_name = info.name;

  std::filesystem::path build_dir = path / "build";
  std::filesystem::path so_file;
  bool found = false;

  if (std::filesystem::exists(build_dir)) {
    for (const auto &entry : std::filesystem::directory_iterator(build_dir)) {
      if (entry.path().extension() == ".so") {
        so_file = entry.path();
        found = true;
        break;
      }
    }
  }

  if (!found) {
    throw std::runtime_error("Built plugin file (.so) not found in " +
                             build_dir.string());
  }

  std::filesystem::path data_home = Lawnch::Fs::get_data_home();
  std::filesystem::path plugin_install_dir =
      data_home / "lawnch" / "plugins" / plugin_name;
  std::filesystem::path assets_install_dir = plugin_install_dir / "assets";

  std::cout << "--- Installing plugin '" << plugin_name << "' ---\n";
  std::filesystem::create_directories(plugin_install_dir);

  std::filesystem::copy_file(so_file,
                             plugin_install_dir / (plugin_name + ".so"),
                             std::filesystem::copy_options::overwrite_existing);

  std::filesystem::copy_file(path / "pinfo", plugin_install_dir / "pinfo",
                             std::filesystem::copy_options::overwrite_existing);

  if (!info.assets.empty()) {
    std::filesystem::create_directories(assets_install_dir);
    for (const auto &[src, dest] : info.assets) {
      std::filesystem::path asset_src = path / src;
      std::filesystem::path asset_dest = assets_install_dir / dest;
      if (std::filesystem::exists(asset_src)) {
        std::filesystem::create_directories(asset_dest.parent_path());
        if (std::filesystem::is_directory(asset_src)) {
          std::filesystem::copy(
              asset_src, asset_dest,
              std::filesystem::copy_options::recursive |
                  std::filesystem::copy_options::overwrite_existing);
        } else {
          std::filesystem::copy_file(
              asset_src, asset_dest,
              std::filesystem::copy_options::overwrite_existing);
        }
      } else {
        std::cerr << "Warning: Asset " << src << " not found at " << asset_src
                  << "\n";
      }
    }
  }

  std::cout << "Plugin '" << plugin_name << "' installed successfully.\n";
  std::cout
      << "WARNING: Installing third-party plugins can be a security risk.\n";
}

void PluginManager::uninstall(const std::string &plugin_name) {
  std::filesystem::path plugin_dir =
      Lawnch::Fs::get_data_home() / "lawnch" / "plugins" / plugin_name;
  if (std::filesystem::exists(plugin_dir)) {
    std::filesystem::remove_all(plugin_dir);
    std::cout << "Plugin '" << plugin_name << "' uninstalled.\n";
  } else {
    std::cerr << "Plugin '" << plugin_name << "' not found.\n";
  }
}

void PluginManager::list(const std::string &filter) {
  std::filesystem::path install_dir =
      Lawnch::Fs::get_data_home() / "lawnch" / "plugins";
  if (!std::filesystem::exists(install_dir)) {
    std::cout << "No plugins installed.\n";
    return;
  }

  std::cout << "Installed plugins:\n";
  for (const auto &entry : std::filesystem::directory_iterator(install_dir)) {
    if (entry.is_directory()) {
      std::string name = entry.path().filename().string();
      std::cout << "  - " << name << "\n";
    }
  }
}

static void update_config(const std::string &plugin_name, bool enabled) {
  std::filesystem::path config_path =
      Lawnch::Fs::get_config_home() / "lawnch" / "config.ini";
  if (!std::filesystem::exists(config_path)) {
    throw std::runtime_error("Config file not found");
  }

  std::ifstream file_in(config_path);
  std::vector<std::string> lines;
  std::string line;
  bool plugins_section_found = false;
  bool key_found = false;

  while (std::getline(file_in, line)) {
    if (line.find("[plugins]") != std::string::npos) {
      plugins_section_found = true;
    }

    if (plugins_section_found && line.find(plugin_name + "=") == 0) {
      lines.push_back(plugin_name + "=" + (enabled ? "true" : "false"));
      key_found = true;
    } else {
      lines.push_back(line);
    }
  }
  file_in.close();

  std::ofstream file_out(config_path);
  for (const auto &l : lines)
    file_out << l << "\n";

  if (!key_found) {
    if (!plugins_section_found) {
      file_out << "\n[plugins]\n";
    }
    file_out << plugin_name << "=" << (enabled ? "true" : "false") << "\n";
  }
}

void PluginManager::enable(const std::string &plugin_name) {
  update_config(plugin_name, true);
  std::cout << "Plugin '" << plugin_name << "' enabled.\n";
}

void PluginManager::disable(const std::string &plugin_name) {
  update_config(plugin_name, false);
  std::cout << "Plugin '" << plugin_name << "' disabled.\n";
}

void PluginManager::info(const std::string &plugin_name) {
  std::filesystem::path pinfo_path = Lawnch::Fs::get_data_home() / "lawnch" /
                                     "plugins" / plugin_name / "pinfo";

  if (!std::filesystem::exists(pinfo_path) &&
      std::filesystem::is_directory(plugin_name)) {
    pinfo_path = std::filesystem::path(plugin_name) / "pinfo";
  }

  if (!std::filesystem::exists(pinfo_path)) {
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
