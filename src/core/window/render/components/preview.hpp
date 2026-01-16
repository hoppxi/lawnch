#pragma once

#include "component_base.hpp"

namespace Lawnch::Core::Window::Render::Components {

class Preview : public ComponentBase {
public:
  ComponentResult draw(ComponentContext &context) override;
  std::string name() const override { return "preview"; }
  static double get_height(const Config::Config &cfg);
};

} // namespace Lawnch::Core::Window::Render::Components
