#include "tm.hpp"
#include "../helpers/fs.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
extern "C" {
#include <ini.h>
}

#include <set>

namespace Lawnch::CLI {

static const std::set<std::string> VALID_THEME_KEYS = {
    // window
    "window.width", "window.height", "window.anchor", "window.margin",
    "window.border_radius", "window.border_width", "window.exclusive_zone",
    "window.ignore_exclusive_zones", "window.keyboard_interactivity",
    "window.border_color", "window.background_color",

    // layout
    "layout.preview_position", "layout.preview_width_percent", "layout.order",
    "layout.theme",

    // input
    "input.font_family", "input.font_size", "input.font_weight",
    "input.placeholder_text", "input.caret_width", "input.padding",
    "input.border_radius", "input.border_width", "input.horizontal_align",
    "input.width", "input.text_align", "input.text_color",
    "input.placeholder_color", "input.background_color", "input.caret_color",
    "input.selection_color", "input.selection_text_color", "input.border_color",

    // input_prompt
    "input_prompt.enable", "input_prompt.content", "input_prompt.position",
    "input_prompt.font_family", "input_prompt.font_size",
    "input_prompt.font_weight", "input_prompt.text_color",
    "input_prompt.background_color", "input_prompt.border_radius",
    "input_prompt.border_width", "input_prompt.border_color",
    "input_prompt.padding",

    // results
    "results.margin", "results.padding", "results.spacing",
    "results.border_width", "results.border_radius", "results.enable_scrollbar",
    "results.scrollbar_width", "results.scrollbar_padding",
    "results.scrollbar_radius", "results.scroll_mode", "results.reverse",
    "results.background_color", "results.border_color",
    "results.scrollbar_color", "results.scrollbar_bg_color",

    // rresult_item
    "result_item.font_family", "result_item.font_size",
    "result_item.font_weight", "result_item.text_align",
    "result_item.enable_comment", "result_item.comment_font_size",
    "result_item.comment_font_weight", "result_item.enable_icon",
    "result_item.icon_size", "result_item.icon_padding_right",
    "result_item.padding", "result_item.border_radius",
    "result_item.border_width", "result_item.selected_border_radius",
    "result_item.selected_border_width", "result_item.enable_highlight",
    "result_item.highlight_font_weight", "result_item.text_color",
    "result_item.comment_color", "result_item.selected_comment_color",
    "result_item.background_color", "result_item.border_color",
    "result_item.selected_background_color", "result_item.selected_text_color",
    "result_item.selected_border_color", "result_item.highlight_color",
    "result_item.selected_highlight_color",

    // preview
    "preview.enable", "preview.icon_size", "preview.padding",
    "preview.vertical_spacing", "preview.horizontal_spacing", "preview.show",
    "preview.name_font_family", "preview.name_font_size",
    "preview.name_font_weight", "preview.comment_font_size",
    "preview.comment_font_weight", "preview.hide_icon_if_fallback",
    "preview.fallback_icon", "preview.preview_image_size",
    "preview.background_color", "preview.name_color", "preview.comment_color",

    // results count
    "results_count.enable", "results_count.format", "results_count.font_family",
    "results_count.font_size", "results_count.font_weight",
    "results_count.text_align", "results_count.padding",
    "results_count.text_color",

    // clock
    "clock.enable", "clock.format", "clock.font_family", "clock.font_size",
    "clock.font_weight", "clock.padding", "clock.text_align",
    "clock.text_color"};

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
      << "  validate <path>          Validate a theme file\n"
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
    } else if (command == "install-all") {
      if (args.size() < 2)
        throw std::runtime_error("Usage: install-all <path>");
      install_all(args[1]);
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
    } else if (command == "validate") {
      if (args.size() < 2)
        throw std::runtime_error("Usage: validate <path>");
      validate(args[1]);
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

static bool validate_file(const std::string &path) {
  struct UserData {
    bool valid;
  } data = {true};

  int error = ini_parse(
      path.c_str(),
      [](void *user, const char *section, const char *name, const char *value) {
        UserData *ud = (UserData *)user;
        std::string key = std::string(section) + "." + std::string(name);

        if (VALID_THEME_KEYS.find(key) == VALID_THEME_KEYS.end()) {
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

void ThemeManager::validate(const std::string &path) {
  if (validate_file(path)) {
    std::cout << "Validation successful: " << path << "\n";
  } else {
    throw std::runtime_error("Validation failed");
  }
}

void ThemeManager::install(const std::string &path_str) {
  std::filesystem::path src(path_str);
  if (!std::filesystem::exists(src))
    throw std::runtime_error("File not found: " + path_str);

  if (!validate_file(path_str)) {
    throw std::runtime_error("Validation failed, aborting installation.");
  }

  std::filesystem::path dest_dir =
      Lawnch::Fs::get_data_home() / "lawnch" / "themes";
  std::filesystem::create_directories(dest_dir);

  std::filesystem::copy_file(src, dest_dir / src.filename(),
                             std::filesystem::copy_options::overwrite_existing);
  std::cout << "Installed theme: " << src.filename().stem().string() << "\n";
}

void ThemeManager::install_all(const std::string &dir_path) {
  std::filesystem::path root(dir_path);
  if (!std::filesystem::exists(root))
    throw std::runtime_error("Directory not found: " + dir_path);

  std::cout << "Installing all themes from " << dir_path << "...\n";

  int success_count = 0;

  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(root)) {
    if (entry.is_regular_file() && entry.path().extension() == ".ini") {
      std::string filename = entry.path().filename().string();
      try {
        install(entry.path().string());
        success_count++;
      } catch (const std::exception &e) {
        std::cerr << "Failed to install " << filename << ": " << e.what()
                  << "\n";
      }
    }
  }
  std::cout << "Installed " << success_count << " files.\n";
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
