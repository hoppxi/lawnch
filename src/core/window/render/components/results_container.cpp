#include "results_container.hpp"
#include "../../../../helpers/gfx.hpp"
#include "../../../icons/manager.hpp"
#include "../render_state.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace Lawnch::Core::Window::Render::Components {

void ResultsContainer::update_metrics(const Config::Config &cfg) const {
  BLFont font = Lawnch::Gfx::get_font(cfg.result_item_default_font_family,
                                      cfg.result_item_default_font_size,
                                      cfg.result_item_default_font_weight,
                                      cfg.result_item_default_font_path);
  BLFontMetrics metrics = font.metrics();
  double name_h = metrics.ascent + metrics.descent;

  double comment_h = 0;
  if (cfg.result_item_comment_enable) {
    BLFont cfont = Lawnch::Gfx::get_font(cfg.result_item_default_comment_font_family,
                                         cfg.result_item_default_comment_font_size,
                                         cfg.result_item_default_comment_font_weight,
                                         cfg.result_item_default_comment_font_path);
    BLFontMetrics cmetrics = cfont.metrics();
    comment_h = cmetrics.ascent + cmetrics.descent;
  }

  double text_gap = 4.0;
  double item_inner_h = name_h;
  if (cfg.result_item_comment_enable) {
    item_inner_h += text_gap + comment_h;
  }

  cached_item_height =
      (cfg.result_item_default_padding.top + cfg.result_item_default_padding.bottom) +
      item_inner_h + cfg.results_gap + cfg.result_item_default_margin.top +
      cfg.result_item_default_margin.bottom;
  metrics_valid = true;
}

double ResultsContainer::get_item_height(const Config::Config &cfg) const {
  if (!metrics_valid)
    update_metrics(cfg);
  return cached_item_height;
}

