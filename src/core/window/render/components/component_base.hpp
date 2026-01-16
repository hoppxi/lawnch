#pragma once

#include <blend2d.h>
#include <string>

#include "../../../config/config.hpp"
#include "../../../search/interface.hpp"

namespace Lawnch::Core::Window::Render {

struct RenderState;

struct ComponentContext {
  BLContext &ctx;
  int window_width;
  int window_height;
  const Config::Config &cfg;
  const RenderState &state;

  double x;
  double y;
  double available_w;
  double available_h;
};

struct ComponentResult {
  double used_width = 0;
  double used_height = 0;
};

class ComponentBase {
public:
  virtual ~ComponentBase() = default;
  virtual ComponentResult draw(ComponentContext &context) = 0;
  virtual std::string name() const = 0;
};

} // namespace Lawnch::Core::Window::Render
