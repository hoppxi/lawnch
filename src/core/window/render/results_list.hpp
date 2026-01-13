#pragma once

#include "../../config/config.hpp"
#include "../../search/interface.hpp"
#include <blend2d.h>
#include <vector>

namespace Lawnch::Core::Window::Render {

class ResultsList {
public:
  static void draw(BLContext &ctx, int width, int height, double start_y,
                   const Config::Config &cfg,
                   const std::vector<Search::SearchResult> &results,
                   int start_index, int selected_index, double item_height,
                   const std::string &search_text = "");
};

} // namespace Lawnch::Core::Window::Render
