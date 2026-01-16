#pragma once

#include <blend2d.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../config/config.hpp"
#include "components/component_base.hpp"
#include "render_state.hpp"

namespace Lawnch::Core::Window::Render {

class Renderer {
public:
  Renderer();
  ~Renderer();

  void render(BLContext &ctx, int width, int height, const Config::Config &cfg,
              const RenderState &state);
  int get_visible_count(int height, const Config::Config &cfg);

private:
  struct Metrics {
    double item_height = 0;
    bool valid = false;
  } cached_metrics;

  std::map<std::string, std::unique_ptr<ComponentBase>> components;

  void init_components();
  void update_metrics(const Config::Config &cfg);
  void render_vertical(BLContext &ctx, int width, int height,
                       const Config::Config &cfg, const RenderState &state);
  void render_with_side_preview(BLContext &ctx, int width, int height,
                                const Config::Config &cfg,
                                const RenderState &state, bool preview_on_left);
};

} // namespace Lawnch::Core::Window::Render
