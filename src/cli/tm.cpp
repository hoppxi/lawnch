#include "tm.hpp"
#include "../core/config/validator.hpp"
#include "../helpers/fs.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
extern "C" {
#include <ini.h>
}

namespace Lawnch::CLI {

void ThemeManager::print_help() {
  std::cout
      << "Usage: lawnch tm <command> [arguments]\n\n"
      << "Commands:\n"
      << "  current                  Show currently active theme\n"
      << "  install <path>           Install a theme file\n"
      << "  install-all <path>       Install all themes from a directory\n"
      << "  uninstall <name>         Uninstall a theme\n"
      << "  list                     List installed themes\n"
      << "  switch <name>            Switch active theme\n"
      << "  help                     Show this help\n";
}

int ThemeManager::handle_command(const std::vector<std::string> &args) {
  if (args.empty() || args[0] == "help" || args[0] == "--help") {
    print_help();
    return 0;
  }

  std::string command = args[0];

  try {
    if (command == "current") {
      current();
    } else if (command == "install") {
      if (args.size() < 2)
        throw std::runtime_error("Usage: install <path>");
      install(args[1]);
    } else if (command == "uninstall") {
      if (args.size() < 2)
        throw std::runtime_error("Usage: uninstall <name>");
      uninstall(args[1]);
    } else if (command == "list") {
      list();
    } else if (command == "switch") {
      if (args.size() < 2)
        throw std::runtime_error("Usage: switch <name>");
      switch_theme(args[1]);
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

bool ThemeManager::validate(const std::string &path) {
  struct UserData {
    bool valid;
  } data = {true};

  int error = ini_parse(
      path.c_str(),
      [](void *user, const char *section, const char *name, const char *value) {
        UserData *ud = (UserData *)user;
        std::string key = std::string(section) + "." + std::string(name);

        if (!Lawnch::Core::Config::Validator::isValidThemeKey(key)) {
          std::cerr << "  [Error] Unknown key: " << key << "\n";
          ud->valid = false;
          return 0;
        }
        return 1;
      },
      &data);

  if (error < 0) {
    std::cerr << "  [Error] Failed to parse INI file: " << path << "\n";
    return false;
  }

  if (!data.valid) {
    std::cerr << "  [Error] File contains invalid keys.\n";
    return false;
  }

  return true;
}

void ThemeManager::install(const std::string &path_str) {
  std::filesystem::path src(path_str);
  if (!std::filesystem::exists(src))
    throw std::runtime_error("File not found: " + path_str);

  if (!validate(path_str)) {
    throw std::runtime_error("Validation failed, aborting installation.");
  }

  std::filesystem::path dest_dir =
      Lawnch::Fs::get_data_home() / "lawnch" / "themes";
  std::filesystem::create_directories(dest_dir);

  std::filesystem::copy_file(src, dest_dir / src.filename(),
                             std::filesystem::copy_options::overwrite_existing);
  std::cout << "Installed theme: " << src.filename().stem().string() << "\n";
}

void ThemeManager::uninstall(const std::string &name) {
  std::filesystem::path file =
      Lawnch::Fs::get_data_home() / "lawnch" / "themes" / (name + ".ini");
  if (std::filesystem::exists(file)) {
    std::filesystem::remove(file);
    std::cout << "Uninstalled theme: " << name << "\n";
  } else {
    std::cerr << "Theme '" << name << "' not found.\n";
  }
}

void ThemeManager::list() {
  std::filesystem::path dir = Lawnch::Fs::get_data_home() / "lawnch" / "themes";
  std::cout << "Installed Themes:\n";
  if (std::filesystem::exists(dir)) {
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".ini") {
        std::cout << "  - " << entry.path().stem().string() << "\n";
      }
    }
  }
}

static void update_config_key(const std::string &key,
                              const std::string &value) {
  std::filesystem::path config_path =
      Lawnch::Fs::get_config_home() / "lawnch" / "config.ini";
  if (!std::filesystem::exists(config_path)) {
    throw std::runtime_error("Config file not found");
  }

  std::ifstream file_in(config_path);
  std::vector<std::string> lines;
  std::string line;
  bool layout_section = false;
  bool key_found = false;

  std::string target_key = key + "=";

  while (std::getline(file_in, line)) {
    if (line.find("[layout]") != std::string::npos) {
      layout_section = true;
    } else if (line.find("[") == 0) {
      layout_section = false;
    }

    if (layout_section && line.find(target_key) == 0) {
      lines.push_back(target_key + value);
      key_found = true;
    } else {
      lines.push_back(line);
    }
  }
  file_in.close();

  std::ofstream file_out(config_path);
  for (const auto &l : lines)
    file_out << l << "\n";
}

void ThemeManager::switch_theme(const std::string &name) {
  std::filesystem::path file =
      Lawnch::Fs::get_data_home() / "lawnch" / "themes" / (name + ".ini");

  if (!std::filesystem::exists(file)) {
    bool found = false;
    for (const auto &dir : Lawnch::Fs::get_data_dirs()) {
      std::filesystem::path p =
          std::filesystem::path(dir) / "lawnch" / "themes" / (name + ".ini");
      if (std::filesystem::exists(p)) {
        found = true;
        break;
      }
    }
    if (!std::filesystem::exists(file) && !found) {
      throw std::runtime_error("Theme '" + name + "' not found.");
    }
  }

  update_config_key("theme", name);
  std::cout << "Switched theme to: " << name << "\n";
}

void ThemeManager::current() {
  std::filesystem::path config_path =
      Lawnch::Fs::get_config_home() / "lawnch" / "config.ini";
  if (!std::filesystem::exists(config_path)) {
    std::cout << "Config file not found.\n";
    return;
  }

  std::string current_theme = "gruvbox-compact";

  ini_parse(
      config_path.c_str(),
      [](void *user, const char *section, const char *name, const char *value) {
        std::string *theme = (std::string *)user;
        std::string key = std::string(section) + "." + std::string(name);
        if (key == "layout.theme")
          *theme = value;
        return 1;
      },
      &current_theme);

  std::cout << "Current Theme: " << current_theme << "\n";
}

} // namespace Lawnch::CLI
