#pragma once

#include <blend2d.h>
#include <string>
#include <vector>

#include "../../config/config.hpp"
#include "../../search/interface.hpp"

namespace Lawnch::Core::Window::Render {

struct RenderState {
  std::string search_text;
  int caret_position;
  bool input_selected;

  std::vector<Search::SearchResult> results;
  int selected_index;
  int scroll_offset;
};

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

  void update_metrics(const Config::Config &cfg);
};

} // namespace Lawnch::Core::Window::Render
