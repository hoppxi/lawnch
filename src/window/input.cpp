#include "../config.hpp"
#include "../utils.hpp"
#include "window.hpp"
#include <iostream>
#include <sys/timerfd.h>
#include <xkbcommon/xkbcommon-keysyms.h>

static bool is_modifier(xkb_keysym_t sym) {
  switch (sym) {
  case XKB_KEY_Shift_L:
  case XKB_KEY_Shift_R:
  case XKB_KEY_Control_L:
  case XKB_KEY_Control_R:
  case XKB_KEY_Alt_L:
  case XKB_KEY_Alt_R:
  case XKB_KEY_Super_L:
  case XKB_KEY_Super_R:
  case XKB_KEY_Hyper_L:
  case XKB_KEY_Hyper_R:
  case XKB_KEY_Meta_L:
  case XKB_KEY_Meta_R:
  case XKB_KEY_Caps_Lock:
  case XKB_KEY_Num_Lock:
  case XKB_KEY_Scroll_Lock:
    return true;
  default:
    return false;
  }
}

// Global undo/redo stacks
static std::vector<std::string> undo_stack;
static std::vector<std::string> redo_stack;

void LauncherWindow::push_undo() {
  undo_stack.push_back(search_text);
  redo_stack.clear();
}

// check for UTF-8 continuation bytes (0b10xxxxxx)
static bool is_continuation_byte(char c) { return (c & 0xC0) == 0x80; }

