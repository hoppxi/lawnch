#include "renderer.hpp"
#include "../../../helpers/gfx.hpp"
#include "../../../helpers/string.hpp"
#include "../../icons/manager.hpp"
#include "components/background.hpp"
#include "components/breadcrumbs.hpp"
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

namespace {
bool is_horizontal(const std::string &dir) {
  return dir == "h" || dir == "horizontal";
}
} // namespace

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
  components["breadcrumbs"] = std::make_unique<Components::Breadcrumbs>();
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
           cfg.input_padding.bottom + cfg.input_margin.top +
           cfg.input_margin.bottom;
  }
  if (name == "input_prompt" && cfg.input_prompt_enable) {
    BLFont font = Lawnch::Gfx::get_font(cfg.input_prompt_font_family,
                                        cfg.input_prompt_font_size,
                                        cfg.input_prompt_font_weight);
    BLFontMetrics fm = font.metrics();
    return fm.ascent + fm.descent + cfg.input_prompt_padding.top +
           cfg.input_prompt_padding.bottom + cfg.input_prompt_margin.top +
           cfg.input_prompt_margin.bottom;
  }
  if (name == "results_count" && cfg.results_count_enable) {
    BLFont font = Lawnch::Gfx::get_font(cfg.results_count_font_family,
                                        cfg.results_count_font_size,
                                        cfg.results_count_font_weight);
    BLFontMetrics fm = font.metrics();
    return fm.ascent + fm.descent + cfg.results_count_padding.top +
           cfg.results_count_padding.bottom + cfg.results_count_margin.top +
           cfg.results_count_margin.bottom;
  }
  if (name == "clock" && cfg.clock_enable) {
    BLFont font = Lawnch::Gfx::get_font(
        cfg.clock_font_family, cfg.clock_font_size, cfg.clock_font_weight);
    BLFontMetrics fm = font.metrics();
    return fm.ascent + fm.descent + cfg.clock_padding.top +
           cfg.clock_padding.bottom + cfg.clock_margin.top +
           cfg.clock_margin.bottom;
  }
  if (name == "breadcrumbs" && cfg.breadcrumbs_enable &&
      !state.breadcrumb_trail.empty()) {
    BLFont font = Lawnch::Gfx::get_font(
        cfg.breadcrumbs_font_family, cfg.breadcrumbs_font_size,
        cfg.breadcrumbs_font_weight, cfg.breadcrumbs_font_path);
    BLFontMetrics fm = font.metrics();
    return fm.ascent + fm.descent + cfg.breadcrumbs_padding.top +
           cfg.breadcrumbs_padding.bottom + cfg.breadcrumbs_margin.top +
           cfg.breadcrumbs_margin.bottom;
  }
  if (name == "preview" && cfg.preview_enable && !state.results.empty()) {
    return Components::Preview::get_height(cfg, state) +
           cfg.preview_margin.top + cfg.preview_margin.bottom;
  }
  return 0;
}

