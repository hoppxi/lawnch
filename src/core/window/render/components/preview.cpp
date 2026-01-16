#include "preview.hpp"
#include "../../../../helpers/gfx.hpp"
#include "../../../icons/manager.hpp"
#include "../render_state.hpp"

namespace Lawnch::Core::Window::Render::Components {

double Preview::get_height(const Config::Config &cfg) {
  if (!cfg.preview_enable) {
    return 0;
  }

  double height = cfg.preview_padding.top + cfg.preview_padding.bottom;
  bool first = true;

  for (const auto &item : cfg.preview_show) {
    if (item == "icon") {
      if (!first)
        height += 8;
      height += cfg.preview_icon_size;
      first = false;
    } else if (item == "name") {
      if (!first)
        height += 8;
      BLFont font = Lawnch::Gfx::get_font(cfg.preview_name_font_family,
                                          cfg.preview_name_font_size,
                                          cfg.preview_name_font_weight);
      BLFontMetrics fm = font.metrics();
      height += fm.ascent + fm.descent;
      first = false;
    } else if (item == "comment") {
      if (!first)
        height += 4;
      BLFont font = Lawnch::Gfx::get_font(cfg.preview_name_font_family,
                                          cfg.preview_comment_font_size,
                                          cfg.preview_comment_font_weight);
      BLFontMetrics fm = font.metrics();
      height += fm.ascent + fm.descent;
      first = false;
    }
  }

  return height;
}

ComponentResult Preview::draw(ComponentContext &context) {
  auto &ctx = context.ctx;
  auto &cfg = context.cfg;
  auto &state = context.state;

  if (!cfg.preview_enable || state.results.empty()) {
    return {0, 0};
  }

  if (state.selected_index < 0 ||
      state.selected_index >= (int)state.results.size()) {
    return {0, 0};
  }

  const auto &selected = state.results[state.selected_index];

  double preview_w = context.available_w;
  double preview_h = get_height(cfg);

  if (cfg.preview_background_color.a > 0) {
    ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.preview_background_color));
    ctx.fill_rect(context.x, context.y, preview_w, preview_h);
  }

  double content_x = context.x + cfg.preview_padding.left;
  double content_y = context.y + cfg.preview_padding.top;
  double content_w =
      preview_w - cfg.preview_padding.left - cfg.preview_padding.right;

  double current_y = content_y;
  bool first = true;

  for (const auto &item : cfg.preview_show) {
    if (item == "icon") {
      if (!first)
        current_y += 8;

      double icon_size = cfg.preview_icon_size;
      double icon_x = content_x + (content_w - icon_size) / 2.0;
      Icons::Manager::Instance().render_icon(ctx, selected.icon, icon_x,
                                             current_y, icon_size);
      current_y += icon_size;
      first = false;
    } else if (item == "name") {
      if (!first)
        current_y += 8;

      BLFont name_font = Lawnch::Gfx::get_font(cfg.preview_name_font_family,
                                               cfg.preview_name_font_size,
                                               cfg.preview_name_font_weight);
      BLFontMetrics name_fm = name_font.metrics();

      std::string display_name =
          Lawnch::Gfx::truncate_text(selected.name, name_font, content_w);

      BLGlyphBuffer gb;
      gb.set_utf8_text(display_name.c_str(), display_name.size());
      name_font.shape(gb);
      BLTextMetrics tm;
      name_font.get_text_metrics(gb, tm);

      double text_x = content_x + (content_w - tm.advance.x) / 2.0;

      ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.preview_name_color));
      ctx.fill_utf8_text(BLPoint(text_x, current_y + name_fm.ascent), name_font,
                         display_name.c_str());

      current_y += name_fm.ascent + name_fm.descent;
      first = false;
    } else if (item == "comment" && !selected.comment.empty()) {
      if (!first)
        current_y += 4;

      BLFont comment_font = Lawnch::Gfx::get_font(
          cfg.preview_name_font_family, cfg.preview_comment_font_size,
          cfg.preview_comment_font_weight);
      BLFontMetrics comment_fm = comment_font.metrics();

      std::string display_comment =
          Lawnch::Gfx::truncate_text(selected.comment, comment_font, content_w);

      BLGlyphBuffer gb;
      gb.set_utf8_text(display_comment.c_str(), display_comment.size());
      comment_font.shape(gb);
      BLTextMetrics tm;
      comment_font.get_text_metrics(gb, tm);

      double text_x = content_x + (content_w - tm.advance.x) / 2.0;

      ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.preview_comment_color));
      ctx.fill_utf8_text(BLPoint(text_x, current_y + comment_fm.ascent),
                         comment_font, display_comment.c_str());

      current_y += comment_fm.ascent + comment_fm.descent;
      first = false;
    }
  }

  return {preview_w, preview_h};
}

} // namespace Lawnch::Core::Window::Render::Components
