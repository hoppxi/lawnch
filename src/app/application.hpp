#pragma once

#include "../ipc/server.hpp"
#include <memory>
#include <optional>
#include <vector>

#include "../core/config/manager.hpp"
#include "../core/icons/manager.hpp"
#include "../core/search/engine.hpp"
#include "../core/search/plugins/manager.hpp"
#include "../core/window/input/history.hpp"
#include "../core/window/input/keyboard.hpp"
#include "../core/window/input/pointer.hpp"
#include "../core/window/render/buffer.hpp"
#include "../core/window/render/renderer.hpp"
#include "../core/window/wayland/display.hpp"
#include "../core/window/wayland/layer_surface.hpp"
#include "../core/window/wayland/registry.hpp"
#include "../core/window/wayland/seat.hpp"

namespace Lawnch::App {

class Application {
public:
  Application(std::unique_ptr<IPC::Server> server,
              std::optional<std::string> config_path);
  ~Application();

  void run();
  void stop();

private:
  bool running = false;

  std::unique_ptr<IPC::Server> ipc_server;

  Core::Config::Manager &config_manager;
  Core::Icons::Manager &icon_manager;

  std::unique_ptr<Core::Search::Plugins::Manager> plugin_manager;
  std::unique_ptr<Core::Search::Engine> search_engine;

  std::unique_ptr<Core::Window::Wayland::Display> display;
  std::unique_ptr<Core::Window::Wayland::Registry> registry;
  std::unique_ptr<Core::Window::Wayland::Seat> seat;
  std::unique_ptr<Core::Window::Wayland::LayerSurface> layer_surface;

  Core::Window::Input::History history;
  std::unique_ptr<Core::Window::Input::Keyboard> keyboard;
  std::unique_ptr<Core::Window::Input::Pointer> pointer;

  Core::Window::Render::Buffer buffer;
  Core::Window::Render::Renderer renderer;

  std::vector<Core::Search::SearchResult> current_results;
  int scroll_offset = 0;

  void on_keyboard_update();
  void on_keyboard_execute(std::string cmd);
  void on_keyboard_stop();
  void on_keyboard_render();

  void on_pointer_scroll(double delta);

  void
  on_search_results(const std::vector<Core::Search::SearchResult> &results);

  void resize(int width, int height);
  void render_frame();
};

} // namespace Lawnch::App
