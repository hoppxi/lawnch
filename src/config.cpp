#include "config.hpp"
#include "utils.hpp"

extern "C" {
#include <ini.h>
}

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string_view>

Config ConfigManager::instance;

static Color parse_color(const std::string &s) {
  Color c = {0, 0, 0, 1};
  if (s.rfind("rgba(", 0) == 0) {
    std::sscanf(s.c_str(), "rgba(%lf, %lf, %lf, %lf)", &c.r, &c.g, &c.b, &c.a);
    c.r /= 255.0;
    c.g /= 255.0;
    c.b /= 255.0;
  }
  return c;
}

static bool parse_bool(const std::string &s) {
  std::string v = s;
  std::transform(v.begin(), v.end(), v.begin(), ::tolower);
  return (v == "true" || v == "1" || v == "yes" || v == "on");
}

static int handler(void *user, const char *section, const char *name,
                   const char *value) {
  Config *pconfig = (Config *)user;

  auto to_sv = [](const char *s) { return std::string_view(s); };

#define MATCH(s, n) (to_sv(section) == to_sv(s) && to_sv(name) == to_sv(n))

  try {
    if (MATCH("window", "width"))
      pconfig->window_width = std::stoi(value);
    else if (MATCH("window", "height"))
      pconfig->window_height = std::stoi(value);
    else if (MATCH("window", "anchor"))
      pconfig->window_anchor = value;
    else if (MATCH("window", "margin_top"))
      pconfig->window_margin_top = std::stoi(value);
    else if (MATCH("window", "margin_bottom"))
      pconfig->window_margin_bottom = std::stoi(value);
    else if (MATCH("window", "margin_left"))
      pconfig->window_margin_left = std::stoi(value);
    else if (MATCH("window", "margin_right"))
      pconfig->window_margin_right = std::stoi(value);
    else if (MATCH("window", "background_color"))
      pconfig->window_background_color = parse_color(value);
    else if (MATCH("window", "border_color"))
      pconfig->window_border_color = parse_color(value);
    else if (MATCH("window", "border_radius"))
      pconfig->window_border_radius = std::stoi(value);
    else if (MATCH("window", "border_width"))
      pconfig->window_border_width = std::stoi(value);

    else if (MATCH("input", "font_family"))
      pconfig->input_font_family = value;
    else if (MATCH("input", "font_size"))
      pconfig->input_font_size = std::stoi(value);
    else if (MATCH("input", "font_weight"))
      pconfig->input_font_weight = value;
    else if (MATCH("input", "text_color"))
      pconfig->input_text_color = parse_color(value);
    else if (MATCH("input", "placeholder_color"))
      pconfig->input_placeholder_color = parse_color(value);
    else if (MATCH("input", "background_color"))
      pconfig->input_background_color = parse_color(value);
    else if (MATCH("input", "caret_color"))
      pconfig->input_caret_color = parse_color(value);
    else if (MATCH("input", "border_radius"))
      pconfig->input_border_radius = std::stoi(value);
    else if (MATCH("input", "border_width"))
      pconfig->input_border_width = std::stoi(value);
    else if (MATCH("input", "border_color"))
      pconfig->input_border_color = parse_color(value);
    else if (MATCH("input", "padding_top"))
      pconfig->input_padding_top = std::stoi(value);
    else if (MATCH("input", "padding_bottom"))
      pconfig->input_padding_bottom = std::stoi(value);
    else if (MATCH("input", "padding_left"))
      pconfig->input_padding_left = std::stoi(value);
    else if (MATCH("input", "padding_right"))
      pconfig->input_padding_right = std::stoi(value);
    else if (MATCH("input", "horizontal_align"))
      pconfig->input_horizontal_align = value;

    else if (MATCH("results", "font_family"))
      pconfig->results_font_family = value;
    else if (MATCH("results", "font_size"))
      pconfig->results_font_size = std::stoi(value);
    else if (MATCH("results", "padding_top"))
      pconfig->results_padding_top = std::stoi(value);
    else if (MATCH("results", "padding_bottom"))
      pconfig->results_padding_bottom = std::stoi(value);
    else if (MATCH("results", "padding_left"))
      pconfig->results_padding_left = std::stoi(value);
    else if (MATCH("results", "padding_right"))
      pconfig->results_padding_right = std::stoi(value);
    else if (MATCH("results", "item_spacing"))
      pconfig->results_item_spacing = std::stoi(value);
    else if (MATCH("results", "item_padding"))
      pconfig->results_item_padding = std::stoi(value);
    else if (MATCH("results", "default_border_radius"))
      pconfig->results_default_border_radius = std::stoi(value);
    else if (MATCH("results", "default_border_width"))
      pconfig->results_default_border_width = std::stoi(value);
    else if (MATCH("results", "default_border_color"))
      pconfig->results_default_border_color = parse_color(value);
    else if (MATCH("results", "selected_border_radius"))
      pconfig->results_selected_border_radius = std::stoi(value);
    else if (MATCH("results", "selected_border_width"))
      pconfig->results_selected_border_width = std::stoi(value);
    else if (MATCH("results", "selected_border_color"))
      pconfig->results_selected_border_color = parse_color(value);
    else if (MATCH("results", "default_text_color"))
      pconfig->results_default_text_color = parse_color(value);
    else if (MATCH("results", "default_background_color"))
      pconfig->results_default_background_color = parse_color(value);
    else if (MATCH("results", "selected_text_color"))
      pconfig->results_selected_text_color = parse_color(value);
    else if (MATCH("results", "selected_background_color"))
      pconfig->results_selected_background_color = parse_color(value);
    else if (MATCH("results", "enable_comment"))
      pconfig->results_enable_comment = parse_bool(value);
    else if (MATCH("results", "comment_font_size"))
      pconfig->results_comment_font_size = std::stoi(value);
    else if (MATCH("results", "comment_color"))
      pconfig->results_comment_color = parse_color(value);
    else if (MATCH("results", "selected_comment_color"))
      pconfig->results_selected_comment_color = parse_color(value);
    else if (MATCH("results", "enable_icon"))
      pconfig->results_enable_icon = parse_bool(value);
    else if (MATCH("results", "icon_size"))
      pconfig->results_icon_size = std::stoi(value);

    else if (std::string_view(section).rfind("plugin/", 0) == 0) {
      std::string_view section_sv(section);
      std::string plugin_name = std::string(section_sv.substr(7));
      pconfig->plugin_configs[plugin_name + "." + name] = value;
    } else if (to_sv(section) == to_sv("plugins")) {
      if (parse_bool(value)) {
        pconfig->enabled_plugins.push_back(name);
      }
    }

  } catch (const std::invalid_argument &e) {
    std::stringstream ss;
    ss << "Invalid value for [" << section << "] " << name << ": " << value;
    Utils::log("Config", Utils::LogLevel::ERROR, ss.str());
    return 0; // Invalid value
  }

  return 1;
}

