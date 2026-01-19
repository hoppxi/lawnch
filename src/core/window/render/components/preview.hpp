#pragma once

#include "component_base.hpp"
#include <string>
#include <vector>

namespace Lawnch::Core::Window::Render::Components {

class Preview : public ComponentBase {
public:
  ComponentResult draw(ComponentContext &context) override;
  std::string name() const override { return "preview"; }

  static double get_height(const Config::Config &cfg, const RenderState &state);
};

} // namespace Lawnch::Core::Window::Render::Components
