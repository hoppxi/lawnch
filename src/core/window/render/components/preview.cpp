#include "preview.hpp"
#include "../../../../helpers/gfx.hpp"
#include "../../../../helpers/image_cache.hpp"
#include "../../../icons/manager.hpp"
#include "../render_state.hpp"
#include <algorithm>
#include <deque>
#include <iostream>
#include <unordered_map>

namespace Lawnch::Core::Window::Render::Components {

namespace {

std::unordered_map<std::string, BLImage> preview_lru;
std::deque<std::string> preview_lru_order;
constexpr size_t LRU_MAX = 5;

BLImage *get_cached_preview(const std::string &path) {
  auto it = preview_lru.find(path);
  if (it != preview_lru.end()) {
    auto pos =
        std::find(preview_lru_order.begin(), preview_lru_order.end(), path);
    if (pos != preview_lru_order.end()) {
      preview_lru_order.erase(pos);
      preview_lru_order.push_front(path);
    }
    return &it->second;
  }
  return nullptr;
}

void cache_preview(const std::string &path, BLImage img) {
  if (preview_lru.size() >= LRU_MAX && !preview_lru_order.empty()) {
    preview_lru.erase(preview_lru_order.back());
    preview_lru_order.pop_back();
  }
  preview_lru_order.push_front(path);
  preview_lru[path] = std::move(img);
}

bool is_horizontal(const std::string &dir) {
  return dir == "h" || dir == "horizontal";
}

struct PreviewAssets {
  BLImage preview_image;
  bool has_preview_image = false;
  bool use_fallback_icon = false;
  bool has_icon = false;
  std::string icon_name;
};

PreviewAssets resolve_assets(const Config::Config &cfg,
                             const Search::SearchResult &selected) {
  PreviewAssets assets;
  assets.icon_name = selected.icon;
  assets.has_icon = !selected.icon.empty();

  if (!selected.preview_image_path.empty()) {
    std::filesystem::path image_path =
        ImageCache::ImageCache::Instance().get_image(
            selected.preview_image_path, cfg.preview_image_size,
            cfg.preview_image_size);

    if (!image_path.empty()) {
      if (BLImage *cached = get_cached_preview(image_path.string())) {
        assets.preview_image = *cached;
        assets.has_preview_image = true;
      } else {
        if (assets.preview_image.read_from_file(image_path.string().c_str()) ==
            BL_SUCCESS) {
          cache_preview(image_path.string(), assets.preview_image);
          assets.has_preview_image = true;
        }
      }
    }
  }

  // If no preview image and fallback is enabled, use icon as preview image
  if (!assets.has_preview_image && cfg.preview_icon_fallback) {
    assets.use_fallback_icon = true;
  }

  return assets;
}

void draw_icon(BLContext &ctx, const std::string &icon_name, double x, double y,
               double size) {
  Icons::Manager::Instance().render_icon(ctx, icon_name, x, y, size);
}

void draw_text(BLContext &ctx, const std::string &text_content, double x,
               double y, double w_avail, const std::string &family, int size,
               const std::string &weight, const std::string &path,
               const Config::Color &color, const std::string &align = "center") {
  BLFont font = Gfx::get_font(family, size, weight, path);
  BLFontMetrics fm = font.metrics();
  std::string text = Gfx::truncate_text(text_content, font, w_avail);

  BLTextMetrics tm;
  BLGlyphBuffer gb;
  gb.set_utf8_text(text.c_str());
  font.shape(gb);
  font.get_text_metrics(gb, tm);

  double text_x;
  if (align == "center") {
    text_x = x + (w_avail - tm.advance.x) / 2.0;
  } else if (align == "right") {
    text_x = x + w_avail - tm.advance.x;
  } else {
    text_x = x;
  }
  ctx.set_fill_style(Gfx::toBLColor(color));
  ctx.fill_utf8_text(BLPoint(text_x, y + fm.ascent), font, text.c_str());
}

double get_text_height(const std::string &family, int size,
                       const std::string &weight, const std::string &path) {
  BLFont font = Gfx::get_font(family, size, weight, path);
  BLFontMetrics fm = font.metrics();
  return fm.ascent + fm.descent;
}

// vertical layout: preview_image on left, name+comment stacked on right
// if icon is present from the provider, icon goes left of name
double calc_vertical_height(const Config::Config &cfg,
                            const PreviewAssets &assets,
                            const Search::SearchResult &selected) {
  double image_h = 0;
  if (assets.has_preview_image || assets.use_fallback_icon) {
    image_h = cfg.preview_image_size;
  }

  double name_h = get_text_height(cfg.preview_name_font_family,
                                  cfg.preview_name_font_size,
                                  cfg.preview_name_font_weight,
                                  cfg.preview_name_font_path);
  double text_h = name_h;
  if (!selected.comment.empty()) {
    double comment_h = get_text_height(cfg.preview_comment_font_family,
                                       cfg.preview_comment_font_size,
                                       cfg.preview_comment_font_weight,
                                       cfg.preview_comment_font_path);
    text_h += cfg.preview_gap_v + comment_h;
  }

  double content_h = std::max(image_h, text_h);
  return cfg.preview_padding.top + content_h + cfg.preview_padding.bottom;
}

// horizontal layout: preview_image on top, name below, comment below that
double calc_horizontal_height(const Config::Config &cfg,
                              const PreviewAssets &assets,
                              const Search::SearchResult &selected) {
  double total_h = 0;

  if (assets.has_preview_image || assets.use_fallback_icon) {
    total_h += cfg.preview_image_size;
  }

  double name_h = get_text_height(cfg.preview_name_font_family,
                                  cfg.preview_name_font_size,
                                  cfg.preview_name_font_weight,
                                  cfg.preview_name_font_path);
  if (total_h > 0) total_h += cfg.preview_gap_v;
  total_h += name_h;

  if (!selected.comment.empty()) {
    double comment_h = get_text_height(cfg.preview_comment_font_family,
                                       cfg.preview_comment_font_size,
                                       cfg.preview_comment_font_weight,
                                       cfg.preview_comment_font_path);
    total_h += cfg.preview_gap_v + comment_h;
  }

  return cfg.preview_padding.top + total_h + cfg.preview_padding.bottom;
}

void draw_vertical_layout(BLContext &ctx, const Config::Config &cfg,
                           const PreviewAssets &assets,
                           const Search::SearchResult &selected,
                           double x, double y, double available_w) {
  double content_x = x + cfg.preview_padding.left;
  double content_y = y + cfg.preview_padding.top;
  double content_w =
      available_w - cfg.preview_padding.left - cfg.preview_padding.right;

  double image_size = 0;
  double text_start_x = content_x;

  if (assets.has_preview_image || assets.use_fallback_icon) {
    image_size = cfg.preview_image_size;

    if (assets.has_preview_image && !assets.preview_image.is_empty()) {
      double img_x = content_x + (image_size - assets.preview_image.width()) / 2.0;
      double img_y = content_y + (image_size - assets.preview_image.height()) / 2.0;
      ctx.blit_image(BLPoint(img_x, img_y), assets.preview_image);
    } else if (assets.use_fallback_icon && assets.has_icon) {
      draw_icon(ctx, assets.icon_name, content_x, content_y, image_size);
    }

    text_start_x = content_x + image_size + cfg.preview_gap_h;
  }

  double text_w = content_w - (text_start_x - content_x);

  double name_h = get_text_height(cfg.preview_name_font_family,
                                  cfg.preview_name_font_size,
                                  cfg.preview_name_font_weight,
                                  cfg.preview_name_font_path);

  double name_x = text_start_x;
  double name_w = text_w;

  if (assets.has_icon && assets.has_preview_image && !assets.use_fallback_icon) {
    double icon_y = content_y + (name_h - cfg.preview_icon_size) / 2.0;
    draw_icon(ctx, assets.icon_name, name_x, icon_y, cfg.preview_icon_size);
    name_x += cfg.preview_icon_size + cfg.preview_gap_h;
    name_w -= cfg.preview_icon_size + cfg.preview_gap_h;
  }

  draw_text(ctx, selected.name, name_x, content_y, name_w,
            cfg.preview_name_font_family, cfg.preview_name_font_size,
            cfg.preview_name_font_weight, cfg.preview_name_font_path,
            cfg.preview_name_color, "left");

  if (!selected.comment.empty()) {
    double comment_y = content_y + name_h + cfg.preview_gap_v;
    draw_text(ctx, selected.comment, text_start_x, comment_y, text_w,
              cfg.preview_comment_font_family, cfg.preview_comment_font_size,
              cfg.preview_comment_font_weight, cfg.preview_comment_font_path,
              cfg.preview_comment_color, "left");
  }
}

void draw_horizontal_layout(BLContext &ctx, const Config::Config &cfg,
                             const PreviewAssets &assets,
                             const Search::SearchResult &selected,
                             double x, double y, double available_w) {
  double content_x = x + cfg.preview_padding.left;
  double content_y = y + cfg.preview_padding.top;
  double content_w =
      available_w - cfg.preview_padding.left - cfg.preview_padding.right;
  double current_y = content_y;

  if (assets.has_preview_image || assets.use_fallback_icon) {
    double image_size = cfg.preview_image_size;

    if (assets.has_preview_image && !assets.preview_image.is_empty()) {
      double img_x = content_x + (content_w - assets.preview_image.width()) / 2.0;
      ctx.blit_image(BLPoint(img_x, current_y), assets.preview_image);
    } else if (assets.use_fallback_icon && assets.has_icon) {
      double icon_x = content_x + (content_w - image_size) / 2.0;
      draw_icon(ctx, assets.icon_name, icon_x, current_y, image_size);
    }

    current_y += image_size + cfg.preview_gap_v;
  }

  double name_h = get_text_height(cfg.preview_name_font_family,
                                  cfg.preview_name_font_size,
                                  cfg.preview_name_font_weight,
                                  cfg.preview_name_font_path);

  if (assets.has_icon && assets.has_preview_image && !assets.use_fallback_icon) {
    BLFont name_font = Gfx::get_font(cfg.preview_name_font_family,
                                     cfg.preview_name_font_size,
                                     cfg.preview_name_font_weight,
                                     cfg.preview_name_font_path);
    std::string truncated = Gfx::truncate_text(
        selected.name, name_font,
        content_w - cfg.preview_icon_size - cfg.preview_gap_h);

    BLGlyphBuffer gb;
    gb.set_utf8_text(truncated.c_str());
    name_font.shape(gb);
    BLTextMetrics tm;
    name_font.get_text_metrics(gb, tm);

    double unit_w = cfg.preview_icon_size + cfg.preview_gap_h + tm.advance.x;
    double unit_x = content_x + (content_w - unit_w) / 2.0;

    double icon_y = current_y + (name_h - cfg.preview_icon_size) / 2.0;
    draw_icon(ctx, assets.icon_name, unit_x, icon_y, cfg.preview_icon_size);

    double text_x = unit_x + cfg.preview_icon_size + cfg.preview_gap_h;
    BLFontMetrics fm = name_font.metrics();
    ctx.set_fill_style(Gfx::toBLColor(cfg.preview_name_color));
    ctx.fill_utf8_text(BLPoint(text_x, current_y + fm.ascent), name_font,
                       truncated.c_str());
  } else {
    draw_text(ctx, selected.name, content_x, current_y, content_w,
              cfg.preview_name_font_family, cfg.preview_name_font_size,
              cfg.preview_name_font_weight, cfg.preview_name_font_path,
              cfg.preview_name_color, "center");
  }

  current_y += name_h;

  if (!selected.comment.empty()) {
    current_y += cfg.preview_gap_v;
    draw_text(ctx, selected.comment, content_x, current_y, content_w,
              cfg.preview_comment_font_family, cfg.preview_comment_font_size,
              cfg.preview_comment_font_weight, cfg.preview_comment_font_path,
              cfg.preview_comment_color, "center");
  }
}

} // namespace

double Preview::get_height(const Config::Config &cfg,
                           const RenderState &state) {
  if (!cfg.preview_enable || state.results.empty() ||
      state.selected_index < 0 ||
      state.selected_index >= (int)state.results.size()) {
    return 0;
  }
  const auto &selected = state.results[state.selected_index];
  PreviewAssets assets = resolve_assets(cfg, selected);

  if (is_horizontal(cfg.preview_direction)) {
    return calc_horizontal_height(cfg, assets, selected);
  } else {
    return calc_vertical_height(cfg, assets, selected);
  }
}

ComponentResult Preview::draw(ComponentContext &context) {
  auto &state = context.state;

  if (!context.cfg.preview_enable || state.results.empty() ||
      state.selected_index < 0 ||
      state.selected_index >= (int)state.results.size()) {
    return {0, 0};
  }

  const auto &selected = state.results[state.selected_index];
  const auto &cfg = context.cfg;
  PreviewAssets assets = resolve_assets(cfg, selected);

  double total_height;
  if (is_horizontal(cfg.preview_direction)) {
    total_height = calc_horizontal_height(cfg, assets, selected);
  } else {
    total_height = calc_vertical_height(cfg, assets, selected);
  }

  if (cfg.preview_background.a > 0) {
    context.ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.preview_background));
    context.ctx.fill_rect(context.x, context.y, context.available_w,
                          total_height);
  }

  if (is_horizontal(cfg.preview_direction)) {
    draw_horizontal_layout(context.ctx, cfg, assets, selected, context.x,
                           context.y, context.available_w);
  } else {
    draw_vertical_layout(context.ctx, cfg, assets, selected, context.x,
                         context.y, context.available_w);
  }

  return {context.available_w, total_height};
}

} // namespace Lawnch::Core::Window::Render::Components
