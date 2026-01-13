#include "pointer.hpp"
#include <cmath>

namespace Lawnch::Core::Window::Input {

Pointer::Pointer(PointerCallbacks cb) : callbacks(cb) {}

void Pointer::handle_axis(double d) {
  int steps = 0;
  if (d > 5.0)
    steps = 1;
  else if (d < -5.0)
    steps = -1;

  if (steps != 0) {
    callbacks.on_scroll((double)steps);
  }
}

} // namespace Lawnch::Core::Window::Input
