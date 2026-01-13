#include "history.hpp"

namespace Lawnch::Core::Window::Input {

void History::push(const std::string &state) {
  if (undo_stack.size() >= max_size) {
    undo_stack.erase(undo_stack.begin());
  }
  undo_stack.push_back(state);
  redo_stack.clear();
}

std::string History::pop_undo() {
  if (undo_stack.empty())
    return "";
  std::string s = undo_stack.back();
  undo_stack.pop_back();

  return s;
}

void History::clear() {
  undo_stack.clear();
  redo_stack.clear();
}

} // namespace Lawnch::Core::Window::Input
