#include "preview.hpp"
#include "../../../../helpers/gfx.hpp"
#include "../../../../helpers/image_cache.hpp"
#include "../../../icons/manager.hpp"
#include "../render_state.hpp"
#include <algorithm>
#include <deque>
#include <functional>
#include <iostream>
#include <sstream>
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

class PreviewLayout {
public:
  struct LayoutItem {
    std::string type;
    bool visible = true;
    bool is_fallback_icon = false;
    double width = 0;
    double height = 0;
    std::vector<LayoutItem> children;
    bool is_group = false;
  };

  PreviewLayout(const Config::Config &cfg, const Search::SearchResult &selected,
                double available_w)
      : cfg(cfg), selected(selected), available_w(available_w) {
    parse_layout_string();
    resolve_item_properties();
    calculate_layout_dimensions();
  }

  double get_total_height() const {
    double height = cfg.preview_padding.top + cfg.preview_padding.bottom;
    bool first = true;
    for (const auto &item : layout) {
      if (!item.visible)
        continue;
      if (!first)
        height += cfg.preview_vertical_spacing;
      height += item.height;
      first = false;
    }
    return height;
  }

  void draw(BLContext &ctx, double x, double y) {
    double preview_h = get_total_height();
    double preview_w = available_w;

    if (cfg.preview_background_color.a > 0) {
      ctx.set_fill_style(Lawnch::Gfx::toBLColor(cfg.preview_background_color));
      ctx.fill_rect(x, y, preview_w, preview_h);
    }

    double content_x = x + cfg.preview_padding.left;
    double content_y = y + cfg.preview_padding.top;
    double content_max_w =
        preview_w - cfg.preview_padding.left - cfg.preview_padding.right;

    double current_y = content_y;
    bool first = true;

    for (const auto &item : layout) {
      if (!item.visible)
        continue;
      if (!first)
        current_y += cfg.preview_vertical_spacing;

      if (item.is_group) {
        draw_group(ctx, item, content_x, current_y, content_max_w);
      } else {
        draw_item(ctx, item, content_x, current_y, content_max_w);
      }

      current_y += item.height;
      first = false;
    }
  }

private:
  const Config::Config &cfg;
  const Search::SearchResult &selected;
  double available_w;
  std::vector<LayoutItem> layout;
  BLImage preview_image;

  void parse_layout_string() {
    for (const auto &raw_item : cfg.preview_show) {
      if (raw_item.rfind("(", 0) == 0) {
        LayoutItem group;
        group.is_group = true;
        group.type = "group";
        std::string content = raw_item.substr(1, raw_item.size() - 2);
        std::stringstream ss(content);
        std::string sub_item;
        while (std::getline(ss, sub_item, ':')) {
          if (!sub_item.empty()) {
            group.children.push_back({sub_item});
          }
        }
        layout.push_back(group);
      } else {
        layout.push_back({raw_item});
      }
    }
  }

  void resolve_item_properties() {
    bool has_preview_image = false;
    bool image_failed = false;

    std::function<void(LayoutItem &)> check_image = [&](LayoutItem &item) {
      if (item.is_group) {
        for (auto &child : item.children)
          check_image(child);
      } else if (item.type == "preview_image") {
        has_preview_image = true;
        if (!selected.preview_image_path.empty()) {
          std::filesystem::path image_path =
              ImageCache::ImageCache::Instance().get_image(
                  selected.preview_image_path, cfg.preview_preview_image_size,
                  cfg.preview_preview_image_size);

          if (!image_path.empty()) {
            if (BLImage *cached = get_cached_preview(image_path.string())) {
              preview_image = *cached;
            } else {
              if (preview_image.read_from_file(image_path.string().c_str()) ==
                  BL_SUCCESS) {
                cache_preview(image_path.string(), preview_image);
              } else {
                image_failed = true;
              }
            }
          } else {
            image_failed = true;
          }
        } else {
          image_failed = true;
        }

        if (image_failed) {
          if (cfg.preview_fallback_icon) {
            item.is_fallback_icon = true;
          } else {
            item.visible = false;
          }
        }
      }
    };

    for (auto &item : layout)
      check_image(item);

    if (has_preview_image && image_failed && cfg.preview_fallback_icon &&
        cfg.preview_hide_icon_if_fallback) {
      std::function<void(LayoutItem &)> hide_icons = [&](LayoutItem &item) {
        if (item.is_group) {
          for (auto &child : item.children)
            hide_icons(child);
        } else if (item.type == "icon") {
          item.visible = false;
        }
      };
      for (auto &item : layout)
        hide_icons(item);
    }
  }

  void calculate_layout_dimensions() {
    std::function<void(LayoutItem &)> calc_dims = [&](LayoutItem &item) {
      if (!item.visible) {
        item.width = 0;
        item.height = 0;
        return;
      }

      if (item.is_group) {
        double max_h = 0;
        std::vector<LayoutItem *> text_children;
        double fixed_width = 0;
        int visible_children = 0;

        for (auto &child : item.children) {
          calculate_atomic_item_dims(child);
          if (child.visible) {
            visible_children++;
            max_h = std::max(max_h, child.height);
            if (child.type == "name" || child.type == "comment") {
              text_children.push_back(&child);
            } else {
              fixed_width += child.width;
            }
          }
        }

        if (visible_children > 0) {
          double spacing_width =
              (visible_children - 1) * cfg.preview_horizontal_spacing;
          double content_max_w = available_w - cfg.preview_padding.left -
                                 cfg.preview_padding.right;
          double remaining_width = content_max_w - fixed_width - spacing_width;

          if (!text_children.empty()) {
            double width_per_text =
                (remaining_width > 0) ? (remaining_width / text_children.size())
                                      : 0;
            for (auto *child_ptr : text_children) {
              child_ptr->width = width_per_text;
            }
          }
        }

        double total_w = 0;
        bool first_child = true;
        for (const auto &child : item.children) {
          if (child.visible) {
            if (!first_child) {
              total_w += cfg.preview_horizontal_spacing;
            }
            total_w += child.width;
            first_child = false;
          }
        }

        item.width = total_w;
        item.height = max_h;

      } else {
        calculate_atomic_item_dims(item);
      }
    };

    for (auto &item : layout)
      calc_dims(item);
  }