void ConfigManager::load(const std::string &path) {
  // Default values
  instance = {.window_width = 500,
              .window_height = 350,
              .window_anchor = "top, center",
              .window_margin_top = 50,
              .window_margin_bottom = 0,
              .window_margin_left = 0,
              .window_margin_right = 0,
              .window_background_color = {0.1, 0.1, 0.1, 0.9},
              .window_border_color = {0.2, 0.2, 0.2, 1.0},
              .window_border_radius = 10,
              .window_border_width = 2,

              .input_font_family = "sans-serif",
              .input_font_size = 16,
              .input_font_weight = "normal",
              .input_text_color = {1.0, 1.0, 1.0, 1.0},
              .input_placeholder_color = {0.5, 0.5, 0.5, 1.0},
              .input_background_color = {0.2, 0.2, 0.2, 1.0},
              .input_caret_color = {1.0, 1.0, 1.0, 1.0},
              .input_border_radius = 5,
              .input_border_width = 1,
              .input_border_color = {0.3, 0.3, 0.3, 1.0},
              .input_padding_top = 10,
              .input_padding_bottom = 10,
              .input_padding_left = 15,
              .input_padding_right = 15,
              .input_horizontal_align = "left",

              .results_font_family = "sans-serif",
              .results_font_size = 14,

              .results_padding_top = 0,
              .results_padding_bottom = 0,
              .results_padding_left = 0,
              .results_padding_right = 0,

              .results_item_spacing = 5,
              .results_item_padding = 10,

              // .results_item_border_radius = 5,
              .results_default_border_radius = 8,
              .results_default_border_width = 1,
              .results_default_border_color = {0, 0, 0, 0.0},
              .results_selected_border_radius = 8,
              .results_selected_border_width = 1,
              .results_selected_border_color = {0.42, 0.44, 0.52, 1.0},

              .results_default_text_color = {0.8, 0.8, 0.8, 1.0},
              .results_default_background_color = {0.1, 0.1, 0.1, 0.0},
              .results_selected_text_color = {1.0, 1.0, 1.0, 1.0},
              .results_selected_background_color = {0.3, 0.3, 0.3, 1.0},

              .results_enable_comment = true,
              .results_comment_font_size = 10,
              .results_comment_color = {0.6, 0.6, 0.6, 1.0},
              .results_selected_comment_color = {0.7, 0.7, 0.7, 1.0},

              .results_enable_icon = true,
              .results_icon_size = 24};

  if (ini_parse(path.c_str(), handler, &instance) < 0) {
    std::stringstream ss;
    ss << "Can't load '" << path << "', using default values.";
    Utils::log("Config", Utils::LogLevel::WARNING, ss.str());
  }
}

Config &ConfigManager::get() { return instance; }