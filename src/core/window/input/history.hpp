#pragma once
#include <string>
#include <vector>

namespace Lawnch::Core::Window::Input {

class History {
public:
  void push(const std::string &state);
  std::string pop_undo();
  // TODO: implement push_redo
  void clear();
  void set_max_size(size_t size) { max_size = size; }

private:
  std::vector<std::string> undo_stack;
  std::vector<std::string> redo_stack;
  size_t max_size = 100;
};

} // namespace Lawnch::Core::Window::Input
