#include "results_container.hpp"
#include "../../../../helpers/gfx.hpp"
#include "../../../icons/manager.hpp"
#include "../render_state.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace Lawnch::Core::Window::Render::Components {

void ResultsContainer::update_metrics(const Config::Config &cfg) const {
  BLFont font = Lawnch::Gfx::get_font(cfg.result_item_font_family,
                                      cfg.result_item_font_size,
                                      cfg.result_item_font_weight);
  BLFontMetrics metrics = font.metrics();
  double name_h = metrics.ascent + metrics.descent;

  double comment_h = 0;
  if (cfg.result_item_enable_comment) {
    BLFont cfont = Lawnch::Gfx::get_font(cfg.result_item_font_family,
                                         cfg.result_item_comment_font_size,
                                         cfg.result_item_comment_font_weight);
    BLFontMetrics cmetrics = cfont.metrics();
    comment_h = cmetrics.ascent + cmetrics.descent;
  }

  double text_gap = 4.0;
  double item_inner_h = name_h;
  if (cfg.result_item_enable_comment) {
    item_inner_h += text_gap + comment_h;
  }

  cached_item_height =
      (cfg.result_item_padding.top + cfg.result_item_padding.bottom) +
      item_inner_h + cfg.results_spacing;
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

  BLFont font = Lawnch::Gfx::get_font(cfg.result_item_font_family,
                                      cfg.result_item_font_size,
                                      cfg.result_item_font_weight);
  BLFontMetrics fm = font.metrics();

  BLFont comment_font = Lawnch::Gfx::get_font(
      cfg.result_item_font_family, cfg.result_item_comment_font_size,
      cfg.result_item_comment_font_weight);
  BLFontMetrics cm = comment_font.metrics();

  double center_y = item_y + (item_h / 2.0);

  auto bg_color = is_selected ? cfg.result_item_selected_background_color
                              : cfg.result_item_default_background_color;
  auto border_color = is_selected ? cfg.result_item_selected_border_color
                                  : cfg.result_item_default_border_color;
  auto text_color = is_selected ? cfg.result_item_selected_text_color
                                : cfg.result_item_default_text_color;
  auto comment_color_cfg = is_selected ? cfg.result_item_selected_comment_color
                                       : cfg.result_item_comment_color;

  int radius = is_selected ? cfg.result_item_selected_border_radius
                           : cfg.result_item_default_border_radius;
  int border_w = is_selected ? cfg.result_item_selected_border_width
                             : cfg.result_item_default_border_width;

  BLRoundRect item_rect =
      Lawnch::Gfx::rounded_rect(item_x, item_y, item_w, item_h, radius);

  if (bg_color.a > 0) {
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(bg_color));
    ctx.fill_round_rect(item_rect);
  }

  if (border_w > 0 && border_color.a > 0) {
    ctx.set_stroke_style(Lawnch::Gfx::toBLColor(border_color));
    ctx.set_stroke_width(border_w);
    ctx.stroke_round_rect(item_rect);
  }

  double draw_x = item_x + cfg.result_item_padding.left;
  double draw_w =
      item_w - (cfg.result_item_padding.left + cfg.result_item_padding.right);

  double current_icon_size = 0;
  if (cfg.result_item_enable_icon) {
    double max_icon_h = item_h * 0.8;
    current_icon_size = std::min((double)cfg.result_item_icon_size, max_icon_h);

    double icon_y = std::floor(center_y - (current_icon_size / 2.0));

    Icons::Manager::Instance().render_icon(ctx, result.icon, std::floor(draw_x),
                                           icon_y, current_icon_size);

    double icon_gap = current_icon_size + cfg.result_item_icon_padding_right;
    draw_x += icon_gap;
    draw_w -= icon_gap;
  }

  if (draw_w > 0) {
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(text_color));
    double text_x_pos = draw_x;

    if (cfg.result_item_text_align == "center") {
      text_x_pos = item_x + (item_w / 2.0);
    } else if (cfg.result_item_text_align == "right") {
      text_x_pos = item_x + item_w - cfg.result_item_padding.right;
    }

    double name_y;
    double comment_y = 0;

    if (cfg.result_item_enable_comment && !result.comment.empty()) {
      double total_text_h = (fm.ascent + fm.descent) + (cm.ascent + cm.descent);
      double start_text_y = center_y - (total_text_h / 2.0);

      name_y = std::floor(start_text_y + fm.ascent);
      comment_y =
          std::floor(start_text_y + (fm.ascent + fm.descent) + cm.ascent);
    } else {
      name_y = std::floor(center_y + (fm.cap_height / 2.0));
    }

    ctx.save();
    ctx.clip_to_rect(BLRect(draw_x, item_y, draw_w, item_h));

    std::string display_name = result.name;
    if (cfg.result_item_text_align != "center" &&
        cfg.result_item_text_align != "right") {
      display_name = Lawnch::Gfx::truncate_text(result.name, font, draw_w);
    }

    double final_text_x = text_x_pos;
    if (cfg.result_item_text_align == "center" ||
        cfg.result_item_text_align == "right") {
      BLGlyphBuffer gb;
      gb.set_utf8_text(display_name.c_str(), display_name.size());
      font.shape(gb);
      BLTextMetrics tm;
      font.get_text_metrics(gb, tm);

      if (cfg.result_item_text_align == "center") {
        final_text_x -= (tm.advance.x / 2.0);
      } else {
        final_text_x -= tm.advance.x;
      }
    }

    if (cfg.result_item_highlight_enable && !search_text.empty()) {
      auto highlight_color = is_selected
                                 ? cfg.result_item_selected_highlight_color
                                 : cfg.result_item_highlight_color;
      BLFont highlight_font = Lawnch::Gfx::get_font(
          cfg.result_item_font_family, cfg.result_item_font_size,
          cfg.result_item_highlight_font_weight);

      // Strip filter prefixes
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
        auto char_color = highlight_mask[i] ? highlight_color : text_color;

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

    if (cfg.result_item_enable_comment && !result.comment.empty()) {
      ctx.set_fill_style(Lawnch::Gfx::toBLColor(comment_color_cfg));

      std::string display_comment = result.comment;
      if (cfg.result_item_text_align != "center" &&
          cfg.result_item_text_align != "right") {
        display_comment =
            Lawnch::Gfx::truncate_text(result.comment, comment_font, draw_w);
      }

      double final_comment_x = text_x_pos;
      if (cfg.result_item_text_align == "center" ||
          cfg.result_item_text_align == "right") {
        BLGlyphBuffer gb;
        gb.set_utf8_text(display_comment.c_str(), display_comment.size());
        comment_font.shape(gb);
        BLTextMetrics tm;
        comment_font.get_text_metrics(gb, tm);

        if (cfg.result_item_text_align == "center") {
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
        (cfg.results_scrollbar_width + (cfg.results_scrollbar_padding * 2));
  }

  int end_index = std::min(total_results, state.scroll_offset + visible_count);

  double container_x = context.x + cfg.results_margin.left;
  double container_y = layout_y;
  double container_w = context.available_w -
                       (cfg.results_margin.left + cfg.results_margin.right);
  double container_h =
      context.available_h - cfg.results_margin.top - cfg.results_margin.bottom;

  // Draw container background
  if (cfg.results_background_color.a > 0) {
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.results_background_color));
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
    // Calculate how much empty space there is and push items down
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

    double item_h = std::floor(item_height - cfg.results_spacing);
    bool is_sel = (i == state.selected_index);

    draw_result_item(ctx, cfg, res, content_x, item_y, content_w, item_h,
                     is_sel, state.search_text);
  }

  if (show_scrollbar) {
    double track_x = context.x + context.available_w -
                     cfg.results_margin.right - cfg.results_padding.right -
                     cfg.results_scrollbar_width -
                     cfg.results_scrollbar_padding;
    double track_y = items_start_y;
    double track_h = available_h;

    if (cfg.results_scrollbar_bg_color.a > 0) {
      ctx.set_fill_style(
          Lawnch::Gfx::toBLColor(cfg.results_scrollbar_bg_color));
      ctx.fill_round_rect(Lawnch::Gfx::rounded_rect(
          track_x, track_y, cfg.results_scrollbar_width, track_h,
          cfg.results_scrollbar_radius));
    }

    double ratio = (double)visible_count / total_results;
    double thumb_h = std::max(20.0, track_h * ratio);
    double thumb_range = track_h - thumb_h;
    double scroll_progress =
        (double)state.scroll_offset / (total_results - visible_count);
    double thumb_y = track_y + (scroll_progress * thumb_range);

    ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.results_scrollbar_color));
    ctx.fill_round_rect(
        Lawnch::Gfx::rounded_rect(track_x, thumb_y, cfg.results_scrollbar_width,
                                  thumb_h, cfg.results_scrollbar_radius));
  }

  return {context.available_w,
          container_h + cfg.results_margin.top + cfg.results_margin.bottom};
}

} // namespace Lawnch::Core::Window::Render::Components
