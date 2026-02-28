#include "keyboard.hpp"
#include "../../../helpers/process.hpp"
#include "../../config/manager.hpp"
#include "keybinding_manager.hpp"
#include <iostream>

namespace Lawnch::Core::Window::Input {

Keyboard::Keyboard(History &h, KeyboardCallbacks cb)
    : history(h), callbacks(cb) {
  KeybindingManager::Instance().load_config();
}

void Keyboard::set_result_count(int count) {
  result_count = count;
  current_commands.resize(count);
  if (selected_index >= count)
    selected_index = std::max(0, count - 1);
}

void Keyboard::set_text(const std::string &text, bool notify) {
  search_text = text;
  caret_position = search_text.size();
  if (notify) {
    callbacks.on_update();
  }
}

void Keyboard::set_results_command(int index, const std::string &cmd) {
  if (index >= 0 && index < (int)current_commands.size()) {
    current_commands[index] = cmd;
  }
}

std::string Keyboard::get_result_command(int index) const {
  if (index >= 0 && index < (int)current_commands.size()) {
    return current_commands[index];
  }
  return "";
}

void Keyboard::clear() {
  search_text.clear();
  caret_position = 0;
  input_selected = false;
  selected_index = 0;
  result_count = 0;
  current_commands.clear();
}

void Keyboard::push_undo() { history.push(search_text); }

bool Keyboard::is_continuation_byte(char c) { return (c & 0xC0) == 0x80; }

void Keyboard::handle_key(uint32_t keycode, xkb_keysym_t sym,
                          struct xkb_state *xkb_state) {

  uint32_t mods = 0;
  auto effective = static_cast<xkb_state_component>(XKB_STATE_MODS_DEPRESSED |
                                                    XKB_STATE_MODS_LATCHED);

  if (xkb_state_mod_name_is_active(xkb_state, XKB_MOD_NAME_CTRL, effective))
    mods |= MOD_CTRL;
  if (xkb_state_mod_name_is_active(xkb_state, XKB_MOD_NAME_ALT, effective))
    mods |= MOD_ALT;
  if (xkb_state_mod_name_is_active(xkb_state, XKB_MOD_NAME_SHIFT, effective))
    mods |= MOD_SHIFT;
  if (xkb_state_mod_name_is_active(xkb_state, "Mod4", effective) ||
      xkb_state_mod_name_is_active(xkb_state, XKB_MOD_NAME_LOGO, effective))
    mods |= MOD_SUPER;

  Action action = KeybindingManager::Instance().get_action(sym, mods);