void ResultsContainer::draw_result_item(BLContext &ctx,
                                        const Config::Config &cfg,
                                        const Search::SearchResult &result,
                                        double item_x, double item_y,
                                        double item_w, double item_h,
                                        bool is_selected,
                                        const std::string &search_text) const {

  auto family = is_selected ? cfg.result_item_selected_font_family
                            : cfg.result_item_default_font_family;
  auto size = is_selected ? cfg.result_item_selected_font_size
                          : cfg.result_item_default_font_size;
  auto weight = is_selected ? cfg.result_item_selected_font_weight
                            : cfg.result_item_default_font_weight;

  auto path = is_selected ? cfg.result_item_selected_font_path
                          : cfg.result_item_default_font_path;

  BLFont font = Lawnch::Gfx::get_font(family, size, weight, path);
  BLFontMetrics fm = font.metrics();

  auto c_size = is_selected ? cfg.result_item_selected_comment_font_size
                            : cfg.result_item_default_comment_font_size;
  auto c_weight = is_selected ? cfg.result_item_selected_comment_font_weight
                              : cfg.result_item_default_comment_font_weight;

  auto c_family = is_selected ? cfg.result_item_selected_comment_font_family
                              : cfg.result_item_default_comment_font_family;
  auto c_path = is_selected ? cfg.result_item_selected_comment_font_path
                            : cfg.result_item_default_comment_font_path;

  BLFont comment_font = Lawnch::Gfx::get_font(c_family, c_size, c_weight, c_path);
  BLFontMetrics cm = comment_font.metrics();

  double center_y = item_y + (item_h / 2.0);

  auto bg_color = is_selected ? cfg.result_item_selected_background
                              : cfg.result_item_default_background;
  auto border_color = is_selected ? cfg.result_item_selected_border_color
                                  : cfg.result_item_default_border_color;
  auto text_color = is_selected ? cfg.result_item_selected_color
                                : cfg.result_item_default_color;
  auto comment_color_cfg = is_selected ? cfg.result_item_selected_comment_color
                                       : cfg.result_item_default_comment_color;

  int radius = is_selected ? cfg.result_item_selected_border_radius
                           : cfg.result_item_default_border_radius;
  int border_w = is_selected ? cfg.result_item_selected_border_width
                             : cfg.result_item_default_border_width;

  auto margin = is_selected ? cfg.result_item_selected_margin
                            : cfg.result_item_default_margin;
  auto padding = is_selected ? cfg.result_item_selected_padding
                             : cfg.result_item_default_padding;
  auto align = is_selected ? cfg.result_item_selected_align
                           : cfg.result_item_default_align;

  double draw_x_rect = item_x + margin.left;
  double draw_y_rect = item_y + margin.top;
  double draw_w_rect = item_w - (margin.left + margin.right);
  double draw_h_rect = item_h - (margin.top + margin.bottom);

  BLRoundRect item_rect = Lawnch::Gfx::rounded_rect(
      draw_x_rect, draw_y_rect, draw_w_rect, draw_h_rect, radius);

  if (bg_color.a > 0) {
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(bg_color));
    ctx.fill_round_rect(item_rect);
  }

  if (border_w > 0 && border_color.a > 0) {
    ctx.set_stroke_style(Lawnch::Gfx::toBLColor(border_color));
    ctx.set_stroke_width(border_w);
    ctx.stroke_round_rect(item_rect);
  }

  double draw_x = draw_x_rect + padding.left;
  double draw_w = draw_w_rect - (padding.left + padding.right);

  double current_icon_size = 0;
  if (cfg.result_item_icon_enable) {
    double max_icon_h = item_h * 0.8;
    current_icon_size = std::min((double)cfg.result_item_icon_size, max_icon_h);

    double icon_y = std::floor(center_y - (current_icon_size / 2.0));

    Icons::Manager::Instance().render_icon(ctx, result.icon, std::floor(draw_x),
                                           icon_y, current_icon_size);

    double icon_gap = current_icon_size + cfg.result_item_icon_gap;
    draw_x += icon_gap;
    draw_w -= icon_gap;
  }

  if (result.is_pinned) {
    double pin_icon_size = current_icon_size > 0 ? current_icon_size * 0.7 : item_h * 0.5;
    double pin_x = draw_x_rect + draw_w_rect - padding.right - pin_icon_size;
    double pin_y = std::floor(center_y - (pin_icon_size / 2.0));
    Icons::Manager::Instance().render_icon(ctx, "view-pin", std::floor(pin_x), pin_y, pin_icon_size);
    draw_w -= (pin_icon_size + cfg.result_item_icon_gap);
  }

  if (draw_w > 0) {
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(text_color));
    double text_x_pos = draw_x;

    if (align == "center") {
      text_x_pos = item_x + (item_w / 2.0);
    } else if (align == "right") {
      text_x_pos = draw_x_rect + draw_w_rect - padding.right;
    }

    double name_y;
    double comment_y = 0;

    if (cfg.result_item_comment_enable && !result.comment.empty()) {
      double total_text_h = (fm.ascent + fm.descent) + (cm.ascent + cm.descent);
      double start_text_y = center_y - (total_text_h / 2.0);

      name_y = std::floor(start_text_y + fm.ascent);
      comment_y =
          std::floor(start_text_y + (fm.ascent + fm.descent) + cm.ascent);
    } else {
      name_y = std::floor(center_y + (fm.cap_height / 2.0));
    }

    ctx.save();
    ctx.clip_to_rect(BLRect(draw_x, draw_y_rect, draw_w, draw_h_rect));

    std::string display_name = result.name;
    if (align != "center" && align != "right") {
      display_name = Lawnch::Gfx::truncate_text(result.name, font, draw_w);
    }

    double final_text_x = text_x_pos;
    if (align == "center" || align == "right") {
      BLGlyphBuffer gb;
      gb.set_utf8_text(display_name.c_str(), display_name.size());
      font.shape(gb);
      BLTextMetrics tm;
      font.get_text_metrics(gb, tm);

      if (align == "center") {
        final_text_x -= (tm.advance.x / 2.0);
      } else {
        final_text_x -= tm.advance.x;
      }
    }
    if (cfg.result_item_highlight_enable && !search_text.empty()) {
      auto h_color = is_selected ? cfg.result_item_selected_highlight_color
                                 : cfg.result_item_default_highlight_color;

      auto h_family = is_selected ? cfg.result_item_selected_highlight_font_family
                                  : cfg.result_item_default_highlight_font_family;
      auto h_size = is_selected ? cfg.result_item_selected_highlight_font_size
                                : cfg.result_item_default_highlight_font_size;
      auto h_weight = is_selected ? cfg.result_item_selected_highlight_font_weight
                                  : cfg.result_item_default_highlight_font_weight;
      auto h_path = is_selected ? cfg.result_item_selected_highlight_font_path
                                : cfg.result_item_default_highlight_font_path;
      BLFont highlight_font = Lawnch::Gfx::get_font(
          h_family, h_size, h_weight, h_path);

      std::string query_term = search_text;
      if (!query_term.empty() && query_term[0] == ':') {
        size_t space_pos = query_term.find(' ');
        if (space_pos != std::string::npos) {
          query_term = query_term.substr(space_pos + 1);
        } else {
          query_term = "";
        }
      } else if (!query_term.empty() &&
                 (query_term[0] == '=' || query_term[0] == '>' ||
                  query_term[0] == '<' || query_term[0] == '!' ||
                  query_term[0] == '~')) {
        query_term = query_term.substr(1);
        while (!query_term.empty() && query_term[0] == ' ')
          query_term = query_term.substr(1);
      }

      std::string name_lower = display_name;
      std::string search_lower = query_term;
      for (auto &c : name_lower)
        c = std::tolower(static_cast<unsigned char>(c));
      for (auto &c : search_lower)
        c = std::tolower(static_cast<unsigned char>(c));

      std::vector<bool> highlight_mask(display_name.size(), false);
      size_t search_idx = 0;
      for (size_t i = 0;
           i < name_lower.size() && search_idx < search_lower.size(); ++i) {
        if (name_lower[i] == search_lower[search_idx]) {
          highlight_mask[i] = true;
          search_idx++;
        }
      }

      double x_pos = final_text_x;
      for (size_t i = 0; i < display_name.size(); ++i) {
        std::string ch(1, display_name[i]);
        BLFont &char_font = highlight_mask[i] ? highlight_font : font;
        auto char_color = highlight_mask[i] ? h_color : text_color;

        ctx.set_fill_style(Lawnch::Gfx::toBLColor(char_color));
        ctx.fill_utf8_text(BLPoint(x_pos, name_y), char_font, ch.c_str());

        BLGlyphBuffer char_gb;
        char_gb.set_utf8_text(ch.c_str());
        char_font.shape(char_gb);
        BLTextMetrics char_tm;
        char_font.get_text_metrics(char_gb, char_tm);
        x_pos += char_tm.advance.x;
      }
    } else {
      ctx.fill_utf8_text(BLPoint(final_text_x, name_y), font,
                         display_name.c_str());
    }

    if (cfg.result_item_comment_enable && !result.comment.empty()) {
      ctx.set_fill_style(Lawnch::Gfx::toBLColor(comment_color_cfg));

      std::string display_comment = result.comment;
      if (align != "center" && align != "right") {
        display_comment =
            Lawnch::Gfx::truncate_text(result.comment, comment_font, draw_w);
      }

      double final_comment_x = text_x_pos;
      if (align == "center" || align == "right") {
        BLGlyphBuffer gb;
        gb.set_utf8_text(display_comment.c_str(), display_comment.size());
        comment_font.shape(gb);
        BLTextMetrics tm;
        comment_font.get_text_metrics(gb, tm);

        if (align == "center") {
          final_comment_x -= (tm.advance.x / 2.0);
        } else {
          final_comment_x -= tm.advance.x;
        }
      }

      ctx.fill_utf8_text(BLPoint(final_comment_x, comment_y), comment_font,
                         display_comment.c_str());
    }

    ctx.restore();
  }
}

