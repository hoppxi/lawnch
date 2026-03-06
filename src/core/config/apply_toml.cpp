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
  ApplyProviders(root);
  ApplyPlugins(root);
}

void Manager::Impl::ApplyGeneral(const toml::table &root) {
  auto *t = getTable(root, "general");
  if (!t)
    return;

  config.general_icon_theme =
      getStr(*t, "icon-theme", config.general_icon_theme);
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
  config.window_anchor = getStr(*t, "anchor", config.window_anchor);
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
}

void Manager::Impl::ApplyInput(const toml::table &root) {
  auto *t = getTable(root, "widget.input");
  if (!t)
    return;

  auto &tc = config.theme_colors;

  if (auto fnode = (*t)["font"]; fnode) {
    if (auto ft = fnode.as_table()) {
      config.input_font_family =
          getStr(*ft, "family", config.input_font_family);
      config.input_font_size = getInt(*ft, "size", config.input_font_size);
      config.input_font_weight =
          getStr(*ft, "weight", config.input_font_weight);
    }
  }

  config.input_text = getColor(*t, "text", tc, config.input_text);
  config.input_background =
      getColor(*t, "background", tc, config.input_background);

  if (auto pnode = (*t)["placeholder"]; pnode) {
    if (auto pt = pnode.as_table()) {
      config.input_placeholder_text =
          getStr(*pt, "text", config.input_placeholder_text);
      config.input_placeholder_color =
          getColor(*pt, "color", tc, config.input_placeholder_color);
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
      config.input_selection_color =
          getColor(*st, "color", tc, config.input_selection_color);
      config.input_selection_text =
          getColor(*st, "text", tc, config.input_selection_text);
    }
  }

  config.input_padding = getPadding(*t, "padding", config.input_padding);
  config.input_align = getStr(*t, "align", config.input_align);
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
          getStr(*ft, "family", config.input_prompt_font_family);
      config.input_prompt_font_size =
          getInt(*ft, "size", config.input_prompt_font_size);
      config.input_prompt_font_weight =
          getStr(*ft, "weight", config.input_prompt_font_weight);
    }
  }

  config.input_prompt_color =
      getColor(*t, "color", tc, config.input_prompt_color);
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
    config.results_scrollbar_width =
        getInt(*sb, "width", config.results_scrollbar_width);
    config.results_scrollbar_padding =
        getInt(*sb, "padding", config.results_scrollbar_padding);
    config.results_scrollbar_radius =
        getInt(*sb, "radius", config.results_scrollbar_radius);
    config.results_scrollbar_thumb =
        getColor(*sb, "thumb", tc, config.results_scrollbar_thumb);
    config.results_scrollbar_track =
        getColor(*sb, "track", tc, config.results_scrollbar_track);
  }
}

