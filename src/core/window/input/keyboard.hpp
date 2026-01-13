#pragma once

#include "history.hpp"
#include <functional>
#include <string>
#include <vector>
#include <xkbcommon/xkbcommon.h>

namespace Lawnch::Core::Window::Input {

struct KeyboardCallbacks {
  std::function<void()> on_update;
  std::function<void(std::string)> on_execute;
  std::function<void()> on_stop;
  std::function<void()> on_render;
};

class Keyboard {
public:
  Keyboard(History &history, KeyboardCallbacks callbacks);

  void handle_key(uint32_t keycode, xkb_keysym_t sym,
                  struct xkb_state *xkb_state);

  void set_result_count(int count);
  void set_results_command(int index, const std::string &cmd);
  std::string get_result_command(int index) const;

  const std::string &get_text() const { return search_text; }
  int get_caret() const { return caret_position; }
  int get_selected_index() const { return selected_index; }
  void set_selected_index(int idx) { selected_index = idx; }
  bool is_input_selected() const { return input_selected; }

  void clear();

private:
  History &history;
  KeyboardCallbacks callbacks;

  std::string search_text;
  int caret_position = 0;
  int selected_index = 0;
  bool input_selected = false;

  int result_count = 0;
  std::vector<std::string> current_commands;

  void push_undo();
  bool is_modifier(xkb_keysym_t sym);
  bool is_continuation_byte(char c);
};

} // namespace Lawnch::Core::Window::Input
