#include "tm.hpp"
#include "../helpers/fs.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <toml++/toml.hpp>
#include "../core/config/validator.hpp"

namespace Lawnch::CLI {

void ThemeManager::print_help() {
  std::cout << "Usage: lawnch tm <command> [arguments] [options]\n\n"
            << "Commands:\n"
            << "  current                  Show active theme and preset\n"
            << "  list                     List installed themes/presets\n"
            << "  install <path>           Install a theme or preset file\n"
            << "  uninstall <name>         Uninstall a theme or preset\n"
            << "  switch <name>            Switch active theme or preset\n"
            << "  validate <path>          Validate a theme or preset file\n"
            << "  help                     Show this help\n\n"
            << "Options:\n"
            << "  --theme                  Apply the command to themes\n"
            << "  --preset                 Apply the command to presets\n";
}

int ThemeManager::handle_command(const std::vector<std::string> &args) {
  if (args.empty() || args[0] == "help" || args[0] == "--help") {
    print_help();
    return 0;
  }

  std::string command = args[0];
  
  bool is_theme = false;
  bool is_preset = false;
  std::vector<std::string> positional_args;

  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i] == "--theme") {
      is_theme = true;
    } else if (args[i] == "--preset") {
      is_preset = true;
    } else {
      positional_args.push_back(args[i]);
    }
  }

  try {
    if (command == "current") {
      current(is_theme, is_preset);
    } else if (command == "list") {
      if (!is_theme && !is_preset) {
        list_themes();
        list_presets();
      } else {
        if (is_theme) list_themes();
        if (is_preset) list_presets();
      }
    } else if (command == "install") {
      if (positional_args.empty()) {
        throw std::runtime_error("Usage: install <path> [--theme|--preset]");
      }
      if (!is_theme && !is_preset) {
        throw std::runtime_error("Must specify --theme or --preset");
      }
      if (is_theme) install_theme(positional_args[0]);
      if (is_preset) install_preset(positional_args[0]);
    } else if (command == "uninstall") {
      if (positional_args.empty()) {
        throw std::runtime_error("Usage: uninstall <name> [--theme|--preset]");
      }
      if (!is_theme && !is_preset) {
        throw std::runtime_error("Must specify --theme or --preset");
      }
      if (is_theme) uninstall_theme(positional_args[0]);
      if (is_preset) uninstall_preset(positional_args[0]);
    } else if (command == "switch") {
      if (positional_args.empty()) {
        throw std::runtime_error("Usage: switch <name> [--theme|--preset]");
      }
      if (!is_theme && !is_preset) {
        throw std::runtime_error("Must specify --theme or --preset");
      }
      if (is_theme) switch_theme(positional_args[0]);
      if (is_preset) switch_preset(positional_args[0]);
    } else if (command == "validate") {
      if (positional_args.empty()) {
        throw std::runtime_error("Usage: validate <path> [--theme|--preset]");
      }
      if (!is_theme && !is_preset) {
        throw std::runtime_error("Must specify --theme or --preset");
      }
      if (is_theme) validate(positional_args[0], true);
      if (is_preset) validate(positional_args[0], false);
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

void ThemeManager::current(bool is_theme, bool is_preset) {
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

  if (!is_theme && !is_preset) {
    std::cout << "Theme:  " << theme << "\n"
              << "Preset: " << preset << "\n";
  } else {
    if (is_theme) std::cout << theme << "\n";
    if (is_preset) std::cout << preset << "\n";
  }
}

void ThemeManager::validate(const std::string &path_str, bool is_theme) {
  std::filesystem::path src(path_str);
  if (!std::filesystem::exists(src))
    throw std::runtime_error("File not found: " + path_str);

  if (src.extension() != ".toml")
    throw std::runtime_error("File must be .toml");

  try {
    auto tbl = toml::parse_file(src.string());
    
    Lawnch::Core::Config::Validator::ValidationResult val_res;
    if (is_theme) {
      val_res = Lawnch::Core::Config::Validator::validateTheme(tbl);
      std::cout << "[Theme Validation: " << path_str << "]\n";
    } else {
      val_res = Lawnch::Core::Config::Validator::validateConfig(tbl, true);
      std::cout << "[Preset Validation: " << path_str << "]\n";
    }

    if (val_res.success && val_res.warnings.empty()) {
      std::cout << "  \033[32m\u2705 " << (is_theme ? "Theme" : "Preset") << " is valid!\033[0m\n";
    } else {
      if (!val_res.errors.empty()) {
        std::cerr << "  \033[31m\u274C Found " << val_res.errors.size() << " error(s):\033[0m\n";
        for (const auto &err : val_res.errors) {
          std::cerr << "    - " << err << "\n";
        }
      }
      if (!val_res.warnings.empty()) {
        std::cerr << "  \033[33m\u26A0\uFE0F Found " << val_res.warnings.size() << " warning(s):\033[0m\n";
        for (const auto &warn : val_res.warnings) {
          std::cerr << "    - " << warn << "\n";
        }
      }
    }
  } catch (const toml::parse_error &e) {
    throw std::runtime_error("Invalid TOML: " + std::string(e.what()));
  }
}

} // namespace Lawnch::CLI
