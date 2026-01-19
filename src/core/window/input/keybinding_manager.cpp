#include "keybinding_manager.hpp"
#include "../../../helpers/string.hpp"
#include "../../config/manager.hpp"
#include <algorithm>
#include <iostream>

namespace Lawnch::Core::Window::Input {

KeybindingManager &KeybindingManager::Instance() {
  static KeybindingManager instance;
  return instance;
}

KeybindingManager::KeybindingManager() { load_defaults(); }

Action KeybindingManager::get_action(xkb_keysym_t key,
                                     uint32_t modifiers) const {
  auto it = bindings.find({key, modifiers});
  if (it != bindings.end()) {
    return it->second;
  }

  // handles cases where user configures "Shift+c" (stored as 'c')
  // but runtime delivers 'C' due to Shift.
  xkb_keysym_t lower = xkb_keysym_to_lower(key);
  if (lower != key) {
    it = bindings.find({lower, modifiers});
    if (it != bindings.end()) {
      return it->second;
    }
  }

  return Action::NONE;
}

void KeybindingManager::set_preset(Preset p) {
  current_preset = p;
  bindings.clear();
  if (p == Preset::VIM) {
    load_vim();
  } else {
    load_defaults();
  }
  load_config();
}

std::string KeybindingManager::action_to_string(Action action) const {
  switch (action) {
  case Action::NAV_UP:
    return "nav_up";
  case Action::NAV_DOWN:
    return "nav_down";
  case Action::NAV_LEFT:
    return "nav_left";
  case Action::NAV_RIGHT:
    return "nav_right";
  case Action::NAV_HOME:
    return "nav_home";
  case Action::NAV_END:
    return "nav_end";
  case Action::EXECUTE:
    return "execute";
  case Action::EXIT:
    return "exit";
  case Action::COPY:
    return "copy";
  case Action::PASTE:
    return "paste";
  case Action::CUT:
    return "cut";
  case Action::CLEAR_INPUT:
    return "clear_input";
  case Action::UNDO:
    return "undo";
  case Action::DELETE_CHAR_BACK:
    return "delete_char_back";
  case Action::DELETE_CHAR_FWD:
    return "delete_char_fwd";
  case Action::DELETE_WORD_BACK:
    return "delete_word_back";
  case Action::SELECT_INPUT:
    return "select_input";
  case Action::QUERY_HISTORY_UP:
    return "query_history_up";
  case Action::QUERY_HISTORY_DOWN:
    return "query_history_down";
  case Action::NAV_WORD_BACK:
    return "nav_word_back";
  case Action::NAV_WORD_FWD:
    return "nav_word_fwd";
  case Action::DELETE_WORD_FWD:
    return "delete_word_fwd";
  default:
    return "none";
  }
}

Action KeybindingManager::string_to_action(const std::string &str) const {
  if (str == "nav_up")
    return Action::NAV_UP;
  if (str == "nav_down")
    return Action::NAV_DOWN;
  if (str == "nav_left")
    return Action::NAV_LEFT;
  if (str == "nav_right")
    return Action::NAV_RIGHT;
  if (str == "nav_home")
    return Action::NAV_HOME;
  if (str == "nav_end")
    return Action::NAV_END;
  if (str == "execute")
    return Action::EXECUTE;
  if (str == "exit")
    return Action::EXIT;
  if (str == "copy")
    return Action::COPY;
  if (str == "paste")
    return Action::PASTE;
  if (str == "cut")
    return Action::CUT;
  if (str == "clear_input")
    return Action::CLEAR_INPUT;
  if (str == "undo")
    return Action::UNDO;
  if (str == "delete_char_back")
    return Action::DELETE_CHAR_BACK;
  if (str == "delete_char_fwd")
    return Action::DELETE_CHAR_FWD;
  if (str == "delete_word_back")
    return Action::DELETE_WORD_BACK;
  if (str == "select_input")
    return Action::SELECT_INPUT;
  if (str == "query_history_up")
    return Action::QUERY_HISTORY_UP;
  if (str == "query_history_down")
    return Action::QUERY_HISTORY_DOWN;
  if (str == "nav_word_back")
    return Action::NAV_WORD_BACK;
  if (str == "nav_word_fwd")
    return Action::NAV_WORD_FWD;
  if (str == "delete_word_fwd")
    return Action::DELETE_WORD_FWD;
  return Action::NONE;
}

void KeybindingManager::load_defaults() {
  bindings[{XKB_KEY_Up, MOD_NONE}] = Action::NAV_UP;
  bindings[{XKB_KEY_Down, MOD_NONE}] = Action::NAV_DOWN;
  bindings[{XKB_KEY_Left, MOD_NONE}] = Action::NAV_LEFT;
  bindings[{XKB_KEY_Right, MOD_NONE}] = Action::NAV_RIGHT;

  bindings[{XKB_KEY_Left, MOD_CTRL}] = Action::NAV_WORD_BACK;
  bindings[{XKB_KEY_Right, MOD_CTRL}] = Action::NAV_WORD_FWD;

  bindings[{XKB_KEY_Up, MOD_ALT}] = Action::QUERY_HISTORY_UP;
  bindings[{XKB_KEY_Down, MOD_ALT}] = Action::QUERY_HISTORY_DOWN;

  bindings[{XKB_KEY_Home, MOD_NONE}] = Action::NAV_HOME;
  bindings[{XKB_KEY_End, MOD_NONE}] = Action::NAV_END;

  bindings[{XKB_KEY_Return, MOD_NONE}] = Action::EXECUTE;
  bindings[{XKB_KEY_KP_Enter, MOD_NONE}] = Action::EXECUTE;

  bindings[{XKB_KEY_Escape, MOD_NONE}] = Action::EXIT;

  bindings[{XKB_KEY_c, MOD_CTRL}] = Action::COPY;
  bindings[{XKB_KEY_C, MOD_CTRL}] = Action::COPY;
  bindings[{XKB_KEY_v, MOD_CTRL}] = Action::PASTE;
  bindings[{XKB_KEY_V, MOD_CTRL}] = Action::PASTE;
  bindings[{XKB_KEY_x, MOD_CTRL}] = Action::CUT;
  bindings[{XKB_KEY_X, MOD_CTRL}] = Action::CUT;
  bindings[{XKB_KEY_a, MOD_CTRL}] = Action::SELECT_INPUT;
  bindings[{XKB_KEY_A, MOD_CTRL}] = Action::SELECT_INPUT;
  bindings[{XKB_KEY_u, MOD_CTRL}] = Action::CLEAR_INPUT;
  bindings[{XKB_KEY_U, MOD_CTRL}] = Action::CLEAR_INPUT;
  bindings[{XKB_KEY_z, MOD_CTRL}] = Action::UNDO;
  bindings[{XKB_KEY_Z, MOD_CTRL}] = Action::UNDO;

  bindings[{XKB_KEY_BackSpace, MOD_NONE}] = Action::DELETE_CHAR_BACK;
  bindings[{XKB_KEY_Delete, MOD_NONE}] = Action::DELETE_CHAR_FWD;
  bindings[{XKB_KEY_BackSpace, MOD_CTRL}] = Action::DELETE_WORD_BACK;
  bindings[{XKB_KEY_Delete, MOD_CTRL}] = Action::DELETE_WORD_FWD;
  bindings[{XKB_KEY_w, MOD_CTRL}] = Action::DELETE_WORD_BACK; // Ctrl+W
}

void KeybindingManager::load_vim() {
  load_defaults();

  // vim like binding
  bindings[{XKB_KEY_k, MOD_CTRL}] = Action::NAV_UP;
  bindings[{XKB_KEY_j, MOD_CTRL}] = Action::NAV_DOWN;
  bindings[{XKB_KEY_h, MOD_CTRL}] = Action::NAV_LEFT;
  bindings[{XKB_KEY_l, MOD_CTRL}] = Action::NAV_RIGHT;

  bindings[{XKB_KEY_b, MOD_CTRL}] = Action::NAV_WORD_BACK;
  bindings[{XKB_KEY_w, MOD_CTRL}] = Action::NAV_WORD_FWD;

  // Ctrl+N/P for history/nav
  bindings[{XKB_KEY_p, MOD_CTRL}] = Action::NAV_UP;
  bindings[{XKB_KEY_n, MOD_CTRL}] = Action::NAV_DOWN;

  // using ctrl to avoid typing conflict
}

void KeybindingManager::load_config() {
  auto &config = Config::Manager::Instance().Get();

  Preset p = Preset::DEFAULT;
  std::string preset_str = config.bindings_preset;
  if (preset_str == "vim") {
    p = Preset::VIM;
  }

  if (current_preset != p) {
    current_preset = p;
    bindings.clear();
    if (current_preset == Preset::VIM)
      load_vim();
    else
      load_defaults();
  } else {
    bindings.clear();
    if (current_preset == Preset::VIM)
      load_vim();
    else
      load_defaults();
  }

  for (const auto &[action_name, key_str] : config.keybindings) {
    Action action = string_to_action(action_name);
    if (action == Action::NONE) {
      std::cerr << "Warning: Unknown action in config: " << action_name
                << std::endl;
      continue;
    }

    uint32_t mods = MOD_NONE;
    xkb_keysym_t key = XKB_KEY_NoSymbol;

    std::string s = key_str;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find('+')) != std::string::npos) {
      token = Lawnch::Str::trim(s.substr(0, pos));
      if (token == "Ctrl" || token == "Control")
        mods |= MOD_CTRL;
      else if (token == "Alt")
        mods |= MOD_ALT;
      else if (token == "Shift")
        mods |= MOD_SHIFT;
      else if (token == "Super" || token == "Mod4")
        mods |= MOD_SUPER;
      s.erase(0, pos + 1);
    }
    token = Lawnch::Str::trim(s);
    key = xkb_keysym_from_name(token.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);

    if (key == XKB_KEY_NoSymbol) {
      std::cerr << "Warning: Invalid key in config: " << token << std::endl;
      continue;
    }

    // remove existing bindings for this action to override default behavior
    for (auto it = bindings.begin(); it != bindings.end();) {
      if (it->second == action) {
        it = bindings.erase(it);
      } else {
        ++it;
      }
    }

    bindings[{key, mods}] = action;
  }
}

} // namespace Lawnch::Core::Window::Input
