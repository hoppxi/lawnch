#pragma once

#include "../../search/interface.hpp"
#include <string>
#include <vector>

namespace Lawnch::Core::Window::Render {

struct RenderState {
  std::string search_text;
  int caret_position;
  bool input_selected;

  std::vector<Search::SearchResult> results;
  int selected_index;
  int scroll_offset;

  std::vector<std::string> breadcrumb_trail;
};

} // namespace Lawnch::Core::Window::Render
