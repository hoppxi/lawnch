#include "manager.hpp"
#include "../../helpers/fs.hpp"
#include "../../helpers/logger.hpp"
#include "../../helpers/string.hpp"

extern "C" {
#include <ini.h>
}

#include <filesystem>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

namespace Lawnch::Core::Config {

struct Manager::Impl {
  Config config;
  mutable std::shared_mutex config_mutex;
  using BinderFunc = std::function<void(const std::string &)>;
  std::unordered_map<std::string, BinderFunc> bindings;

  Impl() { SetDefaultsAndBindings(); }

  template <typename T>
  void Bind(const std::string &section, const std::string &name, T &member,
            const T &default_val) {

    member = default_val;
    std::string key = section + "." + name;

    bindings[key] = [&member, key](const std::string &raw_value) {
      std::string value = Lawnch::Str::trim(raw_value);
      try {
        if constexpr (std::is_same_v<T, int>) {
          member = std::stoi(value);
        } else if constexpr (std::is_same_v<T, double>) {
          member = std::stod(value);
        } else if constexpr (std::is_same_v<T, std::string>) {
          member = value;
        } else if constexpr (std::is_same_v<T, bool>) {
          member = Lawnch::Config::parseBool(value);
        } else if constexpr (std::is_same_v<T, Lawnch::Config::Color>) {
          member = Lawnch::Config::parseColor(value);
        } else if constexpr (std::is_same_v<T, Lawnch::Config::Padding>) {
          member = Lawnch::Config::parsePadding(value);
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
          member = Lawnch::Config::parseStringList(value);
        }
      } catch (const std::exception &e) {
        std::stringstream ss;
        ss << "Error parsing [" << key << "] with value '" << value
           << "': " << e.what();
        Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::ERROR,
                            ss.str());
      }
    };
  }

