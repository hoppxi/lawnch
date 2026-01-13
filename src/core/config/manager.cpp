#include "manager.hpp"
#include "../../helpers/logger.hpp"
#include "../../helpers/string.hpp"

extern "C" {
#include <ini.h>
}

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
    Bind("results", "scrollbar_enable", config.results_scrollbar_enable, true);
    Bind("results", "scrollbar_width", config.results_scrollbar_width, 4);
    Bind("results", "scrollbar_padding", config.results_scrollbar_padding, 2);
    Bind("results", "scrollbar_radius", config.results_scrollbar_radius, 2);
    Bind("results", "scrollbar_color", config.results_scrollbar_color,
         {0.8, 0.8, 0.8, 0.5});
    Bind("results", "scrollbar_bg_color", config.results_scrollbar_bg_color,
         {0.2, 0.2, 0.2, 0.2});
    Bind("results", "scroll_mode", config.results_scroll_mode,
         std::string("follow"));

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
    Bind("result_item", "default_border_radius",
         config.result_item_default_border_radius, 6);
    Bind("result_item", "default_border_width",
         config.result_item_default_border_width, 0);
    Bind("result_item", "default_border_color",
         config.result_item_default_border_color, {1.0, 1.0, 1.0, 0.1});
    Bind("result_item", "default_background_color",
         config.result_item_default_background_color, {0.0, 0.0, 0.0, 0.0});
    Bind("result_item", "default_text_color",
         config.result_item_default_text_color, {0.9, 0.9, 0.9, 1.0});
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
    Bind("result_item", "highlight_enable", config.result_item_highlight_enable,
         false);
    Bind("result_item", "highlight_color", config.result_item_highlight_color,
         {1.0, 0.78, 0.0, 1.0});
    Bind("result_item", "highlight_font_weight",
         config.result_item_highlight_font_weight, std::string("bold"));
    Bind("result_item", "selected_highlight_color",
         config.result_item_selected_highlight_color, {1.0, 1.0, 0.0, 1.0});

    // preview icon
    Bind("preview", "enable", config.preview_enable, false);
    Bind("preview", "icon_size", config.preview_icon_size, 64);
    Bind("preview", "padding", config.preview_padding,
         Lawnch::Config::Padding(10));
    Bind("preview", "background_color", config.preview_background_color,
         {0.0, 0.0, 0.0, 0.0});

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
    } else {
      Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::DEBUG,
                          "Unknown config key: " + key);
    }
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

  int error = ini_parse(path.c_str(), handler, m_impl.get());

  if (error < 0) {
    std::stringstream ss;
    ss << "Unable to load config file '" << path << "'. Using defaults.";
    Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::WARNING, ss.str());
  } else {
    std::stringstream ss;
    ss << "Configuration loaded successfully from '" << path << "'";
    Lawnch::Logger::log("Config", Lawnch::Logger::LogLevel::INFO, ss.str());
  }
}

const Config &Manager::Get() const {
  std::shared_lock lock(m_impl->config_mutex);
  return m_impl->config;
}

} // namespace Lawnch::Core::Config