ComponentResult ResultsContainer::draw(ComponentContext &context) {
  auto &ctx = context.ctx;
  auto &cfg = context.cfg;
  auto &state = context.state;

  if (!metrics_valid)
    update_metrics(cfg);

  double item_height = cached_item_height;
  if (item_height <= 0)
    item_height = 32;

  double layout_y = context.y + cfg.results_margin.top;
  double available_h = context.available_h - cfg.results_margin.top -
                       cfg.results_margin.bottom - cfg.results_padding.top -
                       cfg.results_padding.bottom;

  int total_results = (int)state.results.size();
  int visible_count = std::max(1, (int)std::floor(available_h / item_height));

  bool show_scrollbar =
      cfg.results_scrollbar_enable && total_results > visible_count;

  double content_x =
      context.x + cfg.results_margin.left + cfg.results_padding.left;
  double content_w = context.available_w -
                     (cfg.results_margin.left + cfg.results_margin.right) -
                     (cfg.results_padding.left + cfg.results_padding.right);

  if (show_scrollbar) {
    content_w -=
        (cfg.results_scrollbar_track_width + cfg.results_scrollbar_track_padding.left + cfg.results_scrollbar_track_padding.right);
  }

  int end_index = std::min(total_results, state.scroll_offset + visible_count);

  double container_x = context.x + cfg.results_margin.left;
  double container_y = layout_y;
  double container_w = context.available_w -
                       (cfg.results_margin.left + cfg.results_margin.right);
  double container_h =
      context.available_h - cfg.results_margin.top - cfg.results_margin.bottom;

  if (cfg.results_background.a > 0) {
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.results_background));
    ctx.fill_round_rect(Lawnch::Gfx::rounded_rect(container_x, container_y,
                                                  container_w, container_h,
                                                  cfg.results_border_radius));
  }
  if (cfg.results_border_width > 0 && cfg.results_border_color.a > 0) {
    ctx.set_stroke_style(Lawnch::Gfx::toBLColor(cfg.results_border_color));
    ctx.set_stroke_width(cfg.results_border_width);
    ctx.stroke_round_rect(Lawnch::Gfx::rounded_rect(container_x, container_y,
                                                    container_w, container_h,
                                                    cfg.results_border_radius));
  }

  double items_start_y = layout_y + cfg.results_padding.top;

  int actual_visible = end_index - state.scroll_offset;

  double bottom_offset = 0;
  if (cfg.results_reverse && actual_visible < visible_count) {
    int empty_slots = visible_count - actual_visible;
    bottom_offset = empty_slots * item_height;
  }

  for (int i = state.scroll_offset; i < end_index; ++i) {
    const auto &res = state.results[i];
    int rel_i = i - state.scroll_offset;

    double item_y;
    if (cfg.results_reverse) {
      int reverse_rel_i = actual_visible - 1 - rel_i;
      item_y = std::floor(items_start_y + bottom_offset +
                          reverse_rel_i * item_height);
    } else {
      item_y = std::floor(items_start_y + rel_i * item_height);
    }

    double item_h = std::floor(item_height - cfg.results_gap);
    bool is_sel = (i == state.selected_index);

    draw_result_item(ctx, cfg, res, content_x, item_y, content_w, item_h,
                     is_sel, state.search_text);
  }

  if (show_scrollbar) {
    double track_x = context.x + context.available_w -
                     cfg.results_margin.right - cfg.results_padding.right -
                     cfg.results_scrollbar_track_width -
                     cfg.results_scrollbar_track_padding.right;
    double track_y = items_start_y + cfg.results_scrollbar_track_padding.top;
    double track_h = available_h - cfg.results_scrollbar_track_padding.top -
                     cfg.results_scrollbar_track_padding.bottom;

    if (cfg.results_scrollbar_track_color.a > 0) {
      ctx.set_fill_style(
          Lawnch::Gfx::toBLColor(cfg.results_scrollbar_track_color));
      ctx.fill_round_rect(Lawnch::Gfx::rounded_rect(
          track_x, track_y, cfg.results_scrollbar_track_width, track_h,
          cfg.results_scrollbar_track_radius));
    }

    double ratio = (double)visible_count / total_results;
    double thumb_h = std::max(20.0, track_h * ratio);
    double thumb_range = track_h - thumb_h;

    double scroll_progress =
        (double)state.scroll_offset / (total_results - visible_count);
    double thumb_y = track_y + (scroll_progress * thumb_range);

    double thumb_x = track_x + cfg.results_scrollbar_track_padding.left;
    double thumb_w = cfg.results_scrollbar_track_width - cfg.results_scrollbar_track_padding.left - cfg.results_scrollbar_track_padding.right;

    ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.results_scrollbar_thumb_color));
    ctx.fill_round_rect(
        Lawnch::Gfx::rounded_rect(thumb_x, thumb_y, thumb_w,
                                   thumb_h, cfg.results_scrollbar_thumb_radius));
  }

  return {context.available_w,
          container_h + cfg.results_margin.top + cfg.results_margin.bottom};
}

} // namespace Lawnch::Core::Window::Render::Components
