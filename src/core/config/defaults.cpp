#include "manager_impl.hpp"

namespace Lawnch::Core::Config {

void Manager::Impl::SetDefaults() {
  config = Config{};

  // general
  config.general_icon_theme = "Papirus-Dark";
  config.general_terminal = "auto";
  config.general_terminal_flag = "-e";
  config.general_editor = "auto";
  config.general_history_max_size = 100;
  config.general_history = true;
  config.general_locale = "";

  // launch
  config.launch_command = "{}";
  config.launch_terminal_command = "{terminal} {terminal_exec_flag} {}";
  config.launch_wrapper = "";
  config.launch_scope = "";
  config.launch_initial = ":apps";

  // appearance
  config.appearance_theme = "";
  config.appearance_preset = "";
  config.layout_order = {"input", "results_count", "results", "preview"};
  config.layout_preview_side = "bottom";
  config.layout_preview_ratio = 30;

  // window
  config.window_width = 400;
  config.window_height = 370;
  config.window_anchor = "top, center";
  config.window_margin = Padding(50, 0, 0, 0);
  config.window_padding = Padding(0);
  config.window_background = {0.1, 0.1, 0.1, 0.9};
  config.window_border_color = {0.2, 0.2, 0.2, 1.0};
  config.window_border_radius = 10;
  config.window_border_width = 2;
  config.window_exclusive = false;
  config.window_ignore_exclusive = false;
  config.window_keyboard = "exclusive";

  // input
  config.input_visible = true;
  config.input_font_family = "sans-serif";
  config.input_font_size = 24;
  config.input_font_weight = "normal";
  config.input_text = {1, 1, 1, 1};
  config.input_placeholder_text = "Search...";
  config.input_placeholder_color = {0.6, 0.6, 0.6, 1};
  config.input_background = {0.15, 0.15, 0.15, 1};
  config.input_caret_color = {1, 1, 1, 1};
  config.input_caret_width = 2.0;
  config.input_selection_color = {0.2, 0.4, 0.8, 0.5};
  config.input_selection_text = {1, 1, 1, 1};
  config.input_padding = Padding(12, 16, 12, 16);
  config.input_margin = Padding(0);
  config.input_border_radius = 8;
  config.input_border_width = 2;
  config.input_border_color = {0.3, 0.3, 0.3, 1};
  config.input_align = "left";

  // input prompt
  config.input_prompt_enable = false;
  config.input_prompt_text = "";
  config.input_prompt_side = "left";
  config.input_prompt_font_family = "sans-serif";
  config.input_prompt_font_size = 14;
  config.input_prompt_font_weight = "normal";
  config.input_prompt_color = {0.9, 0.9, 0.9, 1};
  config.input_prompt_background = {0, 0, 0, 0};
  config.input_prompt_border_radius = 0;
  config.input_prompt_border_width = 0;
  config.input_prompt_border_color = {0, 0, 0, 0};
  config.input_prompt_padding = Padding(0, 5, 0, 5);
  config.input_prompt_margin = Padding(0);

  // results
  config.results_margin = Padding(10, 0, 0, 0);
  config.results_padding = Padding(0);
  config.results_gap = 5;
  config.results_background = {0, 0, 0, 0};
  config.results_border_color = {0, 0, 0, 0};
  config.results_border_width = 0;
  config.results_border_radius = 0;
  config.results_scrollbar_enable = true;
  config.results_scrollbar_width = 4;
  config.results_scrollbar_padding = 2;
  config.results_scrollbar_radius = 2;
  config.results_scrollbar_thumb = {0.8, 0.8, 0.8, 0.5};
  config.results_scrollbar_track = {0.2, 0.2, 0.2, 0.2};
  config.results_scroll = "follow";
  config.results_reverse = false;
  config.results_limit = 50;
  config.results_show_help = false;

  // result item
  config.result_item_font_family = "sans-serif";
  config.result_item_font_size = 16;
  config.result_item_font_weight = "normal";
  config.result_item_align = "left";
  config.result_item_comment_enable = true;
  config.result_item_comment_font_size = 10;
  config.result_item_comment_font_weight = "normal";
  config.result_item_comment_color = {0.6, 0.6, 0.6, 1};
  config.result_item_selected_comment = {0.7, 0.7, 0.7, 1};
  config.result_item_icon_show = true;
  config.result_item_icon_size = 24;
  config.result_item_icon_gap = 12;
  config.result_item_padding = Padding(10);
  config.result_item_margin = Padding(0);
  config.result_item_border_radius = 6;
  config.result_item_border_width = 0;
  config.result_item_border_color = {1, 1, 1, 0.1};
  config.result_item_background = {0, 0, 0, 0};
  config.result_item_text = {0.9, 0.9, 0.9, 1};
  config.result_item_selected_border_radius = 6;
  config.result_item_selected_border_width = 2;
  config.result_item_selected_border_color = {0.3, 0.6, 1, 1};
  config.result_item_selected_background = {1, 1, 1, 0.05};
  config.result_item_selected_text = {1, 1, 1, 1};
  config.result_item_highlight_enable = false;
  config.result_item_highlight_color = {1, 0.78, 0, 1};
  config.result_item_highlight_font_weight = "bold";
  config.result_item_selected_highlight = {1, 1, 0, 1};

  // preview
  config.preview_enable = false;
  config.preview_icon_size = 64;
  config.preview_image_size = 64;
  config.preview_icon_hide_on_fallback = false;
  config.preview_icon_fallback = false;
  config.preview_padding = Padding(10);
  config.preview_margin = Padding(0);
  config.preview_background = {0, 0, 0, 0};
  config.preview_gap_v = 5;
  config.preview_gap_h = 5;
  config.preview_composition = {"icon", "name"};
  config.preview_title_font_family = "sans-serif";
  config.preview_title_font_size = 14;
  config.preview_title_font_weight = "bold";
  config.preview_title_color = {0.9, 0.9, 0.9, 1};
  config.preview_description_font_size = 12;
  config.preview_description_font_weight = "normal";
  config.preview_description_color = {0.6, 0.6, 0.6, 1};

  // results count
  config.results_count_enable = false;
  config.results_count_format = "{count} results";
  config.results_count_font_family = "sans-serif";
  config.results_count_font_size = 11;
  config.results_count_font_weight = "normal";
  config.results_count_text = {0.6, 0.6, 0.6, 1};
  config.results_count_align = "right";
  config.results_count_padding = Padding(2, 8, 2, 8);
  config.results_count_margin = Padding(0);

  // clock
  config.clock_enable = false;
  config.clock_format = "%H:%M";
  config.clock_font_family = "sans-serif";
  config.clock_font_size = 24;
  config.clock_font_weight = "bold";
  config.clock_text = {0.9, 0.9, 0.9, 1};
  config.clock_padding = Padding(10, 20, 10, 20);
  config.clock_margin = Padding(0);
  config.clock_align = "center";

  // providers
  config.providers_apps_command = "";
  config.providers_apps_uwsm = false;
  config.providers_apps_uwsm_prefix = "uwsm app --";
  config.providers_apps_history = true;

  config.providers_bins_exec = "spawn";
  config.providers_bins_history = true;
  config.providers_bins_terminal_exec = false;

  // keybindings
  config.keybindings_inherit = "default";

  config.enabled_plugins.clear();
  config.plugin_configs.clear();
  config.keybindings.clear();
  config.theme_colors.clear();
}

} // namespace Lawnch::Core::Config
