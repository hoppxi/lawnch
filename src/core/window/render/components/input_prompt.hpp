#pragma once

#include "component_base.hpp"

namespace Lawnch::Core::Window::Render::Components {

class InputPrompt : public ComponentBase {
public:
  ComponentResult draw(ComponentContext &context) override;
  std::string name() const override { return "input_prompt"; }
  double calculate_width(const Config::Config &cfg);
};

} // namespace Lawnch::Core::Window::Render::Components
