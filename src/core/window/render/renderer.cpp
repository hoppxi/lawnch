#include "renderer.hpp"
#include "../../../helpers/gfx.hpp"
#include "../../../helpers/string.hpp"
#include "../../icons/manager.hpp"
#include "components/background.hpp"
#include "components/clock.hpp"
#include "components/input_box.hpp"
#include "components/input_prompt.hpp"
#include "components/preview.hpp"
#include "components/results_container.hpp"
#include "components/results_count.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

namespace Lawnch::Core::Window::Render {

Renderer::Renderer() { init_components(); }
Renderer::~Renderer() {}

void Renderer::init_components() {
  components["background"] = std::make_unique<Components::Background>();
  components["input"] = std::make_unique<Components::InputBox>();
  components["input_prompt"] = std::make_unique<Components::InputPrompt>();
  components["results_count"] = std::make_unique<Components::ResultsCount>();
  components["results"] = std::make_unique<Components::ResultsContainer>();
  components["preview"] = std::make_unique<Components::Preview>();
  components["clock"] = std::make_unique<Components::Clock>();
}

double estimate_component_height(const std::string &name,
                                 const Config::Config &cfg,
                                 const RenderState &state) {
  if (name == "input") {
    if (!cfg.input_visible)
      return 0;
    BLFont font = Lawnch::Gfx::get_font(
        cfg.input_font_family, cfg.input_font_size, cfg.input_font_weight);
    BLFontMetrics fm = font.metrics();
    return fm.ascent + fm.descent + cfg.input_padding.top +
           cfg.input_padding.bottom;
  }
  if (name == "input_prompt" && cfg.input_prompt_enable) {
    BLFont font = Lawnch::Gfx::get_font(cfg.input_prompt_font_family,
                                        cfg.input_prompt_font_size,
                                        cfg.input_prompt_font_weight);
    BLFontMetrics fm = font.metrics();
    return fm.ascent + fm.descent + cfg.input_prompt_padding.top +
           cfg.input_prompt_padding.bottom;
  }
  if (name == "results_count" && cfg.results_count_enable) {
    BLFont font = Lawnch::Gfx::get_font(cfg.results_count_font_family,
                                        cfg.results_count_font_size,
                                        cfg.results_count_font_weight);
    BLFontMetrics fm = font.metrics();
    return fm.ascent + fm.descent + cfg.results_count_padding.top +
           cfg.results_count_padding.bottom;
  }
  if (name == "clock" && cfg.clock_enable) {
    BLFont font = Lawnch::Gfx::get_font(
        cfg.clock_font_family, cfg.clock_font_size, cfg.clock_font_weight);
    BLFontMetrics fm = font.metrics();
    return fm.ascent + fm.descent + cfg.clock_padding.top +
           cfg.clock_padding.bottom;
  }
  if (name == "preview" && cfg.preview_enable && !state.results.empty()) {
    return Components::Preview::get_height(cfg, state);
  }
  return 0;
}

std::vector<std::string>
get_unique_order(const std::vector<std::string> &order) {
  std::vector<std::string> unique_order;
  std::set<std::string> seen;
  for (const auto &name : order) {
    if (seen.find(name) == seen.end()) {
      unique_order.push_back(name);
      seen.insert(name);
    }
  }
  return unique_order;
}

void Renderer::render(BLContext &ctx, int width, int height,
                      const Config::Config &cfg, const RenderState &state) {
  if (!cached_metrics.valid)
    update_metrics(cfg);

  if (components.count("background")) {
    ComponentContext bg_ctx{ctx,
                            width,
                            height,
                            cfg,
                            state,
                            0,
                            0,
                            static_cast<double>(width),
                            static_cast<double>(height)};
    components["background"]->draw(bg_ctx);
  }

  bool use_side_preview =
      cfg.preview_enable && (cfg.layout_preview_position == "left" ||
                             cfg.layout_preview_position == "right");

  if (use_side_preview && cfg.layout_preview_position == "left") {
    render_with_side_preview(ctx, width, height, cfg, state, true);
  } else if (use_side_preview && cfg.layout_preview_position == "right") {
    render_with_side_preview(ctx, width, height, cfg, state, false);
  } else {
    render_vertical(ctx, width, height, cfg, state);
  }
}

void Renderer::render_vertical(BLContext &ctx, int width, int height,
                               const Config::Config &cfg,
                               const RenderState &state) {
  double available_w = width - (cfg.window_border_width * 2);
  double x = cfg.window_border_width;
  double total_available_h = height - (cfg.window_border_width * 2);

  auto unique_order = get_unique_order(cfg.layout_order);

  double fixed_heights = 0;
  for (const auto &comp_name : unique_order) {
    if (comp_name == "background")
      continue;
    if (comp_name == "results")
      continue;
    fixed_heights += estimate_component_height(comp_name, cfg, state);
  }

  double results_h = total_available_h - fixed_heights;
  if (results_h < 0)
    results_h = 0;

  double current_y = cfg.window_border_width;

  for (const auto &comp_name : unique_order) {
    if (comp_name == "background")
      continue;

    auto it = components.find(comp_name);
    if (it == components.end())
      continue;

    double comp_h;
    if (comp_name == "results") {
      comp_h = results_h;
    } else {
      comp_h = estimate_component_height(comp_name, cfg, state);
    }

    if (comp_h <= 0)
      continue;

    if (comp_name == "input") {
      if (!cfg.input_visible)
        continue;
      auto input_comp = it->second.get();
      auto prompt_comp_it = components.find("input_prompt");
      auto prompt_comp =
          dynamic_cast<Components::InputPrompt *>(prompt_comp_it->second.get());

      if (prompt_comp && cfg.input_prompt_enable) {
        double prompt_width = prompt_comp->calculate_width(cfg);
        double prompt_comp_h =
            estimate_component_height("input_prompt", cfg, state);
        double input_width = available_w - prompt_width;
        bool prompt_on_left = cfg.input_prompt_position != "right";

        double prompt_x = prompt_on_left ? x : x + input_width;
        double input_x = prompt_on_left ? x + prompt_width : x;

        ComponentContext prompt_ctx{ctx,       width,        height,
                                    cfg,       state,        prompt_x,
                                    current_y, prompt_width, prompt_comp_h};
        prompt_comp->draw(prompt_ctx);

        ComponentContext input_ctx{ctx,       width,       height,
                                   cfg,       state,       input_x,
                                   current_y, input_width, comp_h};
        auto result = input_comp->draw(input_ctx);
        current_y += result.used_height;
      } else {
        ComponentContext comp_ctx{ctx, width,     height,      cfg,   state,
                                  x,   current_y, available_w, comp_h};
        auto result = it->second->draw(comp_ctx);
        current_y += result.used_height;
      }
    } else {
      ComponentContext comp_ctx{ctx, width,     height,      cfg,   state,
                                x,   current_y, available_w, comp_h};
      auto result = it->second->draw(comp_ctx);
      current_y += result.used_height;
    }
  }
}

void Renderer::render_with_side_preview(BLContext &ctx, int width, int height,
                                        const Config::Config &cfg,
                                        const RenderState &state,
                                        bool preview_on_left) {
  double available_w = width - (cfg.window_border_width * 2);
  double preview_w = (available_w * cfg.layout_preview_width_percent) / 100.0;
  double content_w = available_w - preview_w;

  double preview_x = preview_on_left ? cfg.window_border_width
                                     : cfg.window_border_width + content_w;
  double content_x = preview_on_left ? cfg.window_border_width + preview_w
                                     : cfg.window_border_width;

  double total_available_h = height - (cfg.window_border_width * 2);

  if (cfg.preview_enable && !state.results.empty()) {
    auto it = components.find("preview");
    if (it != components.end()) {
      ComponentContext preview_ctx{ctx,
                                   width,
                                   height,
                                   cfg,
                                   state,
                                   preview_x,
                                   static_cast<double>(cfg.window_border_width),
                                   preview_w,
                                   total_available_h};
      it->second->draw(preview_ctx);
    }
  }

  auto unique_order = get_unique_order(cfg.layout_order);
  double fixed_heights = 0;
  for (const auto &comp_name : unique_order) {
    if (comp_name == "background" || comp_name == "preview" ||
        comp_name == "results")
      continue;
    fixed_heights += estimate_component_height(comp_name, cfg, state);
  }

  double results_h = total_available_h - fixed_heights;
  if (results_h < 0)
    results_h = 0;

  double current_y = cfg.window_border_width;

  for (const auto &comp_name : unique_order) {
    if (comp_name == "background" || comp_name == "preview")
      continue;

    auto it = components.find(comp_name);
    if (it == components.end())
      continue;

    double comp_h;
    if (comp_name == "results") {
      comp_h = results_h;
    } else {
      comp_h = estimate_component_height(comp_name, cfg, state);
    }

    if (comp_h <= 0)
      continue;

    if (comp_name == "input") {
      auto input_comp = it->second.get();
      auto prompt_comp_it = components.find("input_prompt");
      auto prompt_comp =
          dynamic_cast<Components::InputPrompt *>(prompt_comp_it->second.get());

      if (prompt_comp && cfg.input_prompt_enable) {
        double prompt_width = prompt_comp->calculate_width(cfg);
        double prompt_comp_h =
            estimate_component_height("input_prompt", cfg, state);
        double input_width = content_w - prompt_width;
        bool prompt_on_left = cfg.input_prompt_position != "right";

        double prompt_x = prompt_on_left ? content_x : content_x + input_width;
        double input_x = prompt_on_left ? content_x + prompt_width : content_x;

        ComponentContext prompt_ctx{ctx,       width,        height,
                                    cfg,       state,        prompt_x,
                                    current_y, prompt_width, prompt_comp_h};
        prompt_comp->draw(prompt_ctx);

        ComponentContext input_ctx{ctx,       width,       height,
                                   cfg,       state,       input_x,
                                   current_y, input_width, comp_h};
        auto result = input_comp->draw(input_ctx);
        current_y += result.used_height;
      } else {
        ComponentContext comp_ctx{ctx,       width,     height,    cfg,   state,
                                  content_x, current_y, content_w, comp_h};
        auto result = it->second->draw(comp_ctx);
        current_y += result.used_height;
      }
    } else {
      ComponentContext comp_ctx{ctx,       width,     height,    cfg,   state,
                                content_x, current_y, content_w, comp_h};
      auto result = it->second->draw(comp_ctx);
      current_y += result.used_height;
    }
  }
}

int Renderer::get_visible_count(int height, const Config::Config &cfg) {
  if (!cached_metrics.valid)
    update_metrics(cfg);
  if (cached_metrics.item_height <= 0)
    return 1;

  double input_h_approx = (cfg.input_font_size * 1.5) +
                          (cfg.input_padding.top + cfg.input_padding.bottom);

  double count_h = 0;
  if (cfg.results_count_enable) {
    count_h = cfg.results_count_font_size * 1.5 +
              cfg.results_count_padding.top + cfg.results_count_padding.bottom;
  }

  double preview_h = 0;
  if (cfg.preview_enable && cfg.layout_preview_position == "bottom") {
    preview_h = cfg.preview_icon_size + cfg.preview_padding.top +
                cfg.preview_padding.bottom;
  }

  double clock_h = 0;
  if (cfg.clock_enable) {
    clock_h = cfg.clock_font_size * 1.5 + cfg.clock_padding.top +
              cfg.clock_padding.bottom;
  }

  double start_y = cfg.window_border_width + input_h_approx + count_h;
  double results_start_y = start_y + cfg.results_margin.top;
  double available_h = height - results_start_y - cfg.results_margin.bottom -
                       preview_h - clock_h - cfg.results_padding.top -
                       cfg.results_padding.bottom;

  return std::max(1, (int)std::floor(available_h / cached_metrics.item_height));
}

void Renderer::update_metrics(const Config::Config &cfg) {
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

  cached_metrics.item_height =
      (cfg.result_item_padding.top + cfg.result_item_padding.bottom) +
      item_inner_h + cfg.results_spacing;
  cached_metrics.valid = true;
}

} // namespace Lawnch::Core::Window::Render
