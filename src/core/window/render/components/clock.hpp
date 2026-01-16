#pragma once

#include "component_base.hpp"

namespace Lawnch::Core::Window::Render::Components {

class Clock : public ComponentBase {
public:
  ComponentResult draw(ComponentContext &context) override;
  std::string name() const override { return "clock"; }
};

} // namespace Lawnch::Core::Window::Render::Components
