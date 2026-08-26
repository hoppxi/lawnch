#include "validator.hpp"
#include "../../helpers/color.hpp"
#include "../../helpers/config_parse.hpp"
#include <regex>
#include <set>
#include <string>
#include <vector>

namespace Lawnch::Core::Config::Validator {

static bool isValidHexColor(const std::string &value) {
  if (value.empty() || value[0] != '#')
    return false;

  if (value.length() != 4 && value.length() != 5 && value.length() != 7 &&
      value.length() != 9)
    return false;

  for (size_t i = 1; i < value.length(); i++) {
    char c = value[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F')))
      return false;
  }
  return true;
}

static bool isValidThemeVar(const std::string &value) {
  if (value.length() < 2 || value[0] != '$')
    return false;
  for (size_t i = 1; i < value.length(); i++) {
    char c = value[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_'))
      return false;
  }
  return true;
}

static bool isValidRgbaFormat(const std::string &value) {
  std::regex rgba_regex(
      R"(^rgba\(\s*\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*[0-9]*\.?[0-9]+\s*\)$)");
  return std::regex_match(value, rgba_regex);
}

static bool isValidRgbFormat(const std::string &value) {
  std::regex rgb_regex(R"(^rgb\(\s*\d+\s*,\s*\d+\s*,\s*\d+\s*\)$)");
  return std::regex_match(value, rgb_regex);
}

static bool isValidColorFunction(const std::string &value) {
  if (!Lawnch::Config::isColorFunction(value))
    return false;

  int depth = 0;
  for (char c : value) {
    if (c == '(')
      depth++;
    else if (c == ')')
      depth--;
    if (depth < 0)
      return false;
  }
  return depth == 0;
}

static bool isValidColorFormat(const std::string &value) {
  if (value.empty())
    return false;
  if (value[0] == '$')
    return isValidThemeVar(value);
  if (value[0] == '#')
    return isValidHexColor(value);
  if (value.rfind("rgba(", 0) == 0)
    return isValidRgbaFormat(value);
  if (value.rfind("rgb(", 0) == 0)
    return isValidRgbFormat(value);
  if (isValidColorFunction(value))
    return true;

  return false;
}

static bool isValidPaddingFormat(const std::string &value) {
  std::regex pad_regex(R"(^\s*\d+(?:\s+\d+){0,3}\s*$)");
  return std::regex_match(value, pad_regex);
}

static bool isValidAlignValue(const std::string &value) {
  return value == "left" || value == "center" || value == "right" || "fill";
}

static bool isValidScrollValue(const std::string &value) {
  return value == "follow" || value == "fixed";
}

static bool isValidKeyboardValue(const std::string &value) {
  return value == "exclusive" || value == "on-demand" || value == "none";
}

static bool isValidFontWeight(const std::string &value) {
  if (value == "thin" || value == "light" || value == "normal" ||
      value == "medium" || value == "bold" || value == "black")
    return true;

  try {
    int w = std::stoi(value);
    return w >= 1 && w <= 999;
  } catch (...) {
    return false;
  }
}

static bool isValidPromptSide(const std::string &value) {
  return value == "left" || value == "right";
}

static bool isValidExecMode(const std::string &value) {
  return value == "spawn" || value == "exec" || value == "terminal";
}

static const std::set<std::string> VALID_CONFIG_KEYS = {
    "meta.name",
    "meta.description",
    "meta.author",
    "meta.variant",

    "general.icon-theme",
    "general.icon.theme",
    "general.icon.dirs",
    "general.terminal",
    "general.terminal-flag",
    "general.editor",
    "general.locale",
    "general.history.enable",
    "general.history.max-size",

    "appearance.theme",
    "appearance.preset",
    "appearance.layout.order",
    "appearance.layout.direction",
    "appearance.layout.preview-side",
    "appearance.layout.preview-ratio",

    "launch.initial",
    "launch.scope",
    "launch.command",
    "launch.terminal-command",
    "launch.wrapper",

    "keybindings.inherit",

    "modifier.trigger",
    "modifier.expanded",
    "modifier.type",
    "modifier.scope",

    "window.width",
    "window.height",
    "window.anchor",
    "window.margin",
    "window.padding",
    "window.background",
    "window.border",
    "window.border.width",
    "window.border.radius",
    "window.border.color",
    "window.exclusive",
    "window.ignore-exclusive",
    "window.keyboard",
    "window.font",
    "window.font.family",
    "window.font.size",
    "window.font.weight",
    "window.font.path",

    "widget.input.font",
    "widget.input.font.family",
    "widget.input.font.size",
    "widget.input.font.weight",
    "widget.input.font.path",
    "widget.input.color",
    "widget.input.foreground",
    "widget.input.placeholder",
    "widget.input.placeholder.text",
    "widget.input.placeholder.color",
    "widget.input.placeholder.foreground",
    "widget.input.background",
    "widget.input.border",
    "widget.input.border.width",
    "widget.input.border.radius",
    "widget.input.border.color",
    "widget.input.caret",
    "widget.input.caret.width",
    "widget.input.caret.color",
    "widget.input.selection",
    "widget.input.selection.background",
    "widget.input.selection.color",
    "widget.input.selection.foreground",
    "widget.input.padding",
    "widget.input.margin",
    "widget.input.align",
    "widget.input.visible",

    "widget.input.prompt.enable",
    "widget.input.prompt.text",
    "widget.input.prompt.side",
    "widget.input.prompt.font",
    "widget.input.prompt.font.family",
    "widget.input.prompt.font.size",
    "widget.input.prompt.font.weight",
    "widget.input.prompt.font.path",
    "widget.input.prompt.color",
    "widget.input.prompt.foreground",
    "widget.input.prompt.background",
    "widget.input.prompt.border",
    "widget.input.prompt.border.width",
    "widget.input.prompt.border.radius",
    "widget.input.prompt.border.color",
    "widget.input.prompt.padding",
    "widget.input.prompt.margin",

    "widget.results.direction",
    "widget.results.column",
    "widget.results.margin",
    "widget.results.padding",
    "widget.results.gap",
    "widget.results.background",
    "widget.results.border",
    "widget.results.border.width",
    "widget.results.border.radius",
    "widget.results.border.color",
    "widget.results.scroll",
    "widget.results.reverse",
    "widget.results.limit",
    "widget.results.show-help",

    "widget.results.scrollbar.enable",
    "widget.results.scrollbar.thumb",
    "widget.results.scrollbar.thumb.radius",
    "widget.results.scrollbar.thumb.color",
    "widget.results.scrollbar.track",
    "widget.results.scrollbar.track.width",
    "widget.results.scrollbar.track.radius",
    "widget.results.scrollbar.track.color",
    "widget.results.scrollbar.track.padding",
    "widget.results.scrollbar.track.margin",

    "widget.results.item.icon",
    "widget.results.item.icon.enable",
    "widget.results.item.icon.size",
    "widget.results.item.icon.gap",
    "widget.results.item.comment",
    "widget.results.item.highlight",
    "widget.results.item.direction",

    "widget.results.item.default.font",
    "widget.results.item.default.font.family",
    "widget.results.item.default.font.size",
    "widget.results.item.default.font.weight",
    "widget.results.item.default.font.path",
    "widget.results.item.default.color",
    "widget.results.item.default.foreground",
    "widget.results.item.default.background",
    "widget.results.item.default.border",
    "widget.results.item.default.border.width",
    "widget.results.item.default.border.radius",
    "widget.results.item.default.border.color",
    "widget.results.item.default.padding",
    "widget.results.item.default.margin",
    "widget.results.item.default.align",
    "widget.results.item.default.comment",
    "widget.results.item.default.comment.font",
    "widget.results.item.default.comment.font.family",
    "widget.results.item.default.comment.font.size",
    "widget.results.item.default.comment.font.weight",
    "widget.results.item.default.comment.font.path",
    "widget.results.item.default.comment.color",
    "widget.results.item.default.comment.foreground",
    "widget.results.item.default.highlight",
    "widget.results.item.default.highlight.font",
    "widget.results.item.default.highlight.font.family",
    "widget.results.item.default.highlight.font.size",
    "widget.results.item.default.highlight.font.weight",
    "widget.results.item.default.highlight.font.path",
    "widget.results.item.default.highlight.color",
    "widget.results.item.default.highlight.foreground",

    "widget.results.item.selected.font",
    "widget.results.item.selected.font.family",
    "widget.results.item.selected.font.size",
    "widget.results.item.selected.font.weight",
    "widget.results.item.selected.font.path",
    "widget.results.item.selected.color",
    "widget.results.item.selected.foreground",
    "widget.results.item.selected.text",
    "widget.results.item.selected.background",
    "widget.results.item.selected.border",
    "widget.results.item.selected.border.width",
    "widget.results.item.selected.border.radius",
    "widget.results.item.selected.border.color",
    "widget.results.item.selected.padding",
    "widget.results.item.selected.margin",
    "widget.results.item.selected.align",
    "widget.results.item.selected.comment",
    "widget.results.item.selected.comment.font",
    "widget.results.item.selected.comment.font.family",
    "widget.results.item.selected.comment.font.size",
    "widget.results.item.selected.comment.font.weight",
    "widget.results.item.selected.comment.font.path",
    "widget.results.item.selected.comment.color",
    "widget.results.item.selected.comment.foreground",
    "widget.results.item.selected.highlight",
    "widget.results.item.selected.highlight.font",
    "widget.results.item.selected.highlight.font.family",
    "widget.results.item.selected.highlight.font.size",
    "widget.results.item.selected.highlight.font.weight",
    "widget.results.item.selected.highlight.font.path",
    "widget.results.item.selected.highlight.color",
    "widget.results.item.selected.highlight.foreground",

    "widget.results.count.enable",
    "widget.results.count.format",
    "widget.results.count.font",
    "widget.results.count.font.family",
    "widget.results.count.font.size",
    "widget.results.count.font.weight",
    "widget.results.count.font.path",
    "widget.results.count.color",
    "widget.results.count.foreground",
    "widget.results.count.align",
    "widget.results.count.padding",
    "widget.results.count.margin",

    "widget.preview.enable",
    "widget.preview.direction",
    "widget.preview.preview-image-size",
    "widget.preview.icon",
    "widget.preview.padding",
    "widget.preview.margin",
    "widget.preview.background",
    "widget.preview.gap",
    "widget.preview.gap.v",
    "widget.preview.gap.h",

    "widget.preview.icon.size",
    "widget.preview.icon.fallback",

    "widget.preview.name.font",
    "widget.preview.name.font.family",
    "widget.preview.name.font.size",
    "widget.preview.name.font.weight",
    "widget.preview.name.font.path",
    "widget.preview.name.color",
    "widget.preview.name.foreground",

    "widget.preview.comment.font",
    "widget.preview.comment.font.family",
    "widget.preview.comment.font.size",
    "widget.preview.comment.font.weight",
    "widget.preview.comment.font.path",
    "widget.preview.comment.color",
    "widget.preview.comment.foreground",

    "widget.clock.enable",
    "widget.clock.format",
    "widget.clock.font",
    "widget.clock.font.family",
    "widget.clock.font.size",
    "widget.clock.font.weight",
    "widget.clock.font.path",
    "widget.clock.color",
    "widget.clock.foreground",
    "widget.clock.align",
    "widget.clock.padding",
    "widget.clock.margin",

    "widget.breadcrumbs.enable",
    "widget.breadcrumbs.separator",
    "widget.breadcrumbs.font",
    "widget.breadcrumbs.font.family",
    "widget.breadcrumbs.font.size",
    "widget.breadcrumbs.font.weight",
    "widget.breadcrumbs.font.path",
    "widget.breadcrumbs.color",
    "widget.breadcrumbs.align",
    "widget.breadcrumbs.padding",
    "widget.breadcrumbs.margin",

    "providers.apps.command",
    "providers.apps.uwsm",
    "providers.apps.uwsm-prefix",
    "providers.apps.history",

    "providers.bins.exec",
    "providers.bins.history",
    "providers.bins.terminal-exec",
};

static void validateNode(const toml::node &node,
                         const std::string &current_path,
                         ValidationResult &res) {
  if (current_path == "modifier") {
    if (!node.is_array()) {
      res.errors.push_back("Property 'modifier' must be an array of tables "
                           "(e.g., [[modifier]])");
    } else {
      for (const auto &el : *node.as_array()) {
        if (!el.is_table()) {
          res.errors.push_back("Elements of 'modifier' array must be tables");
        } else {
          for (auto &[k, v] : *el.as_table()) {
            std::string next_path = current_path + "." + std::string(k.str());
            validateNode(v, next_path, res);
          }
        }
      }
    }
    return;
  }

  if (node.is_table()) {
    const auto &table = *node.as_table();
    for (auto &[k, v] : table) {
      std::string next_path = current_path.empty()
                                  ? std::string(k.str())
                                  : current_path + "." + std::string(k.str());

      if (next_path.rfind("keybindings.", 0) == 0) {
        if (!v.is_string()) {
          res.errors.push_back("Keybinding '" + next_path +
                               "' must be a string");
        }
        continue;
      }

      if (next_path == "plugin" || next_path.rfind("plugin.", 0) == 0) {
        continue;
      }

      if (VALID_CONFIG_KEYS.find(next_path) == VALID_CONFIG_KEYS.end()) {
        bool has_valid_children = false;
        std::string prefix = next_path + ".";
        for (const auto &valid_key : VALID_CONFIG_KEYS) {
          if (valid_key.rfind(prefix, 0) == 0) {
            has_valid_children = true;
            break;
          }
        }

        if (!has_valid_children) {
          res.errors.push_back("Unknown configuration key: " + next_path);
        }
      }

      validateNode(v, next_path, res);
    }
    return;
  }

  if (current_path == "window.anchor") {
    if (!node.is_array()) {
      res.errors.push_back("'" + current_path +
                           "' must be an array of strings");
    } else {
      std::set<std::string> seen;
      for (const auto &el : *node.as_array()) {
        if (!el.is_string()) {
          res.errors.push_back("'" + current_path +
                               "' elements must be strings");
        } else {
          std::string val = el.as_string()->get();
          if (val != "top" && val != "bottom" && val != "left" &&
              val != "right" && val != "center") {
            res.errors.push_back(
                "Invalid anchor value '" + val + "' in '" + current_path +
                "'. Valid anchors: top, bottom, left, right, center");
          }
          if (seen.count(val)) {
            res.errors.push_back("Duplicate anchor value '" + val + "' in '" +
                                 current_path + "'");
          }
          seen.insert(val);
        }
      }
    }
    return;
  }

  if (current_path.ends_with(".color") ||
      current_path.ends_with(".foreground") ||
      current_path.ends_with("background") ||
      current_path.ends_with(".thumb") || current_path.ends_with(".track")) {
    if (!node.is_string()) {
      res.errors.push_back("Color property '" + current_path +
                           "' must be a string");
    } else {
      std::string val = node.as_string()->get();
      if (!isValidColorFormat(val)) {
        res.errors.push_back(
            "Invalid color format for '" + current_path + "': " + val +
            ". Expected: #hex, $theme-var, rgb(), rgba(), or color function");
      }
    }
    return;
  }

  if (current_path.ends_with(".padding") || current_path.ends_with(".margin")) {
    if (node.is_string()) {
      if (!isValidPaddingFormat(node.as_string()->get())) {
        res.errors.push_back("Invalid padding format for '" + current_path +
                             "': " + node.as_string()->get());
      }
    } else if (node.is_array()) {
      auto arr = node.as_array();
      if (arr->size() > 4) {
        res.errors.push_back("Padding array '" + current_path +
                             "' cannot have more than 4 elements");
      }
      if (arr->size() == 0) {
        res.errors.push_back("Padding array '" + current_path +
                             "' must have at least 1 element");
      }
      for (const auto &el : *arr) {
        if (!el.is_integer()) {
          res.errors.push_back("Padding array elements for '" + current_path +
                               "' must be integers");
        } else if (el.as_integer()->get() < 0) {
          res.warnings.push_back("Negative padding value in '" + current_path +
                                 "'");
        }
      }
    } else if (node.is_integer()) {
      if (node.as_integer()->get() < 0) {
        res.warnings.push_back("Negative padding value for '" + current_path +
                               "'");
      }
    } else {
      res.errors.push_back("Padding '" + current_path +
                           "' must be an integer, string, or array");
    }
    return;
  }

  if (current_path.ends_with(".enable") || current_path.ends_with("reverse") ||
      current_path.ends_with("show-help") ||
      current_path.ends_with("exclusive") ||
      current_path.ends_with("ignore-exclusive") ||
      current_path.ends_with("visible") || current_path.ends_with("uwsm") ||
      current_path.ends_with("history") ||
      current_path.ends_with("terminal-exec") ||
      current_path.ends_with("fallback") ||
      current_path.ends_with("comment") ||
      current_path.ends_with("highlight")) {
    if (node.is_boolean() == false) {
      res.errors.push_back("Property '" + current_path + "' must be a boolean");
    }
    return;
  }

  if (current_path.ends_with(".size") || current_path.ends_with(".width") ||
      current_path.ends_with(".height") || current_path.ends_with(".radius") ||
      current_path.ends_with(".limit") || current_path.ends_with(".gap") ||
      current_path.ends_with("preview-image-size") ||
      current_path.ends_with("max-size") ||
      current_path.ends_with("preview-ratio") ||
      current_path.ends_with(".column") || current_path.ends_with(".v") ||
      current_path.ends_with(".h")) {
    if (current_path.ends_with(".gap") && node.is_table()) {
      return;
    }
    if (!node.is_integer() && !node.is_floating_point()) {
      res.errors.push_back("Property '" + current_path + "' must be a number");
    } else {
      double val = 0;
      if (node.is_integer())
        val = static_cast<double>(node.as_integer()->get());
      else
        val = node.as_floating_point()->get();

      if (current_path.ends_with(".size") ||
          current_path.ends_with(".radius") ||
          current_path.ends_with("preview-image-size")) {
        if (val < 0) {
          res.errors.push_back("Property '" + current_path +
                               "' must be non-negative");
        }
      }
      if (current_path.ends_with("preview-ratio")) {
        if (val < 0 || val > 100) {
          res.errors.push_back("Property '" + current_path +
                               "' must be between 0 and 100");
        }
      }
    }
    return;
  }

  if (current_path.ends_with(".align")) {
    if (!node.is_string()) {
      res.errors.push_back("Property '" + current_path + "' must be a string");
    } else if (!isValidAlignValue(node.as_string()->get())) {
      res.errors.push_back("Invalid align value '" + node.as_string()->get() +
                           "' for '" + current_path +
                           "'. Valid: left, center, right");
    }
    return;
  }
  if (current_path == "widget.results.scroll") {
    if (!node.is_string()) {
      res.errors.push_back("Property '" + current_path + "' must be a string");
    } else if (!isValidScrollValue(node.as_string()->get())) {
      res.errors.push_back("Invalid scroll value '" + node.as_string()->get() +
                           "' for '" + current_path +
                           "'. Valid: follow, fixed");
    }
    return;
  }
  if (current_path == "modifier.scope") {
    if (node.is_array()) {
      for (const auto &el : *node.as_array()) {
        if (!el.is_string()) {
          res.errors.push_back("Array elements in '" + current_path +
                               "' must be strings");
        }
      }
    } else {
      res.errors.push_back("Property '" + current_path +
                           "' must be an array of strings");
    }
    return;
  }
  if (current_path == "modifier.type") {
    if (!node.is_string()) {
      res.errors.push_back("Property '" + current_path + "' must be a string");
    } else {
      std::string val = node.as_string()->get();
      if (val != "abbr" && val != "alias") {
        res.errors.push_back("Invalid modifier type '" + val +
                             "'. Valid: abbr, alias");
      }
    }
    return;
  }
  if (current_path == "window.keyboard") {
    if (!node.is_string()) {
      res.errors.push_back("Property '" + current_path + "' must be a string");
    } else if (!isValidKeyboardValue(node.as_string()->get())) {
      res.errors.push_back("Invalid keyboard value '" +
                           node.as_string()->get() + "' for '" + current_path +
                           "'. Valid: exclusive, on-demand, none");
    }
    return;
  }
  if (current_path.ends_with(".font.weight") ||
      current_path == "window.font.weight") {
    if (!node.is_string()) {
      res.errors.push_back("Property '" + current_path + "' must be a string");
    } else if (!isValidFontWeight(node.as_string()->get())) {
      res.errors.push_back(
          "Invalid font weight '" + node.as_string()->get() + "' for '" +
          current_path +
          "'. Valid: thin, light, normal, medium, bold, black, or 1-999");
    }
    return;
  }
  if (current_path == "widget.input.prompt.side") {
    if (!node.is_string()) {
      res.errors.push_back("Property '" + current_path + "' must be a string");
    } else if (!isValidPromptSide(node.as_string()->get())) {
      res.errors.push_back("Invalid prompt side '" + node.as_string()->get() +
                           "' for '" + current_path + "'. Valid: left, right");
    }
    return;
  }
  if (current_path == "providers.bins.exec") {
    if (!node.is_string()) {
      res.errors.push_back("Property '" + current_path + "' must be a string");
    } else if (!isValidExecMode(node.as_string()->get())) {
      res.errors.push_back("Invalid exec mode '" + node.as_string()->get() +
                           "' for '" + current_path +
                           "'. Valid: spawn, exec, fork");
    }
    return;
  }

  if (current_path.ends_with(".text") || current_path.ends_with(".family") ||
      current_path.ends_with(".format") ||
      current_path.ends_with(".separator") ||
      current_path.ends_with(".terminal") ||
      current_path.ends_with(".terminal-flag") ||
      current_path.ends_with(".editor") || current_path.ends_with(".locale") ||
      current_path.ends_with(".theme") || current_path.ends_with(".preset") ||
      current_path.ends_with(".initial") || current_path.ends_with(".scope") ||
      current_path == "modifier.trigger" ||
      current_path == "modifier.expanded" || current_path == "modifier.scope" ||
      current_path.ends_with(".command") ||
      current_path.ends_with("terminal-command") ||
      current_path.ends_with(".wrapper") ||
      current_path.ends_with("uwsm-prefix") ||
      current_path.ends_with(".inherit") ||
      current_path.ends_with("preview-side") ||
      current_path.ends_with(".direction") ||
      current_path.ends_with(".side") || current_path.ends_with("icon-theme") ||
      current_path.ends_with(".path")) {
    if (!node.is_string()) {
      res.errors.push_back("Property '" + current_path + "' must be a string");
    }
    return;
  }

  if (current_path.ends_with(".order") ||
      current_path.ends_with(".dirs")) {
    if (!node.is_array()) {
      res.errors.push_back("Property '" + current_path + "' must be an array");
    } else {
      for (const auto &el : *node.as_array()) {
        if (!el.is_string()) {
          res.errors.push_back("Array elements in '" + current_path +
                               "' must be strings");
        }
      }
    }
    return;
  }
}

ValidationResult validateConfig(const toml::table &root, bool is_preset) {
  ValidationResult res{true, {}, {}};

  validateNode(root, "", res);

  res.success = res.errors.empty();
  return res;
}

ValidationResult validateTheme(const toml::table &root) {
  ValidationResult res{true, {}, {}};

  for (auto &[k, v] : root) {
    if (k.str() != "meta" && k.str() != "colors") {
      res.errors.push_back("Unknown root key in theme: " +
                           std::string(k.str()));
    }
  }

  if (!root.contains("colors") || !root["colors"].is_table()) {
    res.errors.push_back("Theme is missing the [colors] table");
    res.success = false;
    return res;
  }

  const auto &colors = *root["colors"].as_table();

  static const std::set<std::string> REQUIRED_THEME_KEYS = {
      "primary",
      "on-primary",
      "primary-container",
      "on-primary-container",
      "secondary",
      "on-secondary",
      "secondary-container",
      "on-secondary-container",
      "tertiary",
      "on-tertiary",
      "tertiary-container",
      "on-tertiary-container",
      "surface",
      "surface-dim",
      "surface-bright",
      "surface-container",
      "surface-container-low",
      "surface-container-high",
      "surface-container-highest",
      "on-surface",
      "on-surface-variant",
      "outline",
      "outline-variant",
      "error",
      "on-error",
      "error-container",
      "on-error-container",
      "inverse-surface",
      "inverse-on-surface",
      "inverse-primary",
      "background",
      "on-background",
      "scrim"};

  for (const auto &req_key : REQUIRED_THEME_KEYS) {
    if (!colors.contains(req_key)) {
      res.warnings.push_back("Theme lacks recommended color key: " + req_key);
    }
  }

  for (auto &[k, v] : colors) {
    std::string key_str = std::string(k.str());
    if (REQUIRED_THEME_KEYS.find(key_str) == REQUIRED_THEME_KEYS.end()) {
      res.warnings.push_back("Theme has unknown color key: " + key_str);
    }

    if (!v.is_string()) {
      res.errors.push_back("Color '" + key_str + "' must be a string");
    } else if (!isValidColorFormat(v.as_string()->get())) {
      res.errors.push_back("Invalid color format for '" + key_str +
                           "': " + v.as_string()->get());
    }
  }

  res.success = res.errors.empty();
  return res;
}

} // namespace Lawnch::Core::Config::Validator