  void calculate_atomic_item_dims(LayoutItem &item) {
    if (item.type == "icon") {
      item.width = item.height = cfg.preview_icon_size;
    } else if (item.type == "preview_image") {
      item.width = item.height = cfg.preview_preview_image_size;
    } else if (item.type == "name" || item.type == "comment") {
      bool is_name = item.type == "name";
      if (!is_name && selected.comment.empty()) {
        item.visible = false;
        item.width = item.height = 0;
        return;
      }
      BLFont font = is_name ? Gfx::get_font(cfg.preview_name_font_family,
                                            cfg.preview_name_font_size,
                                            cfg.preview_name_font_weight)
                            : Gfx::get_font(cfg.preview_name_font_family,
                                            cfg.preview_comment_font_size,
                                            cfg.preview_comment_font_weight);
      BLFontMetrics fm = font.metrics();
      item.height = fm.ascent + fm.descent;
    }
  }

  void draw_group(BLContext &ctx, const LayoutItem &group, double x, double y,
                  double w_avail) {
    double group_start_x = x + (w_avail - group.width) / 2.0;
    double current_x = group_start_x;
    bool first_child = true;
    for (const auto &child : group.children) {
      if (!child.visible)
        continue;
      if (!first_child)
        current_x += cfg.preview_horizontal_spacing;

      double child_y = y + (group.height - child.height) / 2.0;
      draw_item(ctx, child, current_x, child_y, child.width);

      current_x += child.width;
      first_child = false;
    }
  }

  void draw_item(BLContext &ctx, const LayoutItem &item, double x, double y,
                 double w_avail) {
    if (item.type == "icon") {
      draw_icon(ctx, selected.icon, x, y, item.width, item.height, w_avail);
    } else if (item.type == "preview_image") {
      if (item.is_fallback_icon) {
        draw_icon(ctx, selected.icon, x, y, item.width, item.height, w_avail);
      } else if (!preview_image.is_empty()) {
        double draw_x = x + (w_avail - preview_image.width()) / 2.0;
        double draw_y = y + (item.height - preview_image.height()) / 2.0;
        ctx.blit_image(BLPoint(draw_x, draw_y), preview_image);
      }
    } else if (item.type == "name") {
      draw_text(ctx, selected.name, x, y, w_avail, cfg.preview_name_font_family,
                cfg.preview_name_font_size, cfg.preview_name_font_weight,
                cfg.preview_name_color);
    } else if (item.type == "comment") {
      draw_text(ctx, selected.comment, x, y, w_avail,
                cfg.preview_name_font_family, cfg.preview_comment_font_size,
                cfg.preview_comment_font_weight, cfg.preview_comment_color);
    }
  }

  void draw_icon(BLContext &ctx, const std::string &icon_name, double x,
                 double y, double w, double h, double w_avail) {
    double draw_x = x + (w_avail - w) / 2.0;
    Icons::Manager::Instance().render_icon(ctx, icon_name, draw_x, y, w);
  }

  void draw_text(BLContext &ctx, const std::string &text_content, double x,
                 double y, double w_avail, const std::string &family, int size,
                 const std::string &weight, const Config::Color &color) {
    BLFont font = Gfx::get_font(family, size, weight);
    BLFontMetrics fm = font.metrics();
    std::string text = Gfx::truncate_text(text_content, font, w_avail);

    BLTextMetrics tm;
    BLGlyphBuffer gb;
    gb.set_utf8_text(text.c_str());
    font.shape(gb);
    font.get_text_metrics(gb, tm);

    double text_x = x + (w_avail - tm.advance.x) / 2.0;
    ctx.set_fill_style(Gfx::toBLColor(color));
    ctx.fill_utf8_text(BLPoint(text_x, y + fm.ascent), font, text.c_str());
  }
};

} // namespace

double Preview::get_height(const Config::Config &cfg,
                           const RenderState &state) {
  if (!cfg.preview_enable || state.results.empty() ||
      state.selected_index < 0 ||
      state.selected_index >= (int)state.results.size()) {
    return 0;
  }
  const auto &selected = state.results[state.selected_index];
  PreviewLayout layout(cfg, selected, 1000.0);
  return layout.get_total_height();
}

ComponentResult Preview::draw(ComponentContext &context) {
  auto &state = context.state;

  if (!context.cfg.preview_enable || state.results.empty() ||
      state.selected_index < 0 ||
      state.selected_index >= (int)state.results.size()) {
    return {0, 0};
  }

  const auto &selected = state.results[state.selected_index];
  PreviewLayout layout(context.cfg, selected, context.available_w);

  double total_height = layout.get_total_height();
  if (total_height > 0) {
    layout.draw(context.ctx, context.x, context.y);
  }

  return {context.available_w, total_height};
}

} // namespace Lawnch::Core::Window::Render::Components
