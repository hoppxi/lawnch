#pragma once

#include "../../helpers/config_parse.hpp"
#include <map>
#include <string>
#include <vector>

namespace Lawnch::Core::Config {

using Lawnch::Config::Color;
using Lawnch::Config::Padding;

struct Modifier {
  std::string trigger;
  std::string expanded;
  std::string type;
  std::vector<std::string> scope;
};

struct Config {
  // general
  std::string general_icon_theme;
  std::vector<std::string> general_icon_dirs;
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
  std::string layout_direction;
  std::string layout_preview_side;
  int layout_preview_ratio;

  // window
  int window_width;
  int window_height;
  std::vector<std::string> window_anchor;
  Padding window_margin;
  Padding window_padding;
  int window_border_radius;
  int window_border_width;
  Color window_background;
  Color window_border_color;
  bool window_exclusive;
  bool window_ignore_exclusive;
  std::string window_keyboard;
  std::string window_font_family;
  std::string window_font_path;
  int window_font_size;
  std::string window_font_weight;

  // input
  bool input_visible;
  std::string input_font_family;
  std::string input_font_path;
  int input_font_size;
  std::string input_font_weight;
  Color input_color;
  Color input_placeholder_color;
  std::string input_placeholder_text;
  Color input_background;
  Color input_caret_color;
  double input_caret_width;
  Color input_selection_background;
  Color input_selection_color;
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
  std::string input_prompt_font_path;
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
  std::string results_direction;
  int results_column;
  Padding results_margin;
  Padding results_padding;
  int results_gap;
  Color results_background;
  Color results_border_color;
  int results_border_width;
  int results_border_radius;
  bool results_scrollbar_enable;
  Color results_scrollbar_thumb_color;
  int results_scrollbar_thumb_radius;
  int results_scrollbar_track_width;
  Color results_scrollbar_track_color;
  Padding results_scrollbar_track_padding;
  Padding results_scrollbar_track_margin;
  int results_scrollbar_track_radius;
  std::string results_scroll;
  bool results_reverse;
  int results_limit;
  bool results_show_help;

  // result item
  std::string result_item_direction;
  bool result_item_icon_enable;
  int result_item_icon_size;
  int result_item_icon_gap;
  bool result_item_comment_enable;
  bool result_item_highlight_enable;

  // result item default
  std::string result_item_default_font_family;
  std::string result_item_default_font_path;
  int result_item_default_font_size;
  std::string result_item_default_font_weight;
  std::string result_item_default_align;
  Color result_item_default_color;
  Color result_item_default_background;
  int result_item_default_border_radius;
  int result_item_default_border_width;
  Color result_item_default_border_color;
  Padding result_item_default_padding;
  Padding result_item_default_margin;
  std::string result_item_default_comment_font_family;
  std::string result_item_default_comment_font_path;
  int result_item_default_comment_font_size;
  std::string result_item_default_comment_font_weight;
  Color result_item_default_comment_color;
  std::string result_item_default_highlight_font_family;
  std::string result_item_default_highlight_font_path;
  int result_item_default_highlight_font_size;
  std::string result_item_default_highlight_font_weight;
  Color result_item_default_highlight_color;

  // result item selected
  std::string result_item_selected_font_family;
  std::string result_item_selected_font_path;
  int result_item_selected_font_size;
  std::string result_item_selected_font_weight;
  std::string result_item_selected_align;
  Color result_item_selected_color;
  Color result_item_selected_background;
  int result_item_selected_border_radius;
  int result_item_selected_border_width;
  Color result_item_selected_border_color;
  Padding result_item_selected_padding;
  Padding result_item_selected_margin;
  std::string result_item_selected_comment_font_family;
  std::string result_item_selected_comment_font_path;
  int result_item_selected_comment_font_size;
  std::string result_item_selected_comment_font_weight;
  Color result_item_selected_comment_color;
  std::string result_item_selected_highlight_font_family;
  std::string result_item_selected_highlight_font_path;
  int result_item_selected_highlight_font_size;
  std::string result_item_selected_highlight_font_weight;
  Color result_item_selected_highlight_color;

  // preview
  bool preview_enable;
  std::string preview_direction;
  int preview_icon_size;
  int preview_image_size;
  bool preview_icon_fallback;
  Padding preview_padding;
  Padding preview_margin;
  Color preview_background;
  int preview_gap_v;
  int preview_gap_h;
  std::string preview_name_font_family;
  std::string preview_name_font_path;
  int preview_name_font_size;
  std::string preview_name_font_weight;
  Color preview_name_color;
  std::string preview_comment_font_family;
  std::string preview_comment_font_path;
  int preview_comment_font_size;
  std::string preview_comment_font_weight;
  Color preview_comment_color;

  // results count
  bool results_count_enable;
  std::string results_count_format;
  std::string results_count_font_family;
  std::string results_count_font_path;
  int results_count_font_size;
  std::string results_count_font_weight;
  Color results_count_color;
  std::string results_count_align;
  Padding results_count_padding;
  Padding results_count_margin;

  // clock
  bool clock_enable;
  std::string clock_format;
  std::string clock_font_family;
  std::string clock_font_path;
  int clock_font_size;
  std::string clock_font_weight;
  Color clock_color;
  Padding clock_padding;
  Padding clock_margin;
  std::string clock_align;

  // breadcrumbs
  bool breadcrumbs_enable;
  std::string breadcrumbs_separator;
  std::string breadcrumbs_font_family;
  std::string breadcrumbs_font_path;
  int breadcrumbs_font_size;
  std::string breadcrumbs_font_weight;
  Color breadcrumbs_color;
  std::string breadcrumbs_align;
  Padding breadcrumbs_padding;
  Padding breadcrumbs_margin;

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

  // modifiers (abbr/alias)
  std::vector<Modifier> modifiers;
};

} // namespace Lawnch::Core::Config
