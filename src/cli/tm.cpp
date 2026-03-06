#include "tm.hpp"
#include "../helpers/fs.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <toml++/toml.hpp>

namespace Lawnch::CLI {

void ThemeManager::print_help() {
  std::cout << "Usage: lawnch tm <command> [arguments]\n\n"
            << "Commands:\n"
            << "  current                  Show active theme and preset\n"
            << "  theme list               List installed themes\n"
            << "  theme install <path>     Install a theme file\n"
            << "  theme uninstall <name>   Uninstall a theme\n"
            << "  theme switch <name>      Switch active theme\n"
            << "  preset list              List installed presets\n"
            << "  preset install <path>    Install a preset file\n"
            << "  preset uninstall <name>  Uninstall a preset\n"
            << "  preset switch <name>     Switch active preset\n"
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
    } else if (command == "theme") {
      if (args.size() < 2) {
        print_help();
        return 1;
      }
      std::string sub = args[1];
      if (sub == "list") {
        list_themes();
      } else if (sub == "install") {
        if (args.size() < 3)
          throw std::runtime_error("Usage: theme install <path>");
        install_theme(args[2]);
      } else if (sub == "uninstall") {
        if (args.size() < 3)
          throw std::runtime_error("Usage: theme uninstall <name>");
        uninstall_theme(args[2]);
      } else if (sub == "switch") {
        if (args.size() < 3)
          throw std::runtime_error("Usage: theme switch <name>");
        switch_theme(args[2]);
      } else {
        std::cerr << "Unknown theme command: " << sub << std::endl;
        return 1;
      }
    } else if (command == "preset") {
      if (args.size() < 2) {
        print_help();
        return 1;
      }
      std::string sub = args[1];
      if (sub == "list") {
        list_presets();
      } else if (sub == "install") {
        if (args.size() < 3)
          throw std::runtime_error("Usage: preset install <path>");
        install_preset(args[2]);
      } else if (sub == "uninstall") {
        if (args.size() < 3)
          throw std::runtime_error("Usage: preset uninstall <name>");
        uninstall_preset(args[2]);
      } else if (sub == "switch") {
        if (args.size() < 3)
          throw std::runtime_error("Usage: preset switch <name>");
        switch_preset(args[2]);
      } else {
        std::cerr << "Unknown preset command: " << sub << std::endl;
        return 1;
      }
    } else {
      if (command == "list") {
        list_themes();
      } else if (command == "install") {
        if (args.size() < 2)
          throw std::runtime_error("Usage: install <path>");
        install_theme(args[1]);
      } else if (command == "uninstall") {
        if (args.size() < 2)
          throw std::runtime_error("Usage: uninstall <name>");
        uninstall_theme(args[1]);
      } else if (command == "switch") {
        if (args.size() < 2)
          throw std::runtime_error("Usage: switch <name>");
        switch_theme(args[1]);
      } else {
        std::cerr << "Unknown command: " << command << std::endl;
        print_help();
        return 1;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

static void list_toml_files(const std::string &subdir,
                            const std::string &label) {
  std::cout << label << ":\n";
  bool found = false;

  std::filesystem::path user_dir =
      Lawnch::Fs::get_data_home() / "lawnch" / subdir;
  if (std::filesystem::exists(user_dir)) {
    for (const auto &entry : std::filesystem::directory_iterator(user_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".toml") {
        std::string name = entry.path().stem().string();
        try {
          auto tbl = toml::parse_file(entry.path().string());
          if (auto *meta = tbl["meta"].as_table()) {
            if (auto n = (*meta)["name"].as_string()) {
              std::cout << "  - " << name << " (" << n->get() << ") [user]\n";
              found = true;
              continue;
            }
          }
        } catch (...) {
        }
        std::cout << "  - " << name << " [user]\n";
        found = true;
      }
    }
  }

  for (const auto &data_dir : Lawnch::Fs::get_data_dirs()) {
    std::filesystem::path sys_dir =
        std::filesystem::path(data_dir) / "lawnch" / subdir;
    if (std::filesystem::exists(sys_dir)) {
      for (const auto &entry : std::filesystem::directory_iterator(sys_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".toml") {
          std::string name = entry.path().stem().string();
          try {
            auto tbl = toml::parse_file(entry.path().string());
            if (auto *meta = tbl["meta"].as_table()) {
              if (auto n = (*meta)["name"].as_string()) {
                std::cout << "  - " << name << " (" << n->get() << ")\n";
                found = true;
                continue;
              }
            }
          } catch (...) {
          }
          std::cout << "  - " << name << "\n";
          found = true;
        }
      }
    }
  }

  if (!found) {
    std::cout << "  (none found)\n";
  }
}

static bool find_toml_file(const std::string &name, const std::string &subdir) {
  std::filesystem::path user_file =
      Lawnch::Fs::get_data_home() / "lawnch" / subdir / (name + ".toml");
  if (std::filesystem::exists(user_file))
    return true;

  for (const auto &dir : Lawnch::Fs::get_data_dirs()) {
    std::filesystem::path p =
        std::filesystem::path(dir) / "lawnch" / subdir / (name + ".toml");
    if (std::filesystem::exists(p))
      return true;
  }

  return false;
}

static void update_toml_key(const std::string &table_path,
                            const std::string &key, const std::string &value) {
  std::filesystem::path config_path =
      Lawnch::Fs::get_config_home() / "lawnch" / "config.toml";

  if (!std::filesystem::exists(config_path)) {
    std::filesystem::create_directories(config_path.parent_path());
    std::ofstream out(config_path);
    out << "[" << table_path << "]\n" << key << " = \"" << value << "\"\n";
    return;
  }

  try {
    auto tbl = toml::parse_file(config_path.string());

    toml::table *target = &tbl;
    std::istringstream ss(table_path);
    std::string segment;
    while (std::getline(ss, segment, '.')) {
      if (!target->contains(segment)) {
        target->insert_or_assign(segment, toml::table{});
      }
      target = (*target)[segment].as_table();
    }

    target->insert_or_assign(key, value);

    std::ofstream out(config_path);
    out << tbl;
  } catch (const toml::parse_error &e) {
    throw std::runtime_error("Failed to parse config: " +
                             std::string(e.what()));
  }
}

void ThemeManager::install_theme(const std::string &path_str) {
  std::filesystem::path src(path_str);
  if (!std::filesystem::exists(src))
    throw std::runtime_error("File not found: " + path_str);

  if (src.extension() != ".toml")
    throw std::runtime_error("Theme file must be .toml");

  try {
    auto tbl = toml::parse_file(src.string());
    if (!tbl.contains("colors"))
      throw std::runtime_error("Theme file missing [colors] table");
  } catch (const toml::parse_error &e) {
    throw std::runtime_error("Invalid TOML: " + std::string(e.what()));
  }

  std::filesystem::path dest_dir =
      Lawnch::Fs::get_data_home() / "lawnch" / "themes";
  std::filesystem::create_directories(dest_dir);

  std::filesystem::copy_file(src, dest_dir / src.filename(),
                             std::filesystem::copy_options::overwrite_existing);
  std::cout << "Installed theme: " << src.stem().string() << "\n";
}

void ThemeManager::uninstall_theme(const std::string &name) {
  std::filesystem::path file =
      Lawnch::Fs::get_data_home() / "lawnch" / "themes" / (name + ".toml");
  if (std::filesystem::exists(file)) {
    std::filesystem::remove(file);
    std::cout << "Uninstalled theme: " << name << "\n";
  } else {
    std::cerr << "Theme '" << name
              << "' not found in user directory. System themes cannot be "
                 "uninstalled.\n";
  }
}

void ThemeManager::list_themes() { list_toml_files("themes", "Themes"); }

void ThemeManager::switch_theme(const std::string &name) {
  if (!find_toml_file(name, "themes")) {
    throw std::runtime_error("Theme '" + name + "' not found.");
  }
  update_toml_key("appearance", "theme", name);
  std::cout << "Switched theme to: " << name << "\n";
}

void ThemeManager::install_preset(const std::string &path_str) {
  std::filesystem::path src(path_str);
  if (!std::filesystem::exists(src))
    throw std::runtime_error("File not found: " + path_str);

  if (src.extension() != ".toml")
    throw std::runtime_error("Preset file must be .toml");

  try {
    auto _ = toml::parse_file(src.string());
  } catch (const toml::parse_error &e) {
    throw std::runtime_error("Invalid TOML: " + std::string(e.what()));
  }

  std::filesystem::path dest_dir =
      Lawnch::Fs::get_data_home() / "lawnch" / "presets";
  std::filesystem::create_directories(dest_dir);

  std::filesystem::copy_file(src, dest_dir / src.filename(),
                             std::filesystem::copy_options::overwrite_existing);
  std::cout << "Installed preset: " << src.stem().string() << "\n";
}

void ThemeManager::uninstall_preset(const std::string &name) {
  std::filesystem::path file =
      Lawnch::Fs::get_data_home() / "lawnch" / "presets" / (name + ".toml");
  if (std::filesystem::exists(file)) {
    std::filesystem::remove(file);
    std::cout << "Uninstalled preset: " << name << "\n";
  } else {
    std::cerr << "Preset '" << name
              << "' not found in user directory. System presets cannot be "
                 "uninstalled.\n";
  }
}

void ThemeManager::list_presets() { list_toml_files("presets", "Presets"); }

void ThemeManager::switch_preset(const std::string &name) {
  if (!find_toml_file(name, "presets")) {
    throw std::runtime_error("Preset '" + name + "' not found.");
  }
  update_toml_key("appearance", "preset", name);
  std::cout << "Switched preset to: " << name << "\n";
}

void ThemeManager::current() {
  std::filesystem::path config_path =
      Lawnch::Fs::get_config_home() / "lawnch" / "config.toml";
  if (!std::filesystem::exists(config_path)) {
    std::cout << "Config file not found.\n";
    return;
  }

  std::string theme = "gruvbox";
  std::string preset = "compact";

  try {
    auto tbl = toml::parse_file(config_path.string());
    if (auto *app = tbl["appearance"].as_table()) {
      if (auto t = (*app)["theme"].as_string())
        theme = t->get();
      if (auto p = (*app)["preset"].as_string())
        preset = p->get();
    }
  } catch (const toml::parse_error &e) {
    std::cerr << "Failed to parse config: " << e.what() << "\n";
    return;
  }

  std::cout << "Theme:  " << theme << "\n"
            << "Preset: " << preset << "\n";
}

} // namespace Lawnch::CLI