double estimate_component_width(const std::string &name,
                                const Config::Config &cfg,
                                const RenderState &state) {
  if (name == "input") {
    if (!cfg.input_visible)
      return 0;
    BLFont font = Lawnch::Gfx::get_font(
        cfg.input_font_family, cfg.input_font_size, cfg.input_font_weight);
    BLTextMetrics tm;
    BLGlyphBuffer gb;
    std::string text = cfg.input_placeholder_text.empty()
                           ? "Lawnch..."
                           : cfg.input_placeholder_text;
    gb.set_utf8_text(text.c_str(), text.size());
    font.shape(gb);
    font.get_text_metrics(gb, tm);
    double prompt_w = 0;
    if (cfg.input_prompt_enable) {
      BLFont pfont = Lawnch::Gfx::get_font(cfg.input_prompt_font_family,
                                           cfg.input_prompt_font_size,
                                           cfg.input_prompt_font_weight);
      BLTextMetrics ptm;
      BLGlyphBuffer pgb;
      std::string ptext =
          cfg.input_prompt_text.empty() ? ">>" : cfg.input_prompt_text;
      pgb.set_utf8_text(ptext.c_str(), ptext.size());
      pfont.shape(pgb);
      pfont.get_text_metrics(pgb, ptm);
      prompt_w = ptm.advance.x + cfg.input_prompt_padding.left +
                 cfg.input_prompt_padding.right + cfg.input_prompt_margin.left +
                 cfg.input_prompt_margin.right;
    }
    return prompt_w + std::max(120.0, tm.advance.x + 30.0) +
           cfg.input_padding.left + cfg.input_padding.right +
           cfg.input_margin.left + cfg.input_margin.right;
  }
  if (name == "results_count" && cfg.results_count_enable) {
    BLFont font = Lawnch::Gfx::get_font(cfg.results_count_font_family,
                                        cfg.results_count_font_size,
                                        cfg.results_count_font_weight);
    BLTextMetrics tm;
    BLGlyphBuffer gb;
    std::string text = "999 results";
    gb.set_utf8_text(text.c_str(), text.size());
    font.shape(gb);
    font.get_text_metrics(gb, tm);
    return tm.advance.x + cfg.results_count_padding.left +
           cfg.results_count_padding.right + cfg.results_count_margin.left +
           cfg.results_count_margin.right;
  }
  if (name == "clock" && cfg.clock_enable) {
    BLFont font = Lawnch::Gfx::get_font(
        cfg.clock_font_family, cfg.clock_font_size, cfg.clock_font_weight);
    BLTextMetrics tm;
    BLGlyphBuffer gb;
    std::string text = "00:00 PM";
    gb.set_utf8_text(text.c_str(), text.size());
    font.shape(gb);
    font.get_text_metrics(gb, tm);
    return tm.advance.x + cfg.clock_padding.left + cfg.clock_padding.right +
           cfg.clock_margin.left + cfg.clock_margin.right;
  }
  if (name == "breadcrumbs" && cfg.breadcrumbs_enable &&
      !state.breadcrumb_trail.empty()) {
    BLFont font = Lawnch::Gfx::get_font(
        cfg.breadcrumbs_font_family, cfg.breadcrumbs_font_size,
        cfg.breadcrumbs_font_weight, cfg.breadcrumbs_font_path);
    BLTextMetrics tm;
    BLGlyphBuffer gb;
    std::string text = "Home / Sub";
    gb.set_utf8_text(text.c_str(), text.size());
    font.shape(gb);
    font.get_text_metrics(gb, tm);
    return tm.advance.x + cfg.breadcrumbs_padding.left +
           cfg.breadcrumbs_padding.right + cfg.breadcrumbs_margin.left +
           cfg.breadcrumbs_margin.right;
  }
  if (name == "preview" && cfg.preview_enable && !state.results.empty()) {
    return cfg.preview_image_size + cfg.preview_padding.left +
           cfg.preview_padding.right + cfg.preview_margin.left +
           cfg.preview_margin.right + 100.0;
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

  bool is_layout_h = is_horizontal(cfg.layout_direction);

  if (is_layout_h) {
    render_horizontal(ctx, width, height, cfg, state);
  } else {
    bool use_side_preview =
        cfg.preview_enable &&
        (cfg.layout_preview_side == "left" || cfg.layout_preview_side == "right");

    if (use_side_preview && cfg.layout_preview_side == "left") {
      render_with_side_preview(ctx, width, height, cfg, state, true);
    } else if (use_side_preview && cfg.layout_preview_side == "right") {
      render_with_side_preview(ctx, width, height, cfg, state, false);
    } else {
      render_vertical(ctx, width, height, cfg, state);
    }
  }
}

void Renderer::render_vertical(BLContext &ctx, int width, int height,
                               const Config::Config &cfg,
                               const RenderState &state) {
  double available_w = width - (cfg.window_border_width * 2) -
                       cfg.window_padding.left - cfg.window_padding.right;
  double x = cfg.window_border_width + cfg.window_padding.left;
  double total_available_h = height - (cfg.window_border_width * 2) -
                             cfg.window_padding.top - cfg.window_padding.bottom;

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

  double current_y = cfg.window_border_width + cfg.window_padding.top;

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

    // Apply margins
    double margin_top = 0;
    double margin_bottom = 0;
    double margin_left = 0;
    double margin_right = 0;

    if (comp_name == "input") {
      margin_top = cfg.input_margin.top;
      margin_bottom = cfg.input_margin.bottom;
      margin_left = cfg.input_margin.left;
      margin_right = cfg.input_margin.right;
    } else if (comp_name == "clock") {
      margin_top = cfg.clock_margin.top;
      margin_bottom = cfg.clock_margin.bottom;
      margin_left = cfg.clock_margin.left;
      margin_right = cfg.clock_margin.right;
    } else if (comp_name == "results_count") {
      margin_top = cfg.results_count_margin.top;
      margin_bottom = cfg.results_count_margin.bottom;
      margin_left = cfg.results_count_margin.left;
      margin_right = cfg.results_count_margin.right;
    } else if (comp_name == "preview") {
      margin_top = cfg.preview_margin.top;
      margin_bottom = cfg.preview_margin.bottom;
      margin_left = cfg.preview_margin.left;
      margin_right = cfg.preview_margin.right;
    } else if (comp_name == "breadcrumbs") {
      margin_top = cfg.breadcrumbs_margin.top;
      margin_bottom = cfg.breadcrumbs_margin.bottom;
      margin_left = cfg.breadcrumbs_margin.left;
      margin_right = cfg.breadcrumbs_margin.right;
    }

    current_y += margin_top;
    double draw_h = comp_h - (margin_top + margin_bottom);
    double draw_x = x + margin_left;
    double draw_w = available_w - (margin_left + margin_right);

    if (comp_name == "input") {
      if (!cfg.input_visible)
        continue;
      auto input_comp = it->second.get();
      auto prompt_comp_it = components.find("input_prompt");
      auto prompt_comp =
          dynamic_cast<Components::InputPrompt *>(prompt_comp_it->second.get());

      if (prompt_comp && cfg.input_prompt_enable) {
        double prompt_margin_left = cfg.input_prompt_margin.left;
        double prompt_margin_right = cfg.input_prompt_margin.right;
        double prompt_margin_top = cfg.input_prompt_margin.top;
        double prompt_margin_bottom = cfg.input_prompt_margin.bottom;

        double prompt_width = prompt_comp->calculate_width(cfg) +
                              prompt_margin_left + prompt_margin_right;
        double prompt_comp_h =
            estimate_component_height("input_prompt", cfg, state);

        double input_width = draw_w - prompt_width;
        bool prompt_on_left = cfg.input_prompt_side != "right";

        double prompt_draw_x = prompt_on_left
                                   ? draw_x + prompt_margin_left
                                   : draw_x + input_width + prompt_margin_left;
        double prompt_draw_y = (current_y - margin_top) + prompt_margin_top;

        double prompt_draw_h =
            prompt_comp_h - (prompt_margin_top + prompt_margin_bottom);
        double prompt_draw_w =
            prompt_width - (prompt_margin_left + prompt_margin_right);

        double input_draw_x = prompt_on_left ? draw_x + prompt_width : draw_x;

        ComponentContext prompt_ctx{
            ctx,           width,         height,        cfg,          state,
            prompt_draw_x, prompt_draw_y, prompt_draw_w, prompt_draw_h};
        prompt_comp->draw(prompt_ctx);

        ComponentContext input_ctx{ctx,       width,       height,
                                   cfg,       state,       input_draw_x,
                                   current_y, input_width, draw_h};
        input_comp->draw(input_ctx);
        current_y += draw_h + margin_bottom;
      } else {
        ComponentContext comp_ctx{ctx,    width,     height, cfg,   state,
                                  draw_x, current_y, draw_w, draw_h};
        auto result = it->second->draw(comp_ctx);
        current_y += result.used_height + margin_bottom;
      }
    } else {
      ComponentContext comp_ctx{ctx,    width,     height, cfg,   state,
                                draw_x, current_y, draw_w, draw_h};
      auto result = it->second->draw(comp_ctx);
      current_y += result.used_height + margin_bottom;
    }
  }
}