  switch (action) {
  case Action::SELECT_INPUT:
    if (!search_text.empty()) {
      input_selected = true;
      callbacks.on_render();
    }
    break;

  case Action::COPY:
    if (!search_text.empty()) {
      std::string escaped;
      for (char c : search_text) {
        if (c == '\'')
          escaped += "'\\''";
        else
          escaped += c;
      }
      std::string cmd = "echo -n '" + escaped + "' | wl-copy";
      Lawnch::Proc::exec_detached(cmd);
    }
    break;

  case Action::CUT:
    if (!search_text.empty()) {
      push_undo();
      std::string cmd = "echo -n '" + search_text + "' | wl-copy";
      Lawnch::Proc::exec_detached(cmd);
      search_text.clear();
      caret_position = 0;
      input_selected = false;
      callbacks.on_update();
    }
    break;

  case Action::PASTE: {
    std::string clipboard = Lawnch::Proc::exec("wl-paste --no-newline");
    if (!clipboard.empty()) {
      push_undo();
      if (input_selected) {
        search_text.clear();
        caret_position = 0;
        input_selected = false;
      }
      search_text.insert(caret_position, clipboard);
      caret_position += clipboard.size();
      callbacks.on_update();
    }
    break;
  }

  case Action::CLEAR_INPUT:
    if (!search_text.empty()) {
      push_undo();
      search_text.clear();
      caret_position = 0;
      input_selected = false;
      callbacks.on_update();
    }
    break;

  case Action::UNDO: {
    std::string s = history.pop_undo();
    if (!s.empty()) {
      search_text = s;
      caret_position = search_text.size();
      input_selected = false;
      callbacks.on_update();
    }
  } break;

  case Action::EXIT:
    if (input_selected) {
      input_selected = false;
      callbacks.on_render();
    } else {
      callbacks.on_stop();
    }
    break;

  case Action::EXECUTE:
    if (selected_index >= 0 && selected_index < result_count) {
      callbacks.on_execute(get_result_command(selected_index));
    }
    break;

  case Action::NAV_UP:
  case Action::QUERY_HISTORY_UP:
    if (reverse_navigation) {
      if (selected_index < result_count - 1) {
        selected_index++;
        callbacks.on_render();
      }
    } else {
      if (selected_index > 0) {
        selected_index--;
        callbacks.on_render();
      }
    }
    break;

  case Action::NAV_DOWN:
  case Action::QUERY_HISTORY_DOWN:
    if (reverse_navigation) {
      if (selected_index > 0) {
        selected_index--;
        callbacks.on_render();
      }
    } else {
      if (selected_index < result_count - 1) {
        selected_index++;
        callbacks.on_render();
      }
    }
    break;

  case Action::NAV_LEFT:
    if (caret_position > 0) {
      caret_position--;
      while (caret_position > 0 &&
             is_continuation_byte(search_text[caret_position])) {
        caret_position--;
      }
      if (input_selected)
        input_selected = false;
      callbacks.on_render();
    }
    break;

  case Action::NAV_RIGHT:
    if (caret_position < (int)search_text.size()) {
      caret_position++;
      while (caret_position < (int)search_text.size() &&
             is_continuation_byte(search_text[caret_position])) {
        caret_position++;
      }
      if (input_selected)
        input_selected = false;
      callbacks.on_render();
    }
    break;

  case Action::NAV_WORD_BACK:
    if (caret_position > 0) {
      int pos = caret_position;
      while (pos > 0 && search_text[pos - 1] == ' ')
        pos--;
      while (pos > 0 && search_text[pos - 1] != ' ')
        pos--;

      if (pos < caret_position) {
        caret_position = pos;
        if (input_selected)
          input_selected = false;
        callbacks.on_render();
      }
    }
    break;

  case Action::NAV_WORD_FWD:
    if (caret_position < (int)search_text.size()) {
      int pos = caret_position;
      while (pos < (int)search_text.size() && search_text[pos] != ' ')
        pos++;
      while (pos < (int)search_text.size() && search_text[pos] == ' ')
        pos++;

      if (pos > caret_position) {
        caret_position = pos;
        if (input_selected)
          input_selected = false;
        callbacks.on_render();
      }
    }
    break;

  case Action::NAV_HOME:
    caret_position = 0;
    if (input_selected)
      input_selected = false;
    callbacks.on_render();
    break;

  case Action::NAV_END:
    caret_position = search_text.size();
    if (input_selected)
      input_selected = false;
    callbacks.on_render();
    break;

  case Action::DELETE_CHAR_BACK:
    if (input_selected) {
      search_text.clear();
      caret_position = 0;
      input_selected = false;
      callbacks.on_update();
    } else if (!search_text.empty() && caret_position > 0) {
      int prev_pos = caret_position - 1;
      while (prev_pos > 0 && is_continuation_byte(search_text[prev_pos])) {
        prev_pos--;
      }
      int len = caret_position - prev_pos;
      search_text.erase(prev_pos, len);
      caret_position = prev_pos;
      callbacks.on_update();
    }
    break;

  case Action::DELETE_CHAR_FWD:
    if (input_selected) {
      search_text.clear();
      caret_position = 0;
      input_selected = false;
      callbacks.on_update();
    } else if (caret_position < (int)search_text.size()) {
      int next_pos = caret_position + 1;
      while (next_pos < (int)search_text.size() &&
             is_continuation_byte(search_text[next_pos])) {
        next_pos++;
      }
      search_text.erase(caret_position, next_pos - caret_position);
      callbacks.on_update();
    }
    break;

  case Action::DELETE_WORD_BACK:
    if (input_selected) {
      search_text.clear();
      caret_position = 0;
      input_selected = false;
      callbacks.on_update();
    } else if (caret_position > 0) {
      int pos = caret_position;
      while (pos > 0 && search_text[pos - 1] == ' ')
        pos--;
      while (pos > 0 && search_text[pos - 1] != ' ')
        pos--;

      if (pos < caret_position) {
        search_text.erase(pos, caret_position - pos);
        caret_position = pos;
        callbacks.on_update();
      }
    }
    break;

  case Action::DELETE_WORD_FWD:
    if (input_selected) {
      search_text.clear();
      caret_position = 0;
      input_selected = false;
      callbacks.on_update();
    } else if (caret_position < (int)search_text.size()) {
      int pos = caret_position;
      while (pos < (int)search_text.size() && search_text[pos] != ' ')
        pos++;
      while (pos < (int)search_text.size() && search_text[pos] == ' ')
        pos++;

      if (pos > caret_position) {
        search_text.erase(caret_position, pos - caret_position);
        callbacks.on_update();
      }
    }
    break;

  case Action::NONE:
    char buf[128];
    if (xkb_state &&
        xkb_state_key_get_utf8(xkb_state, keycode, buf, sizeof(buf)) > 0) {
      if ((unsigned char)buf[0] < 32)
        return;

      if (input_selected) {
        search_text.clear();
        caret_position = 0;
        input_selected = false;
      }
      std::string input_str(buf);
      search_text.insert(caret_position, input_str);
      caret_position += input_str.size();
      callbacks.on_update();
    }
    break;
  }
}

} // namespace Lawnch::Core::Window::Input
