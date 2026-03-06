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
  std::string general_terminal_flag;
  std::string general_editor;
  int general_history_max_size;
  bool general_history;
  std::string general_locale;

  // launch
  std::string launch_command;
  std::string launch_terminal_command;
  std::string launch_wrapper;
  std::string launch_scope;
  std::string launch_initial;

  // appearance
  std::string appearance_theme;
  std::string appearance_preset;
  std::vector<std::string> layout_order;
  std::string layout_preview_side;
  int layout_preview_ratio;

  // window
  int window_width;
  int window_height;
  std::string window_anchor;
  Padding window_margin;
  Padding window_padding;
  int window_border_radius;
  int window_border_width;
  Color window_background;
  Color window_border_color;
  bool window_exclusive;
  bool window_ignore_exclusive;
  std::string window_keyboard;

  // input
  bool input_visible;
  std::string input_font_family;
  int input_font_size;
  std::string input_font_weight;
  Color input_text;
  Color input_placeholder_color;
  std::string input_placeholder_text;
  Color input_background;
  Color input_caret_color;
  double input_caret_width;
  Color input_selection_color;
  Color input_selection_text;
  Padding input_padding;
  Padding input_margin;
  int input_border_radius;
  int input_border_width;
  Color input_border_color;
  std::string input_align;

  // input prompt
  bool input_prompt_enable;
  std::string input_prompt_text;
  std::string input_prompt_side;
  std::string input_prompt_font_family;
  int input_prompt_font_size;
  std::string input_prompt_font_weight;
  Color input_prompt_color;
  Color input_prompt_background;
  int input_prompt_border_radius;
  int input_prompt_border_width;
  Color input_prompt_border_color;
  Padding input_prompt_padding;
  Padding input_prompt_margin;

  // results
  Padding results_margin;
  Padding results_padding;
  int results_gap;
  Color results_background;
  Color results_border_color;
  int results_border_width;
  int results_border_radius;
  bool results_scrollbar_enable;
  int results_scrollbar_width;
  int results_scrollbar_padding;
  int results_scrollbar_radius;
  Color results_scrollbar_thumb;
  Color results_scrollbar_track;
  std::string results_scroll;
  bool results_reverse;
  int results_limit;
  bool results_show_help;

  // result item
  std::string result_item_font_family;
  int result_item_font_size;
  std::string result_item_font_weight;
  std::string result_item_align;
  bool result_item_comment_enable;
  int result_item_comment_font_size;
  std::string result_item_comment_font_weight;
  Color result_item_comment_color;
  Color result_item_selected_comment;
  bool result_item_icon_show;
  int result_item_icon_size;
  int result_item_icon_gap;
  Padding result_item_padding;
  Padding result_item_margin;
  int result_item_border_radius;
  int result_item_border_width;
  Color result_item_border_color;
  Color result_item_background;
  Color result_item_text;
  int result_item_selected_border_radius;
  int result_item_selected_border_width;
  Color result_item_selected_border_color;
  Color result_item_selected_background;
  Color result_item_selected_text;
  bool result_item_highlight_enable;
  Color result_item_highlight_color;
  std::string result_item_highlight_font_weight;
  Color result_item_selected_highlight;

  // preview
  bool preview_enable;
  int preview_icon_size;
  int preview_image_size;
  bool preview_icon_hide_on_fallback;
  bool preview_icon_fallback;
  Padding preview_padding;
  Padding preview_margin;
  Color preview_background;
  int preview_gap_v;
  int preview_gap_h;
  std::vector<std::string> preview_composition;
  std::string preview_title_font_family;
  int preview_title_font_size;
  std::string preview_title_font_weight;
  Color preview_title_color;
  int preview_description_font_size;
  std::string preview_description_font_weight;
  Color preview_description_color;

  // results count
  bool results_count_enable;
  std::string results_count_format;
  std::string results_count_font_family;
  int results_count_font_size;
  std::string results_count_font_weight;
  Color results_count_text;
  std::string results_count_align;
  Padding results_count_padding;
  Padding results_count_margin;

  // clock
  bool clock_enable;
  std::string clock_format;
  std::string clock_font_family;
  int clock_font_size;
  std::string clock_font_weight;
  Color clock_text;
  Padding clock_padding;
  Padding clock_margin;
  std::string clock_align;

  // providers
  // apps provider
  std::string providers_apps_command;
  bool providers_apps_uwsm;
  std::string providers_apps_uwsm_prefix;
  bool providers_apps_history;

  // bins provider
  std::string providers_bins_exec;
  bool providers_bins_history;
  bool providers_bins_terminal_exec;

  // plugins
  std::vector<std::string> enabled_plugins;
  std::map<std::string, std::string> plugin_configs;

  // keybindings
  std::string keybindings_inherit;
  std::map<std::string, std::string> keybindings;

  // theme
  std::map<std::string, Color> theme_colors;
};

} // namespace Lawnch::Core::Config