void Renderer::render_horizontal(BLContext &ctx, int width, int height,
                                 const Config::Config &cfg,
                                 const RenderState &state) {
  double total_available_w = width - (cfg.window_border_width * 2) -
                             cfg.window_padding.left - cfg.window_padding.right;
  double available_h = height - (cfg.window_border_width * 2) -
                       cfg.window_padding.top - cfg.window_padding.bottom;
  double y = cfg.window_border_width + cfg.window_padding.top;

  auto unique_order = get_unique_order(cfg.layout_order);

  double fixed_widths = 0;
  for (const auto &comp_name : unique_order) {
    if (comp_name == "background" || comp_name == "results")
      continue;
    fixed_widths += estimate_component_width(comp_name, cfg, state);
  }

  double results_w = total_available_w - fixed_widths;
  if (results_w < 0)
    results_w = 0;

  double current_x = cfg.window_border_width + cfg.window_padding.left;

  for (const auto &comp_name : unique_order) {
    if (comp_name == "background")
      continue;

    auto it = components.find(comp_name);
    if (it == components.end())
      continue;

    double comp_w;
    if (comp_name == "results") {
      comp_w = results_w;
    } else {
      comp_w = estimate_component_width(comp_name, cfg, state);
    }

    if (comp_w <= 0)
      continue;

    double margin_top = 0;
    double margin_bottom = 0;
    double margin_left = 0;
    double margin_right = 0;

    if (comp_name == "input") {
      margin_top = cfg.input_margin.top;
      margin_bottom = cfg.input_margin.bottom;
      margin_left = cfg.input_margin.left;
      margin_right = cfg.input_margin.right;
    } else if (comp_name == "clock") {
      margin_top = cfg.clock_margin.top;
      margin_bottom = cfg.clock_margin.bottom;
      margin_left = cfg.clock_margin.left;
      margin_right = cfg.clock_margin.right;
    } else if (comp_name == "results_count") {
      margin_top = cfg.results_count_margin.top;
      margin_bottom = cfg.results_count_margin.bottom;
      margin_left = cfg.results_count_margin.left;
      margin_right = cfg.results_count_margin.right;
    } else if (comp_name == "preview") {
      margin_top = cfg.preview_margin.top;
      margin_bottom = cfg.preview_margin.bottom;
      margin_left = cfg.preview_margin.left;
      margin_right = cfg.preview_margin.right;
    } else if (comp_name == "breadcrumbs") {
      margin_top = cfg.breadcrumbs_margin.top;
      margin_bottom = cfg.breadcrumbs_margin.bottom;
      margin_left = cfg.breadcrumbs_margin.left;
      margin_right = cfg.breadcrumbs_margin.right;
    }

    current_x += margin_left;
    double draw_w = comp_w - (margin_left + margin_right);
    double draw_y = y + margin_top;
    double draw_h = available_h - (margin_top + margin_bottom);

    if (comp_name == "input") {
      if (!cfg.input_visible)
        continue;
      auto input_comp = it->second.get();
      auto prompt_comp_it = components.find("input_prompt");
      auto prompt_comp =
          dynamic_cast<Components::InputPrompt *>(prompt_comp_it->second.get());

      if (prompt_comp && cfg.input_prompt_enable) {
        double prompt_margin_left = cfg.input_prompt_margin.left;
        double prompt_margin_right = cfg.input_prompt_margin.right;
        double prompt_margin_top = cfg.input_prompt_margin.top;
        double prompt_margin_bottom = cfg.input_prompt_margin.bottom;

        double prompt_width = prompt_comp->calculate_width(cfg) +
                              prompt_margin_left + prompt_margin_right;
        double input_width = draw_w - prompt_width;
        bool prompt_on_left = cfg.input_prompt_side != "right";

        double prompt_draw_x = prompt_on_left
                                   ? current_x + prompt_margin_left
                                   : current_x + input_width + prompt_margin_left;
        double prompt_draw_y = draw_y + prompt_margin_top;
        double prompt_draw_h = draw_h - (prompt_margin_top + prompt_margin_bottom);
        double prompt_draw_w =
            prompt_width - (prompt_margin_left + prompt_margin_right);

        double input_draw_x =
            prompt_on_left ? current_x + prompt_width : current_x;

        ComponentContext prompt_ctx{
            ctx,           width,         height,        cfg,          state,
            prompt_draw_x, prompt_draw_y, prompt_draw_w, prompt_draw_h};
        prompt_comp->draw(prompt_ctx);

        ComponentContext input_ctx{ctx,       width,       height,
                                   cfg,       state,       input_draw_x,
                                   draw_y,    input_width, draw_h};
        input_comp->draw(input_ctx);
      } else {
        ComponentContext comp_ctx{ctx,       width,  height, cfg,   state,
                                  current_x, draw_y, draw_w, draw_h};
        it->second->draw(comp_ctx);
      }
    } else {
      ComponentContext comp_ctx{ctx,       width,  height, cfg,   state,
                                current_x, draw_y, draw_w, draw_h};
      it->second->draw(comp_ctx);
    }

    current_x += draw_w + margin_right;
  }
}

