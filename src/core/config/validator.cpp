#include "validator.hpp"
#include <set>
#include <string>

namespace Lawnch::Core::Config::Validator {

static const std::set<std::string> VALID_CONFIG_KEYS = {

    "general.icon-theme",
    "general.terminal",
    "general.terminal-flag",
    "general.editor",
    "general.locale",
    "general.history.enable",
    "general.history.max-size",

    "appearance.theme",
    "appearance.preset",
    "appearance.layout.order",
    "appearance.layout.preview-side",
    "appearance.layout.preview-ratio",

    "launch.initial",
    "launch.scope",
    "launch.command",
    "launch.terminal-command",
    "launch.wrapper",

    "keybindings.inherit",
    "keybindings.nav-up",
    "keybindings.nav-down",
    "keybindings.nav-left",
    "keybindings.nav-right",
    "keybindings.delete-word",
    "keybindings.execute",
    "keybindings.clear",

    "window.width",
    "window.height",
    "window.anchor",
    "window.margin",
    "window.background",
    "window.border",
    "window.exclusive",
    "window.ignore-exclusive",
    "window.keyboard",

    "widget.input.font",
    "widget.input.text",
    "widget.input.placeholder",
    "widget.input.background",
    "widget.input.border",
    "widget.input.caret",
    "widget.input.selection",
    "widget.input.padding",
    "widget.input.align",

    "widget.input.prompt.enable",
    "widget.input.prompt.text",
    "widget.input.prompt.side",
    "widget.input.prompt.font",
    "widget.input.prompt.color",
    "widget.input.prompt.background",
    "widget.input.prompt.border",
    "widget.input.prompt.padding",

    "widget.results.margin",
    "widget.results.padding",
    "widget.results.gap",
    "widget.results.background",
    "widget.results.border",
    "widget.results.scroll",
    "widget.results.reverse",
    "widget.results.limit",
    "widget.results.show-help",

    "widget.results.scrollbar.enable",
    "widget.results.scrollbar.width",
    "widget.results.scrollbar.padding",
    "widget.results.scrollbar.radius",
    "widget.results.scrollbar.thumb",
    "widget.results.scrollbar.track",

    "widget.results.item.font",
    "widget.results.item.text",
    "widget.results.item.background",
    "widget.results.item.border",
    "widget.results.item.padding",
    "widget.results.item.align",
    "widget.results.item.icon",

    "widget.results.item.comment.enable",
    "widget.results.item.comment.font",
    "widget.results.item.comment.text",

    "widget.results.item.highlight.enable",
    "widget.results.item.highlight.font",
    "widget.results.item.highlight.text",

    "widget.results.item.selected.text",
    "widget.results.item.selected.background",
    "widget.results.item.selected.border",
    "widget.results.item.selected.comment",
    "widget.results.item.selected.highlight",

    "widget.results.count.enable",
    "widget.results.count.format",
    "widget.results.count.font",
    "widget.results.count.text",
    "widget.results.count.align",
    "widget.results.count.padding",

    "widget.preview.enable",
    "widget.preview.composition",
    "widget.preview.image-size",
    "widget.preview.icon",
    "widget.preview.padding",
    "widget.preview.background",
    "widget.preview.gap",

    "widget.preview.title.font",
    "widget.preview.title.text",

    "widget.preview.description.font",
    "widget.preview.description.text",

    "widget.clock.enable",
    "widget.clock.format",
    "widget.clock.font",
    "widget.clock.text",
    "widget.clock.align",
    "widget.clock.padding",
};

bool isValidConfigKey(const std::string &key) {
  return VALID_CONFIG_KEYS.find(key) != VALID_CONFIG_KEYS.end();
}

bool isValidThemeKey(const std::string &key) {
  if (key.rfind("meta.", 0) == 0 || key.rfind("colors.", 0) == 0) {
    return true;
  }
  return false;
}

} // namespace Lawnch::Core::Config::Validator