void Manager::Impl::ApplyResultItem(const toml::table &root) {
  auto &tc = config.theme_colors;

  if (auto *t = getTable(root, "widget.results.item")) {
    if (auto fnode = (*t)["font"]; fnode) {
      if (auto ft = fnode.as_table()) {
        config.result_item_font_family =
            getStr(*ft, "family", config.result_item_font_family);
        config.result_item_font_size =
            getInt(*ft, "size", config.result_item_font_size);
        config.result_item_font_weight =
            getStr(*ft, "weight", config.result_item_font_weight);
      }
    }
    config.result_item_text = getColor(*t, "text", tc, config.result_item_text);
    config.result_item_background =
        getColor(*t, "background", tc, config.result_item_background);

    if (auto bnode = (*t)["border"]; bnode) {
      if (auto bt = bnode.as_table()) {
        config.result_item_border_radius =
            getInt(*bt, "radius", config.result_item_border_radius);
        config.result_item_border_width =
            getInt(*bt, "width", config.result_item_border_width);
        config.result_item_border_color =
            getColor(*bt, "color", tc, config.result_item_border_color);
      }
    }
    config.result_item_padding =
        getPadding(*t, "padding", config.result_item_padding);
    config.result_item_align = getStr(*t, "align", config.result_item_align);

    if (auto inode = (*t)["icon"]; inode) {
      if (auto it = inode.as_table()) {
        config.result_item_icon_size =
            getInt(*it, "size", config.result_item_icon_size);
        config.result_item_icon_gap =
            getInt(*it, "gap", config.result_item_icon_gap);
        config.result_item_icon_show =
            getBool(*it, "show", config.result_item_icon_show);
      }
    }
  }

  if (auto *t = getTable(root, "widget.results.item.comment")) {
    config.result_item_comment_enable =
        getBool(*t, "enable", config.result_item_comment_enable);
    if (auto fnode = (*t)["font"]; fnode) {
      if (auto ft = fnode.as_table()) {
        config.result_item_comment_font_size =
            getInt(*ft, "size", config.result_item_comment_font_size);
        config.result_item_comment_font_weight =
            getStr(*ft, "weight", config.result_item_comment_font_weight);
      }
    }
    config.result_item_comment_color =
        getColor(*t, "text", tc, config.result_item_comment_color);
  }

  if (auto *t = getTable(root, "widget.results.item.highlight")) {
    config.result_item_highlight_enable =
        getBool(*t, "enable", config.result_item_highlight_enable);
    if (auto fnode = (*t)["font"]; fnode) {
      if (auto ft = fnode.as_table())
        config.result_item_highlight_font_weight =
            getStr(*ft, "weight", config.result_item_highlight_font_weight);
    }
    config.result_item_highlight_color =
        getColor(*t, "text", tc, config.result_item_highlight_color);
  }

  if (auto *t = getTable(root, "widget.results.item.selected")) {
    config.result_item_selected_text =
        getColor(*t, "text", tc, config.result_item_selected_text);
    config.result_item_selected_background =
        getColor(*t, "background", tc, config.result_item_selected_background);
    config.result_item_selected_comment =
        getColor(*t, "comment", tc, config.result_item_selected_comment);
    config.result_item_selected_highlight =
        getColor(*t, "highlight", tc, config.result_item_selected_highlight);

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
          getStr(*ft, "family", config.results_count_font_family);
      config.results_count_font_size =
          getInt(*ft, "size", config.results_count_font_size);
      config.results_count_font_weight =
          getStr(*ft, "weight", config.results_count_font_weight);
    }
  }

  config.results_count_text =
      getColor(*t, "text", tc, config.results_count_text);
  config.results_count_align = getStr(*t, "align", config.results_count_align);
  config.results_count_padding =
      getPadding(*t, "padding", config.results_count_padding);
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

    if (auto inode = (*t)["icon"]; inode) {
      if (auto it = inode.as_table()) {
        config.preview_icon_size =
            getInt(*it, "size", config.preview_icon_size);
        config.preview_icon_fallback =
            getBool(*it, "fallback", config.preview_icon_fallback);
        config.preview_icon_hide_on_fallback = getBool(
            *it, "hide-on-fallback", config.preview_icon_hide_on_fallback);
      }
    }

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

  if (auto *t = getTable(root, "widget.preview.title")) {
    if (auto fnode = (*t)["font"]; fnode) {
      if (auto ft = fnode.as_table()) {
        config.preview_title_font_family =
            getStr(*ft, "family", config.preview_title_font_family);
        config.preview_title_font_size =
            getInt(*ft, "size", config.preview_title_font_size);
        config.preview_title_font_weight =
            getStr(*ft, "weight", config.preview_title_font_weight);
      }
    }
    config.preview_title_color =
        getColor(*t, "text", tc, config.preview_title_color);
  }

  if (auto *t = getTable(root, "widget.preview.description")) {
    if (auto fnode = (*t)["font"]; fnode) {
      if (auto ft = fnode.as_table()) {
        config.preview_description_font_size =
            getInt(*ft, "size", config.preview_description_font_size);
        config.preview_description_font_weight =
            getStr(*ft, "weight", config.preview_description_font_weight);
      }
    }
    config.preview_description_color =
        getColor(*t, "text", tc, config.preview_description_color);
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
          getStr(*ft, "family", config.clock_font_family);
      config.clock_font_size = getInt(*ft, "size", config.clock_font_size);
      config.clock_font_weight =
          getStr(*ft, "weight", config.clock_font_weight);
    }
  }

  config.clock_text = getColor(*t, "text", tc, config.clock_text);
  config.clock_align = getStr(*t, "align", config.clock_align);
  config.clock_padding = getPadding(*t, "padding", config.clock_padding);
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
