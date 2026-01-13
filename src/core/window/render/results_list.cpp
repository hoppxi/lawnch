#include "results_list.hpp"
#include "../../../helpers/gfx.hpp"
#include "../../icons/manager.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace Lawnch::Core::Window::Render {

static BLRgba32 ToBLColor(const Lawnch::Config::Color &c) {
  return BLRgba32(c.r * 255, c.g * 255, c.b * 255, c.a * 255);
}

void ResultsList::draw(BLContext &ctx, int width, int height, double start_y,
                       const Config::Config &cfg,
                       const std::vector<Search::SearchResult> &results,
                       int start_index, int selected_index, double item_height,
                       const std::string &search_text) {

  BLFont font = Lawnch::Gfx::get_font(cfg.result_item_font_family,
                                      cfg.result_item_font_size,
                                      cfg.result_item_font_weight);
  BLFontMetrics fm = font.metrics();

  BLFont comment_font = Lawnch::Gfx::get_font(
      cfg.result_item_font_family, cfg.result_item_comment_font_size,
      cfg.result_item_comment_font_weight);
  BLFontMetrics cm = comment_font.metrics();

  double layout_y = start_y + cfg.results_margin.top;
  double available_h = height - layout_y - cfg.results_margin.bottom -
                       cfg.results_padding.top - cfg.results_padding.bottom;

  if (item_height <= 0)
    item_height = 32;

  int visible_count = std::max(1, (int)std::floor(available_h / item_height));
  int total_results = (int)results.size();
  int end_index = std::min(total_results, start_index + visible_count);

  double content_x = cfg.window_border_width + cfg.results_margin.left +
                     cfg.results_padding.left;
  double content_w = width - (cfg.window_border_width * 2) -
                     (cfg.results_margin.left + cfg.results_margin.right) -
                     (cfg.results_padding.left + cfg.results_padding.right);

  double container_x = cfg.window_border_width + cfg.results_margin.left;
  double container_y = layout_y;
  double container_w = width - (cfg.window_border_width * 2) -
                       (cfg.results_margin.left + cfg.results_margin.right);
  double container_h = height - layout_y - cfg.results_margin.bottom;

  if (cfg.results_background_color.a > 0) {
    ctx.set_fill_style(ToBLColor(cfg.results_background_color));
    ctx.fill_round_rect(Lawnch::Gfx::rounded_rect(container_x, container_y,
                                                  container_w, container_h,
                                                  cfg.results_border_radius));
  }
  if (cfg.results_border_width > 0 && cfg.results_border_color.a > 0) {
    ctx.set_stroke_style(ToBLColor(cfg.results_border_color));
    ctx.set_stroke_width(cfg.results_border_width);
    ctx.stroke_round_rect(Lawnch::Gfx::rounded_rect(container_x, container_y,
                                                    container_w, container_h,
                                                    cfg.results_border_radius));
  }

  double items_start_y = layout_y + cfg.results_padding.top;

  for (int i = start_index; i < end_index; ++i) {
    const auto &res = results[i];
    int rel_i = i - start_index;

    double item_y = std::floor(items_start_y + rel_i * item_height);
    double item_h = std::floor(item_height - cfg.results_spacing);
    double center_y = item_y + (item_h / 2.0);

    bool is_sel = (i == selected_index);

    auto bg_color = is_sel ? cfg.result_item_selected_background_color
                           : cfg.result_item_default_background_color;
    auto border_color = is_sel ? cfg.result_item_selected_border_color
                               : cfg.result_item_default_border_color;
    auto text_color = is_sel ? cfg.result_item_selected_text_color
                             : cfg.result_item_default_text_color;
    auto comment_color_cfg = is_sel ? cfg.result_item_selected_comment_color
                                    : cfg.result_item_comment_color;

    int radius = is_sel ? cfg.result_item_selected_border_radius
                        : cfg.result_item_default_border_radius;
    int border_w = is_sel ? cfg.result_item_selected_border_width
                          : cfg.result_item_default_border_width;

    BLRoundRect item_rect =
        Lawnch::Gfx::rounded_rect(content_x, item_y, content_w, item_h, radius);

    if (bg_color.a > 0) {
      ctx.set_fill_style(ToBLColor(bg_color));
      ctx.fill_round_rect(item_rect);
    }

    if (border_w > 0 && border_color.a > 0) {
      ctx.set_stroke_style(ToBLColor(border_color));
      ctx.set_stroke_width(border_w);
      ctx.stroke_round_rect(item_rect);
    }

    double draw_x = content_x + cfg.result_item_padding.left;
    double draw_w = content_w - (cfg.result_item_padding.left +
                                 cfg.result_item_padding.right);

    double current_icon_size = 0;
    if (cfg.result_item_enable_icon) {
      double max_icon_h = item_h * 0.8;
      current_icon_size =
          std::min((double)cfg.result_item_icon_size, max_icon_h);

      double icon_y = std::floor(center_y - (current_icon_size / 2.0));

      Icons::Manager::Instance().render_icon(ctx, res.icon, std::floor(draw_x),
                                             icon_y, current_icon_size);

      double icon_gap = current_icon_size + cfg.result_item_icon_padding_right;
      draw_x += icon_gap;
      draw_w -= icon_gap;
    }

    if (draw_w > 0) {
      ctx.set_fill_style(ToBLColor(text_color));
      double text_x_pos = draw_x;

      if (cfg.result_item_text_align == "center") {
        text_x_pos = content_x + (content_w / 2.0);
      } else if (cfg.result_item_text_align == "right") {
        text_x_pos = content_x + content_w - cfg.result_item_padding.right;
      }

      double name_y;
      double comment_y;

      if (cfg.result_item_enable_comment && !res.comment.empty()) {
        double total_text_h =
            (fm.ascent + fm.descent) + (cm.ascent + cm.descent);
        double start_text_y = center_y - (total_text_h / 2.0);

        name_y = std::floor(start_text_y + fm.ascent);
        comment_y =
            std::floor(start_text_y + (fm.ascent + fm.descent) + cm.ascent);
      } else {
        name_y = std::floor(center_y + (fm.cap_height / 2.0));
      }

      ctx.save();
      ctx.clip_to_rect(BLRect(draw_x, item_y, draw_w, item_h));

      std::string display_name = res.name;
      if (cfg.result_item_text_align != "center" &&
          cfg.result_item_text_align != "right") {
        display_name = Lawnch::Gfx::truncate_text(res.name, font, draw_w);
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
        auto highlight_color = is_sel ? cfg.result_item_selected_highlight_color
                                      : cfg.result_item_highlight_color;
        BLFont highlight_font = Lawnch::Gfx::get_font(
            cfg.result_item_font_family, cfg.result_item_font_size,
            cfg.result_item_highlight_font_weight);

        // Strip filter prefixes like ":apps ", "=", ">", ":cmd "...
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

          ctx.set_fill_style(ToBLColor(char_color));
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

      if (cfg.result_item_enable_comment && !res.comment.empty()) {
        ctx.set_fill_style(ToBLColor(comment_color_cfg));

        std::string display_comment = res.comment;
        if (cfg.result_item_text_align != "center" &&
            cfg.result_item_text_align != "right") {
          display_comment =
              Lawnch::Gfx::truncate_text(res.comment, comment_font, draw_w);
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

  if (cfg.results_scrollbar_enable && total_results > visible_count) {
    double track_x = width - cfg.results_scrollbar_width -
                     cfg.results_scrollbar_padding - cfg.window_border_width;
    double track_y = layout_y;
    double track_h = available_h;

    if (cfg.results_scrollbar_bg_color.a > 0) {
      ctx.set_fill_style(ToBLColor(cfg.results_scrollbar_bg_color));
      ctx.fill_round_rect(Lawnch::Gfx::rounded_rect(
          track_x, track_y, cfg.results_scrollbar_width, track_h,
          cfg.results_scrollbar_radius));
    }

    double ratio = (double)visible_count / total_results;
    double thumb_h = std::max(20.0, track_h * ratio); // Min height 20px
    double thumb_range = track_h - thumb_h;
    double scroll_progress =
        (double)start_index / (total_results - visible_count);
    double thumb_y = track_y + (scroll_progress * thumb_range);

    ctx.set_fill_style(ToBLColor(cfg.results_scrollbar_color));
    ctx.fill_round_rect(
        Lawnch::Gfx::rounded_rect(track_x, thumb_y, cfg.results_scrollbar_width,
                                  thumb_h, cfg.results_scrollbar_radius));
  }
}

} // namespace Lawnch::Core::Window::Render
