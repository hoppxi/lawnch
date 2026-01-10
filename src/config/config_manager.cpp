#include "config_manager.hpp"
#include "../helpers/logger.hpp"
#include "../helpers/string.hpp"

extern "C" {
#include <ini.h>
}

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <sstream>

// Pimpl

struct ConfigManager::Impl {
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
        } else if constexpr (std::is_same_v<T, std::string>) {
          member = value;
        } else if constexpr (std::is_same_v<T, bool>) {
          member = Lawnch::Str::parseBool(value);
        } else if constexpr (std::is_same_v<T, Lawnch::Str::Color>) {
          member = Lawnch::Str::parseColor(value);
        } else {
          static_assert(!sizeof(T), "Unsupported type for Config Bind");
        }
      } catch (const std::exception &e) {
        std::stringstream ss;
        ss << "Error parsing [" << key << "] with value '" << value
           << "': " << e.what();
        Logger::log("Config", Logger::LogLevel::ERROR, ss.str());
      }
    };
  }

  void SetDefaultsAndBindings() {
    bindings.clear();
    config.enabled_plugins.clear();
    config.plugin_configs.clear();

    // Window
    Bind("window", "width", config.window_width, 500);
    Bind("window", "height", config.window_height, 350);
    Bind("window", "anchor", config.window_anchor, std::string("top, center"));
    Bind("window", "margin_top", config.window_margin_top, 50);
    Bind("window", "margin_bottom", config.window_margin_bottom, 0);
    Bind("window", "margin_left", config.window_margin_left, 0);
    Bind("window", "margin_right", config.window_margin_right, 0);
    Bind("window", "background_color", config.window_background_color,
         {0.1, 0.1, 0.1, 0.9});
    Bind("window", "border_color", config.window_border_color,
         {0.2, 0.2, 0.2, 1.0});
    Bind("window", "border_radius", config.window_border_radius, 10);
    Bind("window", "border_width", config.window_border_width, 2);

    // Input
    Bind("input", "font_family", config.input_font_family,
         std::string("sans-serif"));
    Bind("input", "font_size", config.input_font_size, 16);
    Bind("input", "font_weight", config.input_font_weight,
         std::string("normal"));
    Bind("input", "text_color", config.input_text_color, {1.0, 1.0, 1.0, 1.0});
    Bind("input", "placeholder_color", config.input_placeholder_color,
         {0.5, 0.5, 0.5, 1.0});
    Bind("input", "background_color", config.input_background_color,
         {0.2, 0.2, 0.2, 1.0});
    Bind("input", "caret_color", config.input_caret_color,
         {1.0, 1.0, 1.0, 1.0});
    Bind("input", "border_radius", config.input_border_radius, 5);
    Bind("input", "border_width", config.input_border_width, 1);
    Bind("input", "border_color", config.input_border_color,
         {0.3, 0.3, 0.3, 1.0});
    Bind("input", "padding_top", config.input_padding_top, 10);
    Bind("input", "padding_bottom", config.input_padding_bottom, 10);
    Bind("input", "padding_left", config.input_padding_left, 15);
    Bind("input", "padding_right", config.input_padding_right, 15);
    Bind("input", "horizontal_align", config.input_horizontal_align,
         std::string("left"));

    // Results
    Bind("results", "font_family", config.results_font_family,
         std::string("sans-serif"));
    Bind("results", "font_size", config.results_font_size, 14);
    Bind("results", "padding_top", config.results_padding_top, 0);
    Bind("results", "padding_bottom", config.results_padding_bottom, 0);
    Bind("results", "padding_left", config.results_padding_left, 0);
    Bind("results", "padding_right", config.results_padding_right, 0);
    Bind("results", "item_spacing", config.results_item_spacing, 5);
    Bind("results", "item_padding", config.results_item_padding, 10);
    Bind("results", "default_border_radius",
         config.results_default_border_radius, 8);
    Bind("results", "default_border_width", config.results_default_border_width,
         1);
    Bind("results", "default_border_color", config.results_default_border_color,
         {0, 0, 0, 0.0});
    Bind("results", "selected_border_radius",
         config.results_selected_border_radius, 8);
    Bind("results", "selected_border_width",
         config.results_selected_border_width, 1);
    Bind("results", "selected_border_color",
         config.results_selected_border_color, {0.42, 0.44, 0.52, 1.0});
    Bind("results", "default_text_color", config.results_default_text_color,
         {0.8, 0.8, 0.8, 1.0});
    Bind("results", "default_background_color",
         config.results_default_background_color, {0.1, 0.1, 0.1, 0.0});
    Bind("results", "selected_text_color", config.results_selected_text_color,
         {1.0, 1.0, 1.0, 1.0});
    Bind("results", "selected_background_color",
         config.results_selected_background_color, {0.3, 0.3, 0.3, 1.0});
    Bind("results", "enable_comment", config.results_enable_comment, true);
    Bind("results", "comment_font_size", config.results_comment_font_size, 10);
    Bind("results", "comment_color", config.results_comment_color,
         {0.6, 0.6, 0.6, 1.0});
    Bind("results", "selected_comment_color",
         config.results_selected_comment_color, {0.7, 0.7, 0.7, 1.0});
    Bind("results", "enable_icon", config.results_enable_icon, true);
    Bind("results", "icon_size", config.results_icon_size, 24);
  }

  int HandleEntry(const std::string &secStr, const std::string &nameStr,
                  const std::string &valStr) {
    if (secStr == "plugins") {
      if (Lawnch::Str::parseBool(valStr)) {
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
      Logger::log("Config", Logger::LogLevel::DEBUG,
                  "Unknown config key: " + key);
    }
    return 1;
  }
};

ConfigManager &ConfigManager::Instance() {
  static ConfigManager instance;
  return instance;
}

ConfigManager::ConfigManager() : m_impl(std::make_unique<Impl>()) {}
ConfigManager::~ConfigManager() = default;

void ConfigManager::Load(const std::string &path) {
  std::unique_lock lock(m_impl->config_mutex);
  m_impl->SetDefaultsAndBindings();

  auto handler = [](void *user, const char *section, const char *name,
                    const char *value) -> int {
    auto *impl = static_cast<ConfigManager::Impl *>(user);

    if (!impl || !section || !name || !value)
      return 1;

    return impl->HandleEntry(section, name, value);
  };

  int error = ini_parse(path.c_str(), handler, m_impl.get());

  if (error < 0) {
    std::stringstream ss;
    ss << "Unable to load config file '" << path << "'. Using defaults.";
    Logger::log("Config", Logger::LogLevel::WARNING, ss.str());
  } else {
    std::stringstream ss;
    ss << "Configuration loaded successfully from '" << path << "'";
    Logger::log("Config", Logger::LogLevel::INFO, ss.str());
  }
}

const Config &ConfigManager::Get() const {
  // This allows multiple threads to read config simultaneously,
  // but blocks if a Load (Write) is happening.
  std::shared_lock lock(m_impl->config_mutex);
  return m_impl->config;
}
