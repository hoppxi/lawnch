#pragma once

#include <map>
#include <string>
#include <vector>

struct Color {
  double r, g, b, a;
};

struct Config {
  // Window
  int window_width;
  int window_height;
  int window_margin_top;
  int window_margin_bottom;
  int window_margin_left;
  int window_margin_right;
  Color window_background_color;
  Color window_border_color;
  int window_border_radius;
  int window_border_width;

  // Input
  std::string input_font_family;
  int input_font_size;
  std::string input_font_weight;
  Color input_text_color;
  Color input_placeholder_color;
  Color input_background_color;
  Color input_caret_color;
  int input_border_radius;
  int input_border_width;
  Color input_border_color;
  int input_padding_top;
  int input_padding_bottom;
  int input_padding_left;
  int input_padding_right;
  std::string input_horizontal_align;

  // Results
  std::string results_font_family;
  int results_font_size;
  int results_item_spacing;
  int results_item_padding;
  Color results_default_text_color;
  Color results_default_background_color;
  Color results_selected_text_color;
  Color results_selected_background_color;
  // New Comment Config
  int results_comment_font_size;
  Color results_comment_color;
  Color results_selected_comment_color;
  bool results_enable_icon;
  bool results_enable_comment;

  int results_icon_size;
  int results_item_border_radius;

  // Plugins
  std::vector<std::string> enabled_plugins;
  std::map<std::string, std::string> plugin_configs;
};

class ConfigManager {
public:
  static Config &get();
  static void load(const std::string &path);

private:
  ConfigManager() = default;
  static Config instance;
};