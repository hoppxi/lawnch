#pragma once

#include <functional>

namespace Lawnch::Core::Window::Input {

struct PointerCallbacks {
  std::function<void(double)> on_scroll; // +1 or -1 or delta
};

class Pointer {
public:
  Pointer(PointerCallbacks callbacks);
  void handle_axis(double value);

private:
  PointerCallbacks callbacks;
};

} // namespace Lawnch::Core::Window::Input
