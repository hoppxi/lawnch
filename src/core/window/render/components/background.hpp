#pragma once

#include "component_base.hpp"

namespace Lawnch::Core::Window::Render::Components {

class Background : public ComponentBase {
public:
  ComponentResult draw(ComponentContext &context) override;
  std::string name() const override { return "background"; }
};

} // namespace Lawnch::Core::Window::Render::Components
