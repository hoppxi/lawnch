#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <xkbcommon/xkbcommon.h>

namespace Lawnch::Core::Window::Input {

constexpr uint32_t MOD_NONE = 0;
constexpr uint32_t MOD_CTRL = 1 << 0;
constexpr uint32_t MOD_ALT = 1 << 1;
constexpr uint32_t MOD_SHIFT = 1 << 2;
constexpr uint32_t MOD_SUPER = 1 << 3;

enum class Action {
  NONE,
  NAV_UP,
  NAV_DOWN,
  NAV_LEFT,
  NAV_RIGHT,
  NAV_HOME,
  NAV_END,
  EXECUTE,
  EXIT,
  COPY,
  PASTE,
  CUT,
  CLEAR_INPUT,
  UNDO,
  DELETE_CHAR_BACK,
  DELETE_CHAR_FWD,
  DELETE_WORD_BACK,
  SELECT_INPUT,
  QUERY_HISTORY_UP,
  QUERY_HISTORY_DOWN,
  NAV_WORD_BACK,
  NAV_WORD_FWD,
  DELETE_WORD_FWD
};

enum class Preset { DEFAULT, VIM };

struct KeyHash {
  std::size_t operator()(const std::pair<xkb_keysym_t, uint32_t> &k) const {
    return std::hash<xkb_keysym_t>()(k.first) ^
           (std::hash<uint32_t>()(k.second) << 1);
  }
};

class KeybindingManager {
public:
  static KeybindingManager &Instance();

  void load_config();
  void set_preset(Preset p);

  [[nodiscard]] Action get_action(xkb_keysym_t key, uint32_t modifiers) const;
  [[nodiscard]] std::string action_to_string(Action action) const;
  [[nodiscard]] Action string_to_action(const std::string &str) const;

private:
  KeybindingManager();

  void load_defaults();
  void load_vim();

  std::unordered_map<std::pair<xkb_keysym_t, uint32_t>, Action, KeyHash>
      bindings;
  Preset current_preset = Preset::DEFAULT;
};

} // namespace Lawnch::Core::Window::Input