void Renderer::render_with_side_preview(BLContext &ctx, int width, int height,
                                        const Config::Config &cfg,
                                        const RenderState &state,
                                        bool preview_on_left) {
  double available_w = width - (cfg.window_border_width * 2) -
                       cfg.window_padding.left - cfg.window_padding.right;
  double preview_w = (available_w * cfg.layout_preview_ratio) / 100.0;
  double content_w = available_w - preview_w;

  double preview_x =
      preview_on_left
          ? cfg.window_border_width + cfg.window_padding.left
          : cfg.window_border_width + cfg.window_padding.left + content_w;
  double content_x =
      preview_on_left
          ? cfg.window_border_width + cfg.window_padding.left + preview_w
          : cfg.window_border_width + cfg.window_padding.left;

  double total_available_h = height - (cfg.window_border_width * 2) -
                             cfg.window_padding.top - cfg.window_padding.bottom;

  if (cfg.preview_enable && !state.results.empty()) {
    auto it = components.find("preview");
    if (it != components.end()) {
      double p_margin_left = cfg.preview_margin.left;
      double p_margin_right = cfg.preview_margin.right;
      double p_margin_top = cfg.preview_margin.top;
      double p_margin_bottom = cfg.preview_margin.bottom;

      double p_draw_x = preview_x + p_margin_left;
      double p_draw_w = preview_w - (p_margin_left + p_margin_right);
      double p_draw_y =
          cfg.window_border_width + cfg.window_padding.top + p_margin_top;
      double p_draw_h = total_available_h - (p_margin_top + p_margin_bottom);

      ComponentContext preview_ctx{
          ctx,
          width,
          height,
          cfg,
          state,
          p_draw_x,
          p_draw_y,
          p_draw_w,
          p_draw_h};
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

  double current_y = cfg.window_border_width + cfg.window_padding.top;

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

    double margin_top = 0;
    double margin_bottom = 0;
    double margin_left = 0;
    double margin_right = 0;

    if (comp_name == "input") {
      margin_top = cfg.input_margin.top;
      margin_bottom = cfg.input_margin.bottom;
      margin_left = cfg.input_margin.left;
      margin_right = cfg.input_margin.right;
    } else if (comp_name == "clock") {
      margin_top = cfg.clock_margin.top;
      margin_bottom = cfg.clock_margin.bottom;
      margin_left = cfg.clock_margin.left;
      margin_right = cfg.clock_margin.right;
    } else if (comp_name == "results_count") {
      margin_top = cfg.results_count_margin.top;
      margin_bottom = cfg.results_count_margin.bottom;
      margin_left = cfg.results_count_margin.left;
      margin_right = cfg.results_count_margin.right;
    } else if (comp_name == "breadcrumbs") {
      margin_top = cfg.breadcrumbs_margin.top;
      margin_bottom = cfg.breadcrumbs_margin.bottom;
      margin_left = cfg.breadcrumbs_margin.left;
      margin_right = cfg.breadcrumbs_margin.right;
    }

    current_y += margin_top;
    double draw_h = comp_h - (margin_top + margin_bottom);
    double draw_x = content_x + margin_left;
    double draw_w = content_w - (margin_left + margin_right);

    if (comp_name == "input") {
      auto input_comp = it->second.get();
      auto prompt_comp_it = components.find("input_prompt");
      auto prompt_comp =
          dynamic_cast<Components::InputPrompt *>(prompt_comp_it->second.get());

      if (prompt_comp && cfg.input_prompt_enable) {
        double prompt_margin_left = cfg.input_prompt_margin.left;
        double prompt_margin_right = cfg.input_prompt_margin.right;
        double prompt_margin_top = cfg.input_prompt_margin.top;
        double prompt_margin_bottom = cfg.input_prompt_margin.bottom;

        double prompt_width = prompt_comp->calculate_width(cfg) +
                              prompt_margin_left + prompt_margin_right;
        double prompt_comp_h =
            estimate_component_height("input_prompt", cfg, state);

        double input_width = draw_w - prompt_width;
        bool prompt_on_left = cfg.input_prompt_side != "right";

        double prompt_draw_y = (current_y - margin_top) + prompt_margin_top;
        double prompt_draw_h =
            prompt_comp_h - (prompt_margin_top + prompt_margin_bottom);
        double prompt_draw_w =
            prompt_width - (prompt_margin_left + prompt_margin_right);

        double prompt_draw_x = prompt_on_left
                                   ? draw_x + prompt_margin_left
                                   : draw_x + input_width + prompt_margin_left;

        double input_draw_x = prompt_on_left ? draw_x + prompt_width : draw_x;

        ComponentContext prompt_ctx{
            ctx,           width,         height,        cfg,          state,
            prompt_draw_x, prompt_draw_y, prompt_draw_w, prompt_draw_h};
        prompt_comp->draw(prompt_ctx);

        ComponentContext input_ctx{ctx,       width,       height,
                                   cfg,       state,       input_draw_x,
                                   current_y, input_width, draw_h};
        input_comp->draw(input_ctx);
        current_y += draw_h + margin_bottom;
      } else {
        ComponentContext comp_ctx{ctx,    width,     height, cfg,   state,
                                  draw_x, current_y, draw_w, draw_h};
        auto result = it->second->draw(comp_ctx);
        current_y += result.used_height + margin_bottom;
      }
    } else {
      ComponentContext comp_ctx{ctx,    width,     height, cfg,   state,
                                draw_x, current_y, draw_w, draw_h};
      auto result = it->second->draw(comp_ctx);
      current_y += result.used_height + margin_bottom;
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
  if (cfg.preview_enable && cfg.layout_preview_side == "bottom") {
    preview_h = cfg.preview_image_size + cfg.preview_padding.top +
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
                       cfg.results_padding.bottom - cfg.window_padding.top -
                       cfg.window_padding.bottom;

  int cols = is_horizontal(cfg.results_direction)
                 ? std::max(1, cfg.results_column)
                 : 1;
  int visible_rows =
      std::max(1, (int)std::floor(available_h / cached_metrics.item_height));
  return visible_rows * cols;
}

void Renderer::update_metrics(const Config::Config &cfg) {
  BLFont font = Lawnch::Gfx::get_font(cfg.result_item_default_font_family,
                                      cfg.result_item_default_font_size,
                                      cfg.result_item_default_font_weight);
  BLFontMetrics metrics = font.metrics();
  double name_h = metrics.ascent + metrics.descent;

  double comment_h = 0;
  if (cfg.result_item_comment_enable) {
    BLFont cfont =
        Lawnch::Gfx::get_font(cfg.result_item_default_font_family,
                              cfg.result_item_default_comment_font_size,
                              cfg.result_item_default_comment_font_weight);
    BLFontMetrics cmetrics = cfont.metrics();
    comment_h = cmetrics.ascent + cmetrics.descent;
  }

  double text_gap = 4.0;
  bool item_h_mode = is_horizontal(cfg.result_item_direction);

  double item_inner_h = 0;
  if (item_h_mode) {
    double icon_h = cfg.result_item_icon_enable
                        ? (cfg.result_item_icon_size + cfg.result_item_icon_gap)
                        : 0;
    item_inner_h = icon_h + name_h;
    if (cfg.result_item_comment_enable) {
      item_inner_h += text_gap + comment_h;
    }
  } else {
    double text_h = name_h;
    if (cfg.result_item_comment_enable) {
      text_h += text_gap + comment_h;
    }
    double icon_h = cfg.result_item_icon_enable ? cfg.result_item_icon_size : 0;
    item_inner_h = std::max(icon_h, text_h);
  }

  cached_metrics.item_height = (cfg.result_item_default_padding.top +
                                cfg.result_item_default_padding.bottom) +
                               item_inner_h + cfg.results_gap;
  cached_metrics.valid = true;
}

} // namespace Lawnch::Core::Window::Render
