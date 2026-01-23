#pragma once

#include "../../helpers/config_parse.hpp"
#include <map>
#include <string>
#include <vector>

namespace Lawnch::Core::Config {

using Lawnch::Config::Color;
using Lawnch::Config::Padding;

struct Config {
  // general
  std::string general_icon_theme;
  std::string general_terminal;
  std::string general_terminal_exec_flag;
  std::string general_editor;
  int general_history_max_size;
  bool general_clear_on_exit;
  std::string general_locale;

  // launch
  std::string launch_app_cmd;
  std::string launch_terminal_app_cmd;
  std::string launch_prefix;
  std::string launch_context;
  std::string launch_start_with;

  // layout
  std::string layout_theme;
  std::vector<std::string> layout_order;
  std::string layout_orientation;
  std::string layout_preview_position;
  int layout_preview_width_percent;

  // window
  int window_width;
  int window_height;
  std::string window_anchor;
  Padding window_margin;
  int window_border_radius;
  int window_border_width;
  Color window_background_color;
  Color window_border_color;
  std::string window_layer;
  bool window_exclusive_zone;
  bool window_ignore_exclusive_zones;
  std::string window_keyboard_interactivity;

  // input
  bool input_visible;
  std::string input_font_family;
  int input_font_size;
  std::string input_font_weight;
  Color input_text_color;
  Color input_placeholder_color;
  std::string input_placeholder_text;
  Color input_background_color;
  Color input_caret_color;
  double input_caret_width;
  bool input_cursor_blink;
  int input_cursor_blink_rate;
  Color input_selection_color;
  Color input_selection_text_color;
  Padding input_padding;
  int input_border_radius;
  int input_border_width;
  Color input_border_color;
  std::string input_horizontal_align;

  // input prompt
  bool input_prompt_enable;
  std::string input_prompt_content;
  std::string input_prompt_position;
  std::string input_prompt_font_family;
  int input_prompt_font_size;
  std::string input_prompt_font_weight;
  Color input_prompt_text_color;
  Color input_prompt_background_color;
  int input_prompt_border_radius;
  int input_prompt_border_width;
  Color input_prompt_border_color;
  Padding input_prompt_padding;

  // results
  Padding results_margin;
  Padding results_padding;
  int results_spacing;
  Color results_background_color;
  Color results_border_color;
  int results_border_width;
  int results_border_radius;
  bool results_enable_scrollbar;
  int results_scrollbar_width;
  int results_scrollbar_padding;
  int results_scrollbar_radius;
  Color results_scrollbar_color;
  Color results_scrollbar_bg_color;
  std::string results_scroll_mode;
  bool results_reverse;

  // result item
  std::string result_item_font_family;
  int result_item_font_size;
  std::string result_item_font_weight;
  std::string result_item_text_align;
  bool result_item_enable_comment;
  int result_item_comment_font_size;
  std::string result_item_comment_font_weight;
  Color result_item_comment_color;
  Color result_item_selected_comment_color;
  bool result_item_enable_icon;
  int result_item_icon_size;
  int result_item_icon_padding_right;
  Padding result_item_padding;
  int result_item_border_radius;
  int result_item_border_width;
  Color result_item_border_color;
  Color result_item_background_color;
  Color result_item_text_color;
  int result_item_selected_border_radius;
  int result_item_selected_border_width;
  Color result_item_selected_border_color;
  Color result_item_selected_background_color;
  Color result_item_selected_text_color;
  bool result_item_enable_highlight;
  Color result_item_highlight_color;
  std::string result_item_highlight_font_weight;
  Color result_item_selected_highlight_color;

  // preview
  bool preview_enable;
  int preview_icon_size;
  int preview_preview_image_size;
  bool preview_hide_icon_if_fallback;
  bool preview_fallback_icon;
  Padding preview_padding;
  Color preview_background_color;
  int preview_vertical_spacing;
  int preview_horizontal_spacing;
  std::vector<std::string> preview_show;
  std::string preview_name_font_family;
  int preview_name_font_size;
  std::string preview_name_font_weight;
  Color preview_name_color;
  int preview_comment_font_size;
  std::string preview_comment_font_weight;
  Color preview_comment_color;

  // results count
  bool results_count_enable;
  std::string results_count_format;
  std::string results_count_font_family;
  int results_count_font_size;
  std::string results_count_font_weight;
  Color results_count_text_color;
  std::string results_count_text_align;
  Padding results_count_padding;

  // clock
  bool clock_enable;
  std::string clock_format;
  std::string clock_font_family;
  int clock_font_size;
  std::string clock_font_weight;
  Color clock_text_color;
  Padding clock_padding;
  std::string clock_text_align;

  // plugins
  std::vector<std::string> enabled_plugins;
  std::map<std::string, std::string> plugin_configs;

  // bindings
  std::string bindings_preset;
  std::map<std::string, std::string> keybindings;
};

} // namespace Lawnch::Core::Config