  void SetDefaultsAndBindings() {
    bindings.clear();
    config.enabled_plugins.clear();
    config.plugin_configs.clear();
    config.keybindings.clear();

    Bind("bindings", "preset", config.bindings_preset, std::string("default"));

    // general
    Bind("general", "icon_theme", config.general_icon_theme,
         std::string("Papirus-Dark"));
    Bind("general", "terminal", config.general_terminal, std::string("auto"));
    Bind("general", "terminal_exec_flag", config.general_terminal_exec_flag,
         std::string("-e"));
    Bind("general", "editor", config.general_editor, std::string("auto"));
    Bind("general", "history_max_size", config.general_history_max_size, 100);
    Bind("general", "locale", config.general_locale, std::string(""));

    // launch
    Bind("launch", "app_cmd", config.launch_app_cmd, std::string("{}"));
    Bind("launch", "terminal_app_cmd", config.launch_terminal_app_cmd,
         std::string("{terminal} {terminal_exec_flag} {}"));
    Bind("launch", "prefix", config.launch_prefix, std::string(""));
    Bind("launch", "context", config.launch_context, std::string(""));
    Bind("launch", "start_with", config.launch_start_with,
         std::string(":apps"));

    // layout
    Bind("layout", "theme", config.layout_theme,
         std::string("gruvbox-compact"));
    Bind("layout", "order", config.layout_order,
         std::vector<std::string>{"input", "results_count", "results",
                                  "preview"});
    Bind("layout", "orientation", config.layout_orientation,
         std::string("vertical"));
    Bind("layout", "preview_position", config.layout_preview_position,
         std::string("bottom"));
    Bind("layout", "preview_width_percent", config.layout_preview_width_percent,
         30);

    // window
    Bind("window", "width", config.window_width, 400);
    Bind("window", "height", config.window_height, 370);
    Bind("window", "anchor", config.window_anchor, std::string("top, center"));
    Bind("window", "margin", config.window_margin,
         Lawnch::Config::Padding(50, 0, 0, 0));
    Bind("window", "background_color", config.window_background_color,
         {0.1, 0.1, 0.1, 0.9});
    Bind("window", "border_color", config.window_border_color,
         {0.2, 0.2, 0.2, 1.0});
    Bind("window", "border_radius", config.window_border_radius, 10);
    Bind("window", "border_width", config.window_border_width, 2);
    Bind("window", "exclusive_zone", config.window_exclusive_zone, false);
    Bind("window", "ignore_exclusive_zones",
         config.window_ignore_exclusive_zones, false);
    Bind("window", "keyboard_interactivity",
         config.window_keyboard_interactivity, std::string("exclusive"));

    // input
    Bind("input", "visible", config.input_visible, true);
    Bind("input", "font_family", config.input_font_family,
         std::string("sans-serif"));
    Bind("input", "font_size", config.input_font_size, 24);
    Bind("input", "font_weight", config.input_font_weight,
         std::string("normal"));
    Bind("input", "text_color", config.input_text_color, {1.0, 1.0, 1.0, 1.0});
    Bind("input", "placeholder_text", config.input_placeholder_text,
         std::string("Search..."));
    Bind("input", "placeholder_color", config.input_placeholder_color,
         {0.6, 0.6, 0.6, 1.0});
    Bind("input", "background_color", config.input_background_color,
         {0.15, 0.15, 0.15, 1.0});
    Bind("input", "caret_color", config.input_caret_color,
         {1.0, 1.0, 1.0, 1.0});
    Bind("input", "caret_width", config.input_caret_width, 2.0);
    Bind("input", "selection_color", config.input_selection_color,
         {0.2, 0.4, 0.8, 0.5});
    Bind("input", "selection_text_color", config.input_selection_text_color,
         {1.0, 1.0, 1.0, 1.0});
    Bind("input", "padding", config.input_padding,
         Lawnch::Config::Padding(12, 16, 12, 16));
    Bind("input", "border_radius", config.input_border_radius, 8);
    Bind("input", "border_width", config.input_border_width, 2);
    Bind("input", "border_color", config.input_border_color,
         {0.3, 0.3, 0.3, 1.0});
    Bind("input", "horizontal_align", config.input_horizontal_align,
         std::string("left"));

    // input prompt
    Bind("input_prompt", "enable", config.input_prompt_enable, false);
    Bind("input_prompt", "content", config.input_prompt_content,
         std::string(""));
    Bind("input_prompt", "position", config.input_prompt_position,
         std::string("left"));
    Bind("input_prompt", "font_family", config.input_prompt_font_family,
         std::string("sans-serif"));
    Bind("input_prompt", "font_size", config.input_prompt_font_size, 14);
    Bind("input_prompt", "font_weight", config.input_prompt_font_weight,
         std::string("normal"));
    Bind("input_prompt", "text_color", config.input_prompt_text_color,
         {0.9, 0.9, 0.9, 1.0});
    Bind("input_prompt", "background_color",
         config.input_prompt_background_color, {0.0, 0.0, 0.0, 0.0});
    Bind("input_prompt", "border_radius", config.input_prompt_border_radius, 0);
    Bind("input_prompt", "border_width", config.input_prompt_border_width, 0);
    Bind("input_prompt", "border_color", config.input_prompt_border_color,
         {0.0, 0.0, 0.0, 0.0});
    Bind("input_prompt", "padding", config.input_prompt_padding,
         Lawnch::Config::Padding(0, 5, 0, 5));

    // results
    Bind("results", "margin", config.results_margin,
         Lawnch::Config::Padding(10, 0, 0, 0));
    Bind("results", "padding", config.results_padding,
         Lawnch::Config::Padding(0));
    Bind("results", "spacing", config.results_spacing, 5);
    Bind("results", "background_color", config.results_background_color,
         {0.0, 0.0, 0.0, 0.0});
    Bind("results", "border_color", config.results_border_color,
         {0.0, 0.0, 0.0, 0.0});
    Bind("results", "border_width", config.results_border_width, 0);
    Bind("results", "border_radius", config.results_border_radius, 0);
    Bind("results", "enable_scrollbar", config.results_enable_scrollbar, true);
    Bind("results", "scrollbar_width", config.results_scrollbar_width, 4);
    Bind("results", "scrollbar_padding", config.results_scrollbar_padding, 2);
    Bind("results", "scrollbar_radius", config.results_scrollbar_radius, 2);
    Bind("results", "scrollbar_color", config.results_scrollbar_color,
         {0.8, 0.8, 0.8, 0.5});
    Bind("results", "scrollbar_bg_color", config.results_scrollbar_bg_color,
         {0.2, 0.2, 0.2, 0.2});
    Bind("results", "scroll_mode", config.results_scroll_mode,
         std::string("follow"));
    Bind("results", "reverse", config.results_reverse, false);

    // result item
    Bind("result_item", "font_family", config.result_item_font_family,
         std::string("sans-serif"));
    Bind("result_item", "font_size", config.result_item_font_size, 16);
    Bind("result_item", "font_weight", config.result_item_font_weight,
         std::string("normal"));
    Bind("result_item", "text_align", config.result_item_text_align,
         std::string("left"));
    Bind("result_item", "enable_comment", config.result_item_enable_comment,
         true);
    Bind("result_item", "comment_font_size",
         config.result_item_comment_font_size, 10);
    Bind("result_item", "comment_font_weight",
         config.result_item_comment_font_weight, std::string("normal"));
    Bind("result_item", "comment_color", config.result_item_comment_color,
         {0.6, 0.6, 0.6, 1.0});
    Bind("result_item", "selected_comment_color",
         config.result_item_selected_comment_color, {0.7, 0.7, 0.7, 1.0});
    Bind("result_item", "enable_icon", config.result_item_enable_icon, true);
    Bind("result_item", "icon_size", config.result_item_icon_size, 24);
    Bind("result_item", "icon_padding_right",
         config.result_item_icon_padding_right, 12);
    Bind("result_item", "padding", config.result_item_padding,
         Lawnch::Config::Padding(10));
    Bind("result_item", "border_radius", config.result_item_border_radius, 6);
    Bind("result_item", "border_width", config.result_item_border_width, 0);
    Bind("result_item", "border_color", config.result_item_border_color,
         {1.0, 1.0, 1.0, 0.1});
    Bind("result_item", "background_color", config.result_item_background_color,
         {0.0, 0.0, 0.0, 0.0});
    Bind("result_item", "text_color", config.result_item_text_color,
         {0.9, 0.9, 0.9, 1.0});
    Bind("result_item", "selected_border_radius",
         config.result_item_selected_border_radius, 6);
    Bind("result_item", "selected_border_width",
         config.result_item_selected_border_width, 2);
    Bind("result_item", "selected_border_color",
         config.result_item_selected_border_color, {0.3, 0.6, 1.0, 1.0});
    Bind("result_item", "selected_background_color",
         config.result_item_selected_background_color, {1.0, 1.0, 1.0, 0.05});
    Bind("result_item", "selected_text_color",
         config.result_item_selected_text_color, {1.0, 1.0, 1.0, 1.0});
    Bind("result_item", "enable_highlight", config.result_item_enable_highlight,
         false);
    Bind("result_item", "highlight_color", config.result_item_highlight_color,
         {1.0, 0.78, 0.0, 1.0});
    Bind("result_item", "highlight_font_weight",
         config.result_item_highlight_font_weight, std::string("bold"));
    Bind("result_item", "selected_highlight_color",
         config.result_item_selected_highlight_color, {1.0, 1.0, 0.0, 1.0});

    // preview
    Bind("preview", "enable", config.preview_enable, false);
    Bind("preview", "icon_size", config.preview_icon_size, 64);
    Bind("preview", "padding", config.preview_padding,
         Lawnch::Config::Padding(10));
    Bind("preview", "background_color", config.preview_background_color,
         {0.0, 0.0, 0.0, 0.0});
    Bind("preview", "vertical_spacing", config.preview_vertical_spacing, 5);
    Bind("preview", "horizontal_spacing", config.preview_horizontal_spacing, 5);
    Bind("preview", "show", config.preview_show,
         std::vector<std::string>{"icon", "name"});
    Bind("preview", "name_font_family", config.preview_name_font_family,
         std::string("sans-serif"));
    Bind("preview", "name_font_size", config.preview_name_font_size, 14);
    Bind("preview", "name_font_weight", config.preview_name_font_weight,
         std::string("bold"));
    Bind("preview", "name_color", config.preview_name_color,
         {0.9, 0.9, 0.9, 1.0});
    Bind("preview", "comment_font_size", config.preview_comment_font_size, 12);
    Bind("preview", "comment_font_weight", config.preview_comment_font_weight,
         std::string("normal"));
    Bind("preview", "comment_color", config.preview_comment_color,
         {0.6, 0.6, 0.6, 1.0});

    Bind("preview", "hide_icon_if_fallback",
         config.preview_hide_icon_if_fallback, false);
    Bind("preview", "fallback_icon", config.preview_fallback_icon, false);
    Bind("preview", "preview_image_size", config.preview_preview_image_size,
         64);

    // results count
    Bind("results_count", "enable", config.results_count_enable, false);
    Bind("results_count", "format", config.results_count_format,
         std::string("{count} results"));
    Bind("results_count", "font_family", config.results_count_font_family,
         std::string("sans-serif"));
    Bind("results_count", "font_size", config.results_count_font_size, 11);
    Bind("results_count", "font_weight", config.results_count_font_weight,
         std::string("normal"));
    Bind("results_count", "text_color", config.results_count_text_color,
         {0.6, 0.6, 0.6, 1.0});
    Bind("results_count", "text_align", config.results_count_text_align,
         std::string("right"));
    Bind("results_count", "padding", config.results_count_padding,
         Lawnch::Config::Padding(2, 8, 2, 8));

    // clock
    Bind("clock", "enable", config.clock_enable, false);
    Bind("clock", "format", config.clock_format, std::string("%H:%M"));
    Bind("clock", "font_family", config.clock_font_family,
         std::string("sans-serif"));
    Bind("clock", "font_size", config.clock_font_size, 24);
    Bind("clock", "font_weight", config.clock_font_weight, std::string("bold"));
    Bind("clock", "text_color", config.clock_text_color, {0.9, 0.9, 0.9, 1.0});
    Bind("clock", "padding", config.clock_padding,
         Lawnch::Config::Padding(10, 20, 10, 20));
    Bind("clock", "text_align", config.clock_text_align, std::string("center"));
  }

  int HandleEntry(const std::string &secStr, const std::string &nameStr,
                  const std::string &valStr) {
    if (secStr == "plugins") {
      if (Lawnch::Config::parseBool(valStr)) {
        config.enabled_plugins.push_back(nameStr);
      }
      return 1;
    }

    // Check for "plugin/plugin_name" sections
    if (secStr.rfind("plugin/", 0) == 0) {
      std::string pluginName = secStr.substr(7);
      config.plugin_configs[pluginName + "." + nameStr] = valStr;
      return 1;
    }

    std::string key = secStr + "." + nameStr;
    auto it = bindings.find(key);
    if (it != bindings.end()) {
      it->second(valStr);
      return 1;
    }

    if (secStr == "bindings") {
      config.keybindings[nameStr] = valStr;
      return 1;
    }

    Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::DEBUG,
                        "Unknown config key: " + key);
    return 1;
  }
};

Manager &Manager::Instance() {
  static Manager instance;
  return instance;
}

Manager::Manager() : m_impl(std::make_unique<Impl>()) {}
Manager::~Manager() = default;

void Manager::Load(const std::string &path) {
  std::unique_lock lock(m_impl->config_mutex);
  m_impl->SetDefaultsAndBindings();

  auto handler = [](void *user, const char *section, const char *name,
                    const char *value) -> int {
    auto *impl = static_cast<Manager::Impl *>(user);

    if (!impl || !section || !name || !value)
      return 1;

    return impl->HandleEntry(section, name, value);
  };

  ini_parse(path.c_str(), handler, m_impl.get());
  std::string theme_name = m_impl->config.layout_theme;

  m_impl->SetDefaultsAndBindings();

  std::filesystem::path themes_dir;
  for (const auto &dir : Lawnch::Fs::get_data_dirs()) {
    std::filesystem::path candidate =
        std::filesystem::path(dir) / "lawnch" / "themes";
    if (std::filesystem::exists(candidate)) {
      themes_dir = candidate;
      break;
    }
  }

  if (themes_dir.empty()) {
    std::filesystem::path user_themes =
        Lawnch::Fs::get_data_home() / "lawnch" / "themes";
    if (std::filesystem::exists(user_themes)) {
      themes_dir = user_themes;
    }
  }

  if (themes_dir.empty()) {
    std::filesystem::path config_path(path);
    std::filesystem::path config_sibling = config_path.parent_path() / "themes";
    if (std::filesystem::exists(config_sibling)) {
      themes_dir = config_sibling;
    }
  }

  if (!themes_dir.empty() && !theme_name.empty()) {
    std::filesystem::path theme_file = themes_dir / (theme_name + ".ini");
    if (std::filesystem::exists(theme_file)) {
      ini_parse(theme_file.string().c_str(), handler, m_impl.get());
      Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::INFO,
                          "Loaded theme: " + theme_name);
    }
  }

  int error = ini_parse(path.c_str(), handler, m_impl.get());

  if (error < 0) {
    Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::WARNING,
                        "Unable to load config file '" + path +
                            "'. Using defaults.");
  } else {
    Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::INFO,
                        "Configuration loaded successfully from '" + path +
                            "'");
  }
}

void Manager::Merge(const std::string &path) {
  std::unique_lock lock(m_impl->config_mutex);

  auto handler = [](void *user, const char *section, const char *name,
                    const char *value) -> int {
    auto *impl = static_cast<Manager::Impl *>(user);
    if (!impl || !section || !name || !value)
      return 1;
    return impl->HandleEntry(section, name, value);
  };

  int error = ini_parse(path.c_str(), handler, m_impl.get());
  if (error < 0) {
    Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::WARNING,
                        "Unable to merge config file '" + path +
                            "'. Skipping.");
  } else {
    Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::INFO,
                        "Merged configuration from '" + path + "'");
  }
}

const Config &Manager::Get() const {
  std::shared_lock lock(m_impl->config_mutex);
  return m_impl->config;
}

} // namespace Lawnch::Core::Config
