#include "validator.hpp"
#include <set>
#include <string>

namespace Lawnch::Core::Config::Validator {

static const std::set<std::string> VALID_CONFIG_KEYS = {
    // bindings
    "bindings.preset",

    // general
    "general.icon_theme",
    "general.terminal",
    "general.terminal_exec_flag",
    "general.editor",
    "general.history_max_size",
    "general.clear_on_exit",
    "general.locale",

    // launch
    "launch.app_cmd",
    "launch.terminal_app_cmd",
    "launch.prefix",
    "launch.context",
    "launch.start_with",

    // layout
    "layout.theme",
    "layout.order",
    "layout.orientation",
    "layout.preview_position",
    "layout.preview_width_percent",

    // window
    "window.width",
    "window.height",
    "window.anchor",
    "window.margin",
    "window.border_radius",
    "window.border_width",
    "window.exclusive_zone",
    "window.ignore_exclusive_zones",
    "window.keyboard_interactivity",
    "window.border_color",
    "window.background_color",
    "window.layer",

    // input
    "input.visible",
    "input.font_family",
    "input.font_size",
    "input.font_weight",
    "input.placeholder_text",
    "input.caret_width",
    "input.padding",
    "input.border_radius",
    "input.border_width",
    "input.horizontal_align",
    "input.width",
    "input.text_align",
    "input.text_color",
    "input.placeholder_color",
    "input.background_color",
    "input.caret_color",
    "input.selection_color",
    "input.selection_text_color",
    "input.border_color",
    "input.cursor_blink",
    "input.cursor_blink_rate",

    // input_prompt
    "input_prompt.enable",
    "input_prompt.content",
    "input_prompt.position",
    "input_prompt.font_family",
    "input_prompt.font_size",
    "input_prompt.font_weight",
    "input_prompt.text_color",
    "input_prompt.background_color",
    "input_prompt.border_radius",
    "input_prompt.border_width",
    "input_prompt.border_color",
    "input_prompt.padding",

    // results
    "results.margin",
    "results.padding",
    "results.spacing",
    "results.border_width",
    "results.border_radius",
    "results.enable_scrollbar",
    "results.scrollbar_width",
    "results.scrollbar_padding",
    "results.scrollbar_radius",
    "results.scroll_mode",
    "results.reverse",
    "results.background_color",
    "results.border_color",
    "results.scrollbar_color",
    "results.scrollbar_bg_color",
    "results.show_help_on_empty",

    // result_item
    "result_item.font_family",
    "result_item.font_size",
    "result_item.font_weight",
    "result_item.text_align",
    "result_item.enable_comment",
    "result_item.comment_font_size",
    "result_item.comment_font_weight",
    "result_item.enable_icon",
    "result_item.icon_size",
    "result_item.icon_padding_right",
    "result_item.padding",
    "result_item.border_radius",
    "result_item.border_width",
    "result_item.selected_border_radius",
    "result_item.selected_border_width",
    "result_item.enable_highlight",
    "result_item.highlight_font_weight",
    "result_item.text_color",
    "result_item.comment_color",
    "result_item.selected_comment_color",
    "result_item.background_color",
    "result_item.border_color",
    "result_item.selected_background_color",
    "result_item.selected_text_color",
    "result_item.selected_border_color",
    "result_item.highlight_color",
    "result_item.selected_highlight_color",

    // preview
    "preview.enable",
    "preview.icon_size",
    "preview.padding",
    "preview.vertical_spacing",
    "preview.horizontal_spacing",
    "preview.show",
    "preview.name_font_family",
    "preview.name_font_size",
    "preview.name_font_weight",
    "preview.comment_font_size",
    "preview.comment_font_weight",
    "preview.hide_icon_if_fallback",
    "preview.fallback_icon",
    "preview.preview_image_size",
    "preview.background_color",
    "preview.name_color",
    "preview.comment_color",

    // results_count
    "results_count.enable",
    "results_count.format",
    "results_count.font_family",
    "results_count.font_size",
    "results_count.font_weight",
    "results_count.text_align",
    "results_count.padding",
    "results_count.text_color",

    // clock
    "clock.enable",
    "clock.format",
    "clock.font_family",
    "clock.font_size",
    "clock.font_weight",
    "clock.padding",
    "clock.text_align",
    "clock.text_color",
};

bool isValidConfigKey(const std::string &key) {
  return VALID_CONFIG_KEYS.find(key) != VALID_CONFIG_KEYS.end();
}

bool isValidThemeKey(const std::string &key) { return isValidConfigKey(key); }

} // namespace Lawnch::Core::Config::Validator
