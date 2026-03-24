#include "manager_impl.hpp"

namespace Lawnch::Core::Config {

void Manager::Impl::SetDefaults() {
  config = Config{};

  // general
  config.general_icon_theme = "";
  config.general_icon_dirs = {};
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
  config.launch_initial = ":app";

  // appearance
  config.appearance_theme = "";
  config.appearance_preset = "";
  config.layout_order = {"input", "results"};
  config.layout_preview_side = "bottom";
  config.layout_preview_ratio = 30;

  // window
  config.window_width = 300;
  config.window_height = 300;
  config.window_anchor = {"top", "center"};
  config.window_margin = Padding(0);
  config.window_padding = Padding(0);
  config.window_background = {0, 0, 0, 0};
  config.window_border_color = {0, 0, 0, 0};
  config.window_border_radius = 0;
  config.window_border_width = 0;
  config.window_exclusive = false;
  config.window_ignore_exclusive = false;
  config.window_keyboard = "exclusive";
  config.window_font_family = "sans-serif";
  config.window_font_path = "";
  config.window_font_size = 14;
  config.window_font_weight = "normal";

  // input
  config.input_visible = true;
  config.input_font_family = "sans-serif";
  config.input_font_path = "";
  config.input_font_size = 14;
  config.input_font_weight = "normal";
  config.input_color = {1, 1, 1, 1};
  config.input_placeholder_text = "";
  config.input_placeholder_color = {0.6, 0.6, 0.6, 1};
  config.input_background = {0, 0, 0, 0};
  config.input_caret_color = {1, 1, 1, 1};
  config.input_caret_width = 2.0;
  config.input_selection_background = {0.2, 0.4, 0.8, 0.5};
  config.input_selection_color = {1, 1, 1, 1};
  config.input_padding = Padding(0);
  config.input_margin = Padding(0);
  config.input_border_radius = 0;
  config.input_border_width = 0;
  config.input_border_color = {0, 0, 0, 0};
  config.input_align = "left";

  // input prompt
  config.input_prompt_enable = false;
  config.input_prompt_text = "";
  config.input_prompt_side = "left";
  config.input_prompt_font_family = "sans-serif";
  config.input_prompt_font_path = "";
  config.input_prompt_font_size = 14;
  config.input_prompt_font_weight = "normal";
  config.input_prompt_color = {1, 1, 1, 1};
  config.input_prompt_background = {0, 0, 0, 0};
  config.input_prompt_border_radius = 0;
  config.input_prompt_border_width = 0;
  config.input_prompt_border_color = {0, 0, 0, 0};
  config.input_prompt_padding = Padding(0);
  config.input_prompt_margin = Padding(0);

  // results
  config.results_margin = Padding(0);
  config.results_padding = Padding(0);
  config.results_gap = 0;
  config.results_background = {0, 0, 0, 0};
  config.results_border_color = {0, 0, 0, 0};
  config.results_border_width = 0;
  config.results_border_radius = 0;
  config.results_scrollbar_enable = false;
  config.results_scrollbar_thumb_color = {0.8, 0.8, 0.8, 0.5};
  config.results_scrollbar_thumb_radius = 0;
  config.results_scrollbar_track_width = 2;
  config.results_scrollbar_track_color = {0, 0, 0, 0};
  config.results_scrollbar_track_padding = Padding(0);
  config.results_scrollbar_track_margin = Padding(0);
  config.results_scrollbar_track_radius = 0;
  config.results_scroll = "follow";
  config.results_reverse = false;
  config.results_limit = 50;
  config.results_show_help = false;

  // result item structure
  config.result_item_icon_enable = false;
  config.result_item_icon_size = 24;
  config.result_item_icon_gap = 12;
  config.result_item_comment_enable = false;
  config.result_item_highlight_enable = false;

  // result item default
  config.result_item_default_font_family = "sans-serif";
  config.result_item_default_font_path = "";
  config.result_item_default_font_size = 14;
  config.result_item_default_font_weight = "normal";
  config.result_item_default_align = "left";
  config.result_item_default_color = {1, 1, 1, 1};
  config.result_item_default_background = {0, 0, 0, 0};
  config.result_item_default_border_radius = 0;
  config.result_item_default_border_width = 0;
  config.result_item_default_border_color = {0, 0, 0, 0};
  config.result_item_default_padding = Padding(0);
  config.result_item_default_margin = Padding(0);
  config.result_item_default_comment_font_size = 10;
  config.result_item_default_comment_font_path = "";
  config.result_item_default_comment_font_weight = "normal";
  config.result_item_default_comment_color = {0.6, 0.6, 0.6, 1};
  config.result_item_default_highlight_font_weight = "bold";
  config.result_item_default_highlight_font_path = "";
  config.result_item_default_highlight_color = {1, 0.78, 0, 1};

  // result item selected (inherit from default)
  config.result_item_selected_font_family =
      config.result_item_default_font_family;
  config.result_item_selected_font_size = config.result_item_default_font_size;
  config.result_item_selected_font_weight =
      config.result_item_default_font_weight;
  config.result_item_selected_align = config.result_item_default_align;
  config.result_item_selected_color = config.result_item_default_color;
  config.result_item_selected_background =
      config.result_item_default_background;
  config.result_item_selected_border_radius =
      config.result_item_default_border_radius;
  config.result_item_selected_border_width =
      config.result_item_default_border_width;
  config.result_item_selected_border_color =
      config.result_item_default_border_color;
  config.result_item_selected_padding = config.result_item_default_padding;
  config.result_item_selected_margin = config.result_item_default_margin;
  config.result_item_selected_comment_font_size =
      config.result_item_default_comment_font_size;
  config.result_item_selected_comment_font_weight =
      config.result_item_default_comment_font_weight;
  config.result_item_selected_comment_color =
      config.result_item_default_comment_color;
  config.result_item_selected_highlight_font_weight =
      config.result_item_default_highlight_font_weight;
  config.result_item_selected_highlight_color =
      config.result_item_default_highlight_color;

  // preview
  config.preview_enable = false;
  config.preview_icon_size = 16;
  config.preview_image_size = 64;
  config.preview_icon_hide_on_fallback = false;
  config.preview_icon_fallback = false;
  config.preview_padding = Padding(0);
  config.preview_margin = Padding(0);
  config.preview_background = {0, 0, 0, 0};
  config.preview_gap_v = 5;
  config.preview_gap_h = 5;
  config.preview_composition = {"icon", "name"};
  config.preview_name_font_family = "sans-serif";
  config.preview_name_font_path = "";
  config.preview_name_font_size = 14;
  config.preview_name_font_weight = "bold";
  config.preview_name_color = {1, 1, 1, 1};
  config.preview_comment_font_size = 12;
  config.preview_comment_font_path = "";
  config.preview_comment_font_weight = "normal";
  config.preview_comment_color = {0.6, 0.6, 0.6, 1};

  // results count
  config.results_count_enable = false;
  config.results_count_format = "{count} results";
  config.results_count_font_family = "sans-serif";
  config.results_count_font_path = "";
  config.results_count_font_size = 11;
  config.results_count_font_weight = "normal";
  config.results_count_color = {0.6, 0.6, 0.6, 1};
  config.results_count_align = "right";
  config.results_count_padding = Padding(0);
  config.results_count_margin = Padding(0);

  // clock
  config.clock_enable = false;
  config.clock_format = "%H:%M";
  config.clock_font_family = "sans-serif";
  config.clock_font_path = "";
  config.clock_font_size = 14;
  config.clock_font_weight = "normal";
  config.clock_color = {1, 1, 1, 1};
  config.clock_padding = Padding(0);
  config.clock_margin = Padding(0);
  config.clock_align = "center";

  // breadcrumbs
  config.breadcrumbs_enable = false;
  config.breadcrumbs_separator = "/";
  config.breadcrumbs_font_family = "sans-serif";
  config.breadcrumbs_font_path = "";
  config.breadcrumbs_font_size = 11;
  config.breadcrumbs_font_weight = "normal";
  config.breadcrumbs_color = {0.6, 0.6, 0.6, 1};
  config.breadcrumbs_align = "left";
  config.breadcrumbs_padding = Padding(0);
  config.breadcrumbs_margin = Padding(0);

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
