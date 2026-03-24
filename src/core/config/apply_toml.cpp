#include "manager_impl.hpp"

namespace Lawnch::Core::Config {

void Manager::Impl::LoadThemeColors(const toml::table &theme_tbl) {
  auto *colors = getTable(theme_tbl, "colors");
  if (!colors)
    return;
  for (auto &[key, val] : *colors) {
    if (auto str = val.as_string())
      config.theme_colors[std::string(key.str())] =
          Lawnch::Config::parseHexColor(str->get());
  }
}

void Manager::Impl::ApplyToml(const toml::table &root) {
  ApplyGeneral(root);
  ApplyAppearance(root);
  ApplyLaunch(root);
  ApplyKeybindings(root);
  ApplyWindow(root);
  ApplyInput(root);
  ApplyInputPrompt(root);
  ApplyResults(root);
  ApplyResultItem(root);
  ApplyResultsCount(root);
  ApplyPreview(root);
  ApplyClock(root);
  ApplyBreadcrumbs(root);
  ApplyProviders(root);
  ApplyPlugins(root);
  ApplyModifiers(root);
}

void Manager::Impl::ApplyGeneral(const toml::table &root) {
  auto *t = getTable(root, "general");
  if (!t)
    return;

  if (auto *icon = getTable(*t, "icon")) {
    config.general_icon_theme =
        getStr(*icon, "theme", config.general_icon_theme);
    config.general_icon_dirs =
        getStrArray(*icon, "dirs", config.general_icon_dirs);
  }

  config.general_terminal = getStr(*t, "terminal", config.general_terminal);
  config.general_terminal_flag =
      getStr(*t, "terminal-flag", config.general_terminal_flag);
  config.general_editor = getStr(*t, "editor", config.general_editor);
  config.general_locale = getStr(*t, "locale", config.general_locale);

  if (auto *h = getTable(*t, "history")) {
    config.general_history = getBool(*h, "enable", config.general_history);
    config.general_history_max_size =
        getInt(*h, "max-size", config.general_history_max_size);
  }
}

void Manager::Impl::ApplyAppearance(const toml::table &root) {
  auto *t = getTable(root, "appearance");
  if (!t)
    return;

  config.appearance_theme = getStr(*t, "theme", config.appearance_theme);
  config.appearance_preset = getStr(*t, "preset", config.appearance_preset);

  if (auto *l = getTable(*t, "layout")) {
    config.layout_order = getStrArray(*l, "order", config.layout_order);
    config.layout_preview_side =
        getStr(*l, "preview-side", config.layout_preview_side);
    config.layout_preview_ratio =
        getInt(*l, "preview-ratio", config.layout_preview_ratio);
  }
}

void Manager::Impl::ApplyLaunch(const toml::table &root) {
  auto *t = getTable(root, "launch");
  if (!t)
    return;

  config.launch_initial = getStr(*t, "initial", config.launch_initial);
  config.launch_scope = getStr(*t, "scope", config.launch_scope);
  config.launch_command = getStr(*t, "command", config.launch_command);
  config.launch_terminal_command =
      getStr(*t, "terminal-command", config.launch_terminal_command);
  config.launch_wrapper = getStr(*t, "wrapper", config.launch_wrapper);
}

void Manager::Impl::ApplyKeybindings(const toml::table &root) {
  auto *t = getTable(root, "keybindings");
  if (!t)
    return;

  config.keybindings_inherit =
      getStr(*t, "inherit", config.keybindings_inherit);
  for (auto &[key, val] : *t) {
    std::string k(key.str());
    if (k == "inherit")
      continue;
    if (auto s = val.as_string())
      config.keybindings[k] = s->get();
  }
}

void Manager::Impl::ApplyWindow(const toml::table &root) {
  auto *t = getTable(root, "window");
  if (!t)
    return;

  auto &tc = config.theme_colors;

  config.window_width = getInt(*t, "width", config.window_width);
  config.window_height = getInt(*t, "height", config.window_height);

  if (auto anchor_node = (*t)["anchor"]; anchor_node) {
    if (auto arr = anchor_node.as_array()) {
      std::vector<std::string> anchors;
      for (auto &el : *arr) {
        if (auto s = el.as_string())
          anchors.push_back(s->get());
      }
      if (!anchors.empty())
        config.window_anchor = anchors;
    }
  }

  config.window_margin = getPadding(*t, "margin", config.window_margin);
  config.window_background =
      getColor(*t, "background", tc, config.window_background);

  if (auto bnode = (*t)["border"]; bnode) {
    if (auto bt = bnode.as_table()) {
      config.window_border_radius =
          getInt(*bt, "radius", config.window_border_radius);
      config.window_border_width =
          getInt(*bt, "width", config.window_border_width);
      config.window_border_color =
          getColor(*bt, "color", tc, config.window_border_color);
    }
  }

  config.window_exclusive = getBool(*t, "exclusive", config.window_exclusive);
  config.window_ignore_exclusive =
      getBool(*t, "ignore-exclusive", config.window_ignore_exclusive);
  config.window_keyboard = getStr(*t, "keyboard", config.window_keyboard);

  if (auto fnode = (*t)["font"]; fnode) {
    if (auto ft = fnode.as_table()) {
      config.window_font_family =
          getStr(*ft, "family", config.window_font_family);
      config.window_font_path = getStr(*ft, "path", config.window_font_path);
      config.window_font_size = getInt(*ft, "size", config.window_font_size);
      config.window_font_weight =
          getStr(*ft, "weight", config.window_font_weight);
    }
  }
}

void Manager::Impl::ApplyInput(const toml::table &root) {
  auto *t = getTable(root, "widget.input");
  if (!t)
    return;

  auto &tc = config.theme_colors;

  if (auto fnode = (*t)["font"]; fnode) {
    if (auto ft = fnode.as_table()) {
      config.input_font_family =
          getStr(*ft, "family", config.window_font_family);
      std::string fallback_path =
          ft->contains("family") ? "" : config.window_font_path;
      config.input_font_path = getStr(*ft, "path", fallback_path);
      config.input_font_size = getInt(*ft, "size", config.input_font_size);
      config.input_font_weight =
          getStr(*ft, "weight", config.window_font_weight);
    }
  } else {
    config.input_font_family = config.window_font_family;
    config.input_font_path = config.window_font_path;
    config.input_font_size = config.window_font_size;
    config.input_font_weight = config.window_font_weight;
  }

  config.input_color = getFontColor(*t, tc, config.input_color);
  config.input_background =
      getColor(*t, "background", tc, config.input_background);

  if (auto pnode = (*t)["placeholder"]; pnode) {
    if (auto pt = pnode.as_table()) {
      config.input_placeholder_text =
          getStr(*pt, "text", config.input_placeholder_text);
      config.input_placeholder_color =
          getFontColor(*pt, tc, config.input_placeholder_color);
    }
  }

  if (auto bnode = (*t)["border"]; bnode) {
    if (auto bt = bnode.as_table()) {
      config.input_border_radius =
          getInt(*bt, "radius", config.input_border_radius);
      config.input_border_width =
          getInt(*bt, "width", config.input_border_width);
      config.input_border_color =
          getColor(*bt, "color", tc, config.input_border_color);
    }
  }

  if (auto cnode = (*t)["caret"]; cnode) {
    if (auto ct = cnode.as_table()) {
      config.input_caret_width =
          getDouble(*ct, "width", config.input_caret_width);
      config.input_caret_color =
          getColor(*ct, "color", tc, config.input_caret_color);
    }
  }

  if (auto snode = (*t)["selection"]; snode) {
    if (auto st = snode.as_table()) {
      config.input_selection_background =
          getColor(*st, "background", tc, config.input_selection_background);
      config.input_selection_color =
          getFontColor(*st, tc, config.input_selection_color);
    }
  }

  config.input_padding = getPadding(*t, "padding", config.input_padding);
  config.input_margin = getPadding(*t, "margin", config.input_margin);
  config.input_align = getStr(*t, "align", config.input_align);
  config.input_visible = getBool(*t, "visible", config.input_visible);
}

void Manager::Impl::ApplyInputPrompt(const toml::table &root) {
  auto *t = getTable(root, "widget.input.prompt");
  if (!t)
    return;

  auto &tc = config.theme_colors;

  config.input_prompt_enable =
      getBool(*t, "enable", config.input_prompt_enable);
  config.input_prompt_text = getStr(*t, "text", config.input_prompt_text);
  config.input_prompt_side = getStr(*t, "side", config.input_prompt_side);

  if (auto fnode = (*t)["font"]; fnode) {
    if (auto ft = fnode.as_table()) {
      config.input_prompt_font_family =
          getStr(*ft, "family", config.window_font_family);
      std::string fallback_path =
          ft->contains("family") ? "" : config.window_font_path;
      config.input_prompt_font_path = getStr(*ft, "path", fallback_path);
      config.input_prompt_font_size =
          getInt(*ft, "size", config.window_font_size);
      config.input_prompt_font_weight =
          getStr(*ft, "weight", config.window_font_weight);
    }
  } else {
    config.input_prompt_font_family = config.window_font_family;
    config.input_prompt_font_path = config.window_font_path;
    config.input_prompt_font_size = config.window_font_size;
    config.input_prompt_font_weight = config.window_font_weight;
  }

  config.input_prompt_color = getFontColor(*t, tc, config.input_prompt_color);
  config.input_prompt_background =
      getColor(*t, "background", tc, config.input_prompt_background);

  if (auto bnode = (*t)["border"]; bnode) {
    if (auto bt = bnode.as_table()) {
      config.input_prompt_border_radius =
          getInt(*bt, "radius", config.input_prompt_border_radius);
      config.input_prompt_border_width =
          getInt(*bt, "width", config.input_prompt_border_width);
      config.input_prompt_border_color =
          getColor(*bt, "color", tc, config.input_prompt_border_color);
    }
  }

  config.input_prompt_padding =
      getPadding(*t, "padding", config.input_prompt_padding);
  config.input_prompt_margin =
      getPadding(*t, "margin", config.input_prompt_margin);
}

void Manager::Impl::ApplyResults(const toml::table &root) {
  auto *t = getTable(root, "widget.results");
  if (!t)
    return;

  auto &tc = config.theme_colors;

  config.results_margin = getPadding(*t, "margin", config.results_margin);
  config.results_padding = getPadding(*t, "padding", config.results_padding);
  config.results_gap = getInt(*t, "gap", config.results_gap);
  config.results_background =
      getColor(*t, "background", tc, config.results_background);

  if (auto bnode = (*t)["border"]; bnode) {
    if (auto bt = bnode.as_table()) {
      config.results_border_radius =
          getInt(*bt, "radius", config.results_border_radius);
      config.results_border_width =
          getInt(*bt, "width", config.results_border_width);
      config.results_border_color =
          getColor(*bt, "color", tc, config.results_border_color);
    }
  }

  config.results_scroll = getStr(*t, "scroll", config.results_scroll);
  config.results_reverse = getBool(*t, "reverse", config.results_reverse);
  config.results_limit = getInt(*t, "limit", config.results_limit);
  config.results_show_help = getBool(*t, "show-help", config.results_show_help);

  if (auto *sb = getTable(root, "widget.results.scrollbar")) {
    config.results_scrollbar_enable =
        getBool(*sb, "enable", config.results_scrollbar_enable);

    if (auto thumb_node = (*sb)["thumb"]; thumb_node) {
      if (auto thumb_tbl = thumb_node.as_table()) {
        config.results_scrollbar_thumb_color = getColor(
            *thumb_tbl, "color", tc, config.results_scrollbar_thumb_color);
        config.results_scrollbar_thumb_radius =
            getInt(*thumb_tbl, "radius", config.results_scrollbar_thumb_radius);
      }
    }

    if (auto track_node = (*sb)["track"]; track_node) {
      if (auto track_tbl = track_node.as_table()) {
        config.results_scrollbar_track_width =
            getInt(*track_tbl, "width", config.results_scrollbar_track_width);
        config.results_scrollbar_track_color = getColor(
            *track_tbl, "color", tc, config.results_scrollbar_track_color);
        config.results_scrollbar_track_padding = getPadding(
            *track_tbl, "padding", config.results_scrollbar_track_padding);
        config.results_scrollbar_track_margin = getPadding(
            *track_tbl, "margin", config.results_scrollbar_track_margin);
        config.results_scrollbar_track_radius =
            getInt(*track_tbl, "radius", config.results_scrollbar_track_radius);
      }
    }
  }
}

void Manager::Impl::ApplyResultItem(const toml::table &root) {
  auto &tc = config.theme_colors;

  if (auto *t = getTable(root, "widget.results.item")) {
    config.result_item_comment_enable =
        getBool(*t, "comment", config.result_item_comment_enable);
    config.result_item_highlight_enable =
        getBool(*t, "highlight", config.result_item_highlight_enable);

    if (auto inode = (*t)["icon"]; inode) {
      if (auto it = inode.as_table()) {
        config.result_item_icon_size =
            getInt(*it, "size", config.result_item_icon_size);
        config.result_item_icon_gap =
            getInt(*it, "gap", config.result_item_icon_gap);
        config.result_item_icon_enable =
            getBool(*it, "enable", config.result_item_icon_enable);
      }
    }
  }

  if (auto *t = getTable(root, "widget.results.item.default")) {
    if (auto fnode = (*t)["font"]; fnode) {
      if (auto ft = fnode.as_table()) {
        config.result_item_default_font_family =
            getStr(*ft, "family", config.window_font_family);
        std::string fallback_path =
            ft->contains("family") ? "" : config.window_font_path;
        config.result_item_default_font_path =
            getStr(*ft, "path", fallback_path);
        config.result_item_default_font_size =
            getInt(*ft, "size", config.window_font_size);
        config.result_item_default_font_weight =
            getStr(*ft, "weight", config.window_font_weight);
      }
    } else {
      config.result_item_default_font_family = config.window_font_family;
      config.result_item_default_font_path = config.window_font_path;
      config.result_item_default_font_size = config.window_font_size;
      config.result_item_default_font_weight = config.window_font_weight;
    }
    config.result_item_default_color =
        getFontColor(*t, tc, config.result_item_default_color);
    config.result_item_default_background =
        getColor(*t, "background", tc, config.result_item_default_background);

    if (auto bnode = (*t)["border"]; bnode) {
      if (auto bt = bnode.as_table()) {
        config.result_item_default_border_radius =
            getInt(*bt, "radius", config.result_item_default_border_radius);
        config.result_item_default_border_width =
            getInt(*bt, "width", config.result_item_default_border_width);
        config.result_item_default_border_color =
            getColor(*bt, "color", tc, config.result_item_default_border_color);
      }
    }
    config.result_item_default_padding =
        getPadding(*t, "padding", config.result_item_default_padding);
    config.result_item_default_margin =
        getPadding(*t, "margin", config.result_item_default_margin);
    config.result_item_default_align =
        getStr(*t, "align", config.result_item_default_align);

    if (auto cnode = (*t)["comment"]; cnode) {
      if (auto ct = cnode.as_table()) {
        if (auto cfnode = (*ct)["font"]; cfnode) {
          if (auto cft = cfnode.as_table()) {
            config.result_item_default_comment_font_family =
                getStr(*cft, "family", config.window_font_family);
            std::string fallback_path =
                cft->contains("family") ? "" : config.window_font_path;
            config.result_item_default_comment_font_path =
                getStr(*cft, "path", fallback_path);
            config.result_item_default_comment_font_size =
                getInt(*cft, "size", config.window_font_size);
            config.result_item_default_comment_font_weight =
                getStr(*cft, "weight", config.window_font_weight);
          }
        } else {
          config.result_item_default_comment_font_family =
              config.window_font_family;
          config.result_item_default_comment_font_path =
              config.window_font_path;
          config.result_item_default_comment_font_size =
              config.window_font_size;
          config.result_item_default_comment_font_weight =
              config.window_font_weight;
        }
        config.result_item_default_comment_color =
            getFontColor(*ct, tc, config.result_item_default_comment_color);
      }
    }

    if (auto hnode = (*t)["highlight"]; hnode) {
      if (auto ht = hnode.as_table()) {
        if (auto hfnode = (*ht)["font"]; hfnode) {
          if (auto hft = hfnode.as_table()) {
            config.result_item_default_highlight_font_family =
                getStr(*hft, "family", config.window_font_family);
            std::string fallback_path =
                hft->contains("family") ? "" : config.window_font_path;
            config.result_item_default_highlight_font_path =
                getStr(*hft, "path", fallback_path);
            config.result_item_default_highlight_font_size =
                getInt(*hft, "size", config.window_font_size);
            config.result_item_default_highlight_font_weight =
                getStr(*hft, "weight", config.window_font_weight);
          }
        } else {
          config.result_item_default_highlight_font_family =
              config.window_font_family;
          config.result_item_default_highlight_font_path =
              config.window_font_path;
          config.result_item_default_highlight_font_size =
              config.window_font_size;
          config.result_item_default_highlight_font_weight =
              config.window_font_weight;
        }
        config.result_item_default_highlight_color =
            getFontColor(*ht, tc, config.result_item_default_highlight_color);
      }
    }

    config.result_item_selected_font_family =
        config.result_item_default_font_family;
    config.result_item_selected_font_path =
        config.result_item_default_font_path;
    config.result_item_selected_font_size =
        config.result_item_default_font_size;
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
    config.result_item_selected_comment_font_family =
        config.result_item_default_comment_font_family;
    config.result_item_selected_comment_font_path =
        config.result_item_default_comment_font_path;
    config.result_item_selected_comment_font_size =
        config.result_item_default_comment_font_size;
    config.result_item_selected_comment_font_weight =
        config.result_item_default_comment_font_weight;
    config.result_item_selected_comment_color =
        config.result_item_default_comment_color;
    config.result_item_selected_highlight_font_family =
        config.result_item_default_highlight_font_family;
    config.result_item_selected_highlight_font_path =
        config.result_item_default_highlight_font_path;
    config.result_item_selected_highlight_font_size =
        config.result_item_default_highlight_font_size;
    config.result_item_selected_highlight_font_weight =
        config.result_item_default_highlight_font_weight;
    config.result_item_selected_highlight_color =
        config.result_item_default_highlight_color;
  }

  if (auto *t = getTable(root, "widget.results.item.selected")) {
    if (auto fnode = (*t)["font"]; fnode) {
      if (auto ft = fnode.as_table()) {
        config.result_item_selected_font_family =
            getStr(*ft, "family", config.result_item_selected_font_family);
        std::string fallback_path =
            ft->contains("family") ? "" : config.result_item_selected_font_path;
        config.result_item_selected_font_path =
            getStr(*ft, "path", fallback_path);
        config.result_item_selected_font_size =
            getInt(*ft, "size", config.result_item_selected_font_size);
        config.result_item_selected_font_weight =
            getStr(*ft, "weight", config.result_item_selected_font_weight);
      }
    }

    config.result_item_selected_color =
        getFontColor(*t, tc, config.result_item_selected_color);
    config.result_item_selected_background =
        getColor(*t, "background", tc, config.result_item_selected_background);

    if (auto cnode = (*t)["comment"]; cnode) {
      if (auto ct = cnode.as_table()) {
        if (auto cfnode = (*ct)["font"]; cfnode) {
          if (auto cft = cfnode.as_table()) {
            config.result_item_selected_comment_font_family =
                getStr(*cft, "family", config.window_font_family);
            std::string fallback_path =
                cft->contains("family") ? "" : config.window_font_path;
            config.result_item_selected_comment_font_path =
                getStr(*cft, "path", fallback_path);
            config.result_item_selected_comment_font_size =
                getInt(*cft, "size", config.window_font_size);
            config.result_item_selected_comment_font_weight =
                getStr(*cft, "weight", config.window_font_weight);
          }
        } else {
          config.result_item_selected_comment_font_family =
              config.window_font_family;
          config.result_item_selected_comment_font_path =
              config.window_font_path;
          config.result_item_selected_comment_font_size =
              config.window_font_size;
          config.result_item_selected_comment_font_weight =
              config.window_font_weight;
        }
        config.result_item_selected_comment_color =
            getFontColor(*ct, tc, config.result_item_selected_comment_color);
      }
    }

    if (auto hnode = (*t)["highlight"]; hnode) {
      if (auto ht = hnode.as_table()) {
        if (auto hfnode = (*ht)["font"]; hfnode) {
          if (auto hft = hfnode.as_table()) {
            config.result_item_selected_highlight_font_family =
                getStr(*hft, "family", config.window_font_family);
            std::string fallback_path =
                hft->contains("family") ? "" : config.window_font_path;
            config.result_item_selected_highlight_font_path =
                getStr(*hft, "path", fallback_path);
            config.result_item_selected_highlight_font_size =
                getInt(*hft, "size", config.window_font_size);
            config.result_item_selected_highlight_font_weight =
                getStr(*hft, "weight", config.window_font_weight);
          }
        } else {
          config.result_item_selected_highlight_font_family =
              config.window_font_family;
          config.result_item_selected_highlight_font_path =
              config.window_font_path;
          config.result_item_selected_highlight_font_size =
              config.window_font_size;
          config.result_item_selected_highlight_font_weight =
              config.window_font_weight;
        }
        config.result_item_selected_highlight_color =
            getFontColor(*ht, tc, config.result_item_selected_highlight_color);
      }
    }

    if (auto bnode = (*t)["border"]; bnode) {
      if (auto bt = bnode.as_table()) {
        config.result_item_selected_border_radius =
            getInt(*bt, "radius", config.result_item_selected_border_radius);
        config.result_item_selected_border_width =
            getInt(*bt, "width", config.result_item_selected_border_width);
        config.result_item_selected_border_color = getColor(
            *bt, "color", tc, config.result_item_selected_border_color);
      }
    }

    config.result_item_selected_padding =
        getPadding(*t, "padding", config.result_item_selected_padding);
    config.result_item_selected_margin =
        getPadding(*t, "margin", config.result_item_selected_margin);
    config.result_item_selected_align =
        getStr(*t, "align", config.result_item_selected_align);
  }
}

void Manager::Impl::ApplyResultsCount(const toml::table &root) {
  auto *t = getTable(root, "widget.results.count");
  if (!t)
    return;

  auto &tc = config.theme_colors;

  config.results_count_enable =
      getBool(*t, "enable", config.results_count_enable);
  config.results_count_format =
      getStr(*t, "format", config.results_count_format);

  if (auto fnode = (*t)["font"]; fnode) {
    if (auto ft = fnode.as_table()) {
      config.results_count_font_family =
          getStr(*ft, "family", config.window_font_family);
      std::string fallback_path =
          ft->contains("family") ? "" : config.window_font_path;
      config.results_count_font_path = getStr(*ft, "path", fallback_path);
      config.results_count_font_size =
          getInt(*ft, "size", config.window_font_size);
      config.results_count_font_weight =
          getStr(*ft, "weight", config.window_font_weight);
    }
  } else {
    config.results_count_font_family = config.window_font_family;
    config.results_count_font_path = config.window_font_path;
    config.results_count_font_size = config.window_font_size;
    config.results_count_font_weight = config.window_font_weight;
  }

  config.results_count_color = getFontColor(*t, tc, config.results_count_color);
  config.results_count_align = getStr(*t, "align", config.results_count_align);
  config.results_count_padding =
      getPadding(*t, "padding", config.results_count_padding);
  config.results_count_margin =
      getPadding(*t, "margin", config.results_count_margin);
}

void Manager::Impl::ApplyPreview(const toml::table &root) {
  auto &tc = config.theme_colors;

  if (auto *t = getTable(root, "widget.preview")) {
    config.preview_enable = getBool(*t, "enable", config.preview_enable);
    config.preview_composition =
        getStrArray(*t, "composition", config.preview_composition);
    config.preview_image_size =
        getInt(*t, "image-size", config.preview_image_size);
    config.preview_background =
        getColor(*t, "background", tc, config.preview_background);
    config.preview_padding = getPadding(*t, "padding", config.preview_padding);
    config.preview_margin = getPadding(*t, "margin", config.preview_margin);

    if (auto gnode = (*t)["gap"]; gnode) {
      if (auto gt = gnode.as_table()) {
        config.preview_gap_v = getInt(*gt, "v", config.preview_gap_v);
        config.preview_gap_h = getInt(*gt, "h", config.preview_gap_h);
      } else if (auto gi = gnode.as_integer()) {
        int g = static_cast<int>(gi->get());
        config.preview_gap_v = g;
        config.preview_gap_h = g;
      }
    }
  }

  if (auto *t = getTable(root, "widget.preview.icon")) {
    config.preview_icon_size = getInt(*t, "size", config.preview_icon_size);
    config.preview_icon_fallback =
        getBool(*t, "fallback", config.preview_icon_fallback);
    config.preview_icon_hide_on_fallback =
        getBool(*t, "hide-on-fallback", config.preview_icon_hide_on_fallback);
  }

  if (auto *t = getTable(root, "widget.preview.name")) {
    if (auto fnode = (*t)["font"]; fnode) {
      if (auto ft = fnode.as_table()) {
        config.preview_name_font_family =
            getStr(*ft, "family", config.window_font_family);
        std::string fallback_path =
            ft->contains("family") ? "" : config.window_font_path;
        config.preview_name_font_path = getStr(*ft, "path", fallback_path);
        config.preview_name_font_size =
            getInt(*ft, "size", config.window_font_size);
        config.preview_name_font_weight =
            getStr(*ft, "weight", config.window_font_weight);
      }
    } else {
      config.preview_name_font_family = config.window_font_family;
      config.preview_name_font_path = config.window_font_path;
      config.preview_name_font_size = config.window_font_size;
      config.preview_name_font_weight = config.window_font_weight;
    }
    config.preview_name_color = getFontColor(*t, tc, config.preview_name_color);
  }

  if (auto *t = getTable(root, "widget.preview.comment")) {
    if (auto fnode = (*t)["font"]; fnode) {
      if (auto ft = fnode.as_table()) {
        config.preview_comment_font_family =
            getStr(*ft, "family", config.window_font_family);
        std::string fallback_path =
            ft->contains("family") ? "" : config.window_font_path;
        config.preview_comment_font_path = getStr(*ft, "path", fallback_path);
        config.preview_comment_font_size =
            getInt(*ft, "size", config.window_font_size);
        config.preview_comment_font_weight =
            getStr(*ft, "weight", config.window_font_weight);
      }
    } else {
      config.preview_comment_font_family = config.window_font_family;
      config.preview_comment_font_path = config.window_font_path;
      config.preview_comment_font_size = config.window_font_size;
      config.preview_comment_font_weight = config.window_font_weight;
    }
    config.preview_comment_color =
        getFontColor(*t, tc, config.preview_comment_color);
  }
}

void Manager::Impl::ApplyClock(const toml::table &root) {
  auto *t = getTable(root, "widget.clock");
  if (!t)
    return;

  auto &tc = config.theme_colors;

  config.clock_enable = getBool(*t, "enable", config.clock_enable);
  config.clock_format = getStr(*t, "format", config.clock_format);

  if (auto fnode = (*t)["font"]; fnode) {
    if (auto ft = fnode.as_table()) {
      config.clock_font_family =
          getStr(*ft, "family", config.window_font_family);
      std::string fallback_path =
          ft->contains("family") ? "" : config.window_font_path;
      config.clock_font_path = getStr(*ft, "path", fallback_path);
      config.clock_font_size = getInt(*ft, "size", config.window_font_size);
      config.clock_font_weight =
          getStr(*ft, "weight", config.window_font_weight);
    }
  } else {
    config.clock_font_family = config.window_font_family;
    config.clock_font_path = config.window_font_path;
    config.clock_font_size = config.window_font_size;
    config.clock_font_weight = config.window_font_weight;
  }

  config.clock_color = getFontColor(*t, tc, config.clock_color);
  config.clock_align = getStr(*t, "align", config.clock_align);
  config.clock_padding = getPadding(*t, "padding", config.clock_padding);
  config.clock_margin = getPadding(*t, "margin", config.clock_margin);
}

void Manager::Impl::ApplyBreadcrumbs(const toml::table &root) {
  auto *t = getTable(root, "widget.breadcrumbs");
  if (!t)
    return;

  auto &tc = config.theme_colors;

  config.breadcrumbs_enable = getBool(*t, "enable", config.breadcrumbs_enable);
  config.breadcrumbs_separator =
      getStr(*t, "separator", config.breadcrumbs_separator);

  if (auto fnode = (*t)["font"]; fnode) {
    if (auto ft = fnode.as_table()) {
      config.breadcrumbs_font_family =
          getStr(*ft, "family", config.window_font_family);
      std::string fallback_path =
          ft->contains("family") ? "" : config.window_font_path;
      config.breadcrumbs_font_path = getStr(*ft, "path", fallback_path);
      config.breadcrumbs_font_size =
          getInt(*ft, "size", config.window_font_size);
      config.breadcrumbs_font_weight =
          getStr(*ft, "weight", config.window_font_weight);
    }
  } else {
    config.breadcrumbs_font_family = config.window_font_family;
    config.breadcrumbs_font_path = config.window_font_path;
    config.breadcrumbs_font_size = config.window_font_size;
    config.breadcrumbs_font_weight = config.window_font_weight;
  }

  config.breadcrumbs_color = getFontColor(*t, tc, config.breadcrumbs_color);
  config.breadcrumbs_align = getStr(*t, "align", config.breadcrumbs_align);
  config.breadcrumbs_padding =
      getPadding(*t, "padding", config.breadcrumbs_padding);
  config.breadcrumbs_margin =
      getPadding(*t, "margin", config.breadcrumbs_margin);
}

void Manager::Impl::ApplyModifiers(const toml::table &root) {
  if (auto arr = root["modifier"].as_array()) {
    config.modifiers.clear();
    for (auto &node : *arr) {
      if (auto t = node.as_table()) {
        Modifier m;
        m.trigger = getStr(*t, "trigger", "");
        m.expanded = getStr(*t, "expanded", "");
        m.type = getStr(*t, "type", "abbr");
        if (auto s = (*t)["scope"]) {
          if (auto s_arr = s.as_array()) {
            for (auto &el : *s_arr) {
              if (auto str = el.as_string()) {
                m.scope.push_back(str->get());
              }
            }
          }
        }
        if (m.scope.empty()) {
          m.scope.push_back("*");
        }
        config.modifiers.push_back(m);
      }
    }
  }
}

void Manager::Impl::ApplyProviders(const toml::table &root) {
  if (auto *apps = getTable(root, "providers.apps")) {
    config.providers_apps_command =
        getStr(*apps, "command", config.providers_apps_command);
    config.providers_apps_uwsm =
        getBool(*apps, "uwsm", config.providers_apps_uwsm);
    config.providers_apps_uwsm_prefix =
        getStr(*apps, "uwsm-prefix", config.providers_apps_uwsm_prefix);
    config.providers_apps_history =
        getBool(*apps, "history", config.providers_apps_history);
  }

  if (auto *bins = getTable(root, "providers.bins")) {
    config.providers_bins_exec =
        getStr(*bins, "exec", config.providers_bins_exec);
    config.providers_bins_history =
        getBool(*bins, "history", config.providers_bins_history);
    config.providers_bins_terminal_exec =
        getBool(*bins, "terminal-exec", config.providers_bins_terminal_exec);
  }
}

void Manager::Impl::ApplyPlugins(const toml::table &root) {
  auto *t = getTable(root, "plugin");
  if (!t)
    return;

  for (auto &[key, val] : *t) {
    std::string plugin_name(key.str());
    auto pt = val.as_table();
    if (!pt)
      continue;

    if (getBool(*pt, "enable", false)) {
      bool found = false;
      for (auto &p : config.enabled_plugins) {
        if (p == plugin_name) {
          found = true;
          break;
        }
      }
      if (!found)
        config.enabled_plugins.push_back(plugin_name);
    }

    for (auto &[pk, pv] : *pt) {
      std::string pk_str(pk.str());
      if (pk_str == "enable")
        continue;

      std::string cfg_key = plugin_name + "." + pk_str;
      if (auto s = pv.as_string()) {
        config.plugin_configs[cfg_key] = s->get();
      } else if (auto i = pv.as_integer()) {
        config.plugin_configs[cfg_key] = std::to_string(i->get());
      } else if (auto b = pv.as_boolean()) {
        config.plugin_configs[cfg_key] = b->get() ? "true" : "false";
      } else if (auto arr = pv.as_array()) {
        std::string joined;
        for (size_t idx = 0; idx < arr->size(); idx++) {
          if (idx > 0)
            joined += ", ";
          if (auto sv = (*arr)[idx].as_string())
            joined += sv->get();
        }
        config.plugin_configs[cfg_key] = joined;
      }
    }
  }
}

} // namespace Lawnch::Core::Config