void LauncherWindow::handle_key_press_action(uint32_t keycode,
                                             xkb_keysym_t sym) {
  const auto &cfg = ConfigManager::get();
  bool ctrl = xkb_state_mod_name_is_active(
      xkb_state, XKB_MOD_NAME_CTRL,
      static_cast<xkb_state_component>(XKB_STATE_MODS_DEPRESSED |
                                       XKB_STATE_MODS_LATCHED));
  bool alt = xkb_state_mod_name_is_active(
      xkb_state, XKB_MOD_NAME_ALT,
      static_cast<xkb_state_component>(XKB_STATE_MODS_DEPRESSED |
                                       XKB_STATE_MODS_LATCHED));

  if (ctrl) {
    // CTRL + A (Select All)
    if (sym == XKB_KEY_a || sym == XKB_KEY_A) {
      if (!search_text.empty()) {
        input_selected = true;
        render();
      }
      return;
    }
    // CTRL + C (Copy)
    if (sym == XKB_KEY_c || sym == XKB_KEY_C) {
      if (!search_text.empty()) {
        std::string escaped;
        for (char c : search_text) {
          if (c == '\'')
            escaped += "'\\''";
          else
            escaped += c;
        }
        std::string cmd = "echo -n '" + escaped + "' | wl-copy";
        Utils::exec_detached(cmd);
      }
      return;
    }
    // CTRL + X (Cut)
    if (sym == XKB_KEY_x || sym == XKB_KEY_X) {
      if (!search_text.empty()) {
        push_undo();
        std::string cmd = "echo -n '" + search_text + "' | wl-copy";
        Utils::exec_detached(cmd);
        search_text.clear();
        caret_position = 0;
        input_selected = false;
        update_results();
      }
      return;
    }
    // CTRL + V (Paste)
    if (sym == XKB_KEY_v || sym == XKB_KEY_V) {
      std::string clipboard = Utils::exec("wl-paste --no-newline");
      if (!clipboard.empty()) {
        push_undo();
        if (input_selected) {
          search_text.clear();
          caret_position = 0;
          input_selected = false;
        }
        search_text.insert(caret_position, clipboard);
        caret_position += clipboard.size();
        update_results();
      }
      return;
    }
    // CTRL + U (Clear input)
    if (sym == XKB_KEY_u || sym == XKB_KEY_U) {
      if (!search_text.empty()) {
        push_undo();
        search_text.clear();
        caret_position = 0;
        input_selected = false;
        update_results();
      }
      return;
    }

    // If Control is held but didn't match specific shortcuts, return to avoid
    // printing weird characters.
    return;
  }

  if (alt) {
    return;
  }

  switch (sym) {
  case XKB_KEY_Escape:
    if (input_selected) {
      input_selected = false;
      render();
    } else {
      stop();
    }
    break;
  case XKB_KEY_Return:
  case XKB_KEY_KP_Enter:
    if (selected_index >= 0 && selected_index < (int)current_results.size()) {
      Utils::exec_detached(current_results[selected_index].command);
      stop();
    }
    break;
  case XKB_KEY_Up:
    if (selected_index > 0) {
      selected_index--;
      render();
    }
    break;
  case XKB_KEY_Down:
    if (selected_index < (int)current_results.size() - 1) {
      selected_index++;
      render();
    }
    break;

  case XKB_KEY_Left:
    if (caret_position > 0) {
      // Move back one byte, then skip continuation bytes
      caret_position--;
      while (caret_position > 0 &&
             is_continuation_byte(search_text[caret_position])) {
        caret_position--;
      }
      if (input_selected)
        input_selected = false;
      render();
    }
    break;

  case XKB_KEY_Right:
    if (caret_position < (int)search_text.size()) {
      caret_position++;
      while (caret_position < (int)search_text.size() &&
             is_continuation_byte(search_text[caret_position])) {
        caret_position++;
      }
      if (input_selected)
        input_selected = false;
      render();
    }
    break;

  case XKB_KEY_Home:
    caret_position = 0;
    if (input_selected)
      input_selected = false;
    render();
    break;

  case XKB_KEY_End:
    caret_position = search_text.size();
    if (input_selected)
      input_selected = false;
    render();
    break;

  case XKB_KEY_BackSpace:
    if (input_selected) {
      search_text.clear();
      caret_position = 0;
      input_selected = false;
      update_results();
    } else if (!search_text.empty() && caret_position > 0) {
      // Find start of the character before caret
      int prev_pos = caret_position - 1;
      while (prev_pos > 0 && is_continuation_byte(search_text[prev_pos])) {
        prev_pos--;
      }

      int len = caret_position - prev_pos;
      search_text.erase(prev_pos, len);
      caret_position = prev_pos;
      update_results();
    }
    break;

  case XKB_KEY_Delete:
    if (input_selected) {
      search_text.clear();
      caret_position = 0;
      input_selected = false;
      update_results();
    } else if (caret_position < (int)search_text.size()) {
      // Find end of character at caret
      int next_pos = caret_position + 1;
      while (next_pos < (int)search_text.size() &&
             is_continuation_byte(search_text[next_pos])) {
        next_pos++;
      }
      search_text.erase(caret_position, next_pos - caret_position);
      update_results();
    }
    break;

  default:
    char buf[128];
    if (xkb_state &&
        xkb_state_key_get_utf8(xkb_state, keycode, buf, sizeof(buf)) > 0) {

      // Strict filter: If it's a control character (ASCII < 32) and not valid
      if ((unsigned char)buf[0] < 32) {
        return;
      }

      if (input_selected) {
        search_text.clear();
        caret_position = 0;
        input_selected = false;
      }

      std::string input_str(buf);
      search_text.insert(caret_position, input_str);
      caret_position += input_str.size();
      update_results();
    }
    break;
  }
}

void LauncherWindow::update_repeat_timer(uint32_t keycode, xkb_keysym_t sym,
                                         bool pressed) {
  if (!pressed) {
    last_key_code = 0;
    struct itimerspec its = {};
    timerfd_settime(repeat_timer_fd, 0, &its, NULL);
    return;
  }

  if (is_modifier(sym)) {
    return; // Don't log modifiers
  }

  last_key_code = keycode;
  last_key_sym = sym;
  handle_key_press_action(keycode, sym);

  struct itimerspec its;
  its.it_value.tv_sec = repeat_delay / 1000;
  its.it_value.tv_nsec = (repeat_delay % 1000) * 1000000;
  int interval_ms = (repeat_rate > 0) ? (1000 / repeat_rate) : 100;
  its.it_interval.tv_sec = interval_ms / 1000;
  its.it_interval.tv_nsec = (interval_ms % 1000) * 1000000;
  timerfd_settime(repeat_timer_fd, 0, &its, NULL);
}