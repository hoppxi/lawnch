#include "application.hpp"
#include "../helpers/fs.hpp"
#include "../helpers/locale.hpp"
#include "../helpers/logger.hpp"
#include "../helpers/process.hpp"
#include <filesystem>
#include <iostream>
#include <poll.h>
#include <unistd.h>

namespace Lawnch::App {

Application::Application(std::unique_ptr<IPC::Server> server,
                         std::optional<std::string> config_path_override)
    : ipc_server(std::move(server)),
      config_manager(Core::Config::Manager::Instance()),
      icon_manager(Core::Icons::Manager::Instance()) {

  std::filesystem::path log_path = Fs::get_log_path("lawnch");
  Logger::init(log_path.string());

  Logger::log("App", Logger::LogLevel::INFO, "Initializing Application...");

  ipc_server->set_on_kill([this]() { this->stop(); });
  try {
    ipc_server->init();
  } catch (const std::exception &e) {
    // Single-instance IPC failed? Just log the error and continue; previously
    // considered exiting.
    Logger::log("App", Logger::LogLevel::ERROR,
                std::string("IPC Init Failed: ") + e.what());
  }

  std::string config_path;
  if (config_path_override.has_value()) {
    config_path = config_path_override.value();
  } else {
    std::filesystem::path config_dir = Fs::get_config_home() / "lawnch";
    if (!std::filesystem::exists(config_dir)) {
      std::filesystem::create_directories(config_dir);
    }
    config_path = (config_dir / "config.ini").string();
  }

  if (std::filesystem::exists(config_path)) {
    config_manager.Load(config_path);
  } else {
    Logger::log("App", Logger::LogLevel::WARNING,
                "Config file not found: " + config_path);
  }

  plugin_manager =
      std::make_unique<Core::Search::Plugins::Manager>(config_manager.Get());

  // Apply general config settings
  Locale::set_override(config_manager.Get().general_locale);
  history.set_max_size(config_manager.Get().general_history_max_size);

  search_engine = std::make_unique<Core::Search::Engine>(*plugin_manager);
  search_engine->set_async_callback(
      [this](auto res) { this->on_search_results(res); });

  Core::Window::Input::KeyboardCallbacks kb_cb;
  kb_cb.on_update = [this]() { this->on_keyboard_update(); };
  kb_cb.on_execute = [this](std::string s) { this->on_keyboard_execute(s); };
  kb_cb.on_stop = [this]() { this->on_keyboard_stop(); };
  kb_cb.on_render = [this]() { this->on_keyboard_render(); };

  keyboard = std::make_unique<Core::Window::Input::Keyboard>(history, kb_cb);

  Core::Window::Input::PointerCallbacks ptr_cb;
  ptr_cb.on_scroll = [this](double d) { this->on_pointer_scroll(d); };
  pointer = std::make_unique<Core::Window::Input::Pointer>(ptr_cb);

  display = std::make_unique<Core::Window::Wayland::Display>();
  registry = std::make_unique<Core::Window::Wayland::Registry>(*display);

  if (!registry->has_required_globals()) {
    Logger::log("App", Logger::LogLevel::ERROR,
                "Missing required Wayland globals");
    throw std::runtime_error("Missing Wayland Globals");
  }

  seat = std::make_unique<Core::Window::Wayland::Seat>(registry->seat,
                                                       *keyboard, *pointer);
  layer_surface = std::make_unique<Core::Window::Wayland::LayerSurface>(
      registry->compositor, registry->layer_shell, registry->output);

  layer_surface->apply_config(config_manager.Get());

  layer_surface->on_configure = [this](int w, int h) { this->resize(w, h); };
  layer_surface->on_closed = [this]() { this->stop(); };

  Logger::log("App", Logger::LogLevel::INFO, "Initialization Complete");
}

Application::~Application() {
  // cleanup
}

void Application::run() {
  running = true;
  struct pollfd fds[3];

  fds[0] = {.fd = display->get_fd(), .events = POLLIN, .revents = 0};
  fds[1] = {.fd = seat->get_repeat_timer_fd(), .events = POLLIN, .revents = 0};
  fds[2] = {.fd = ipc_server->get_fd(), .events = POLLIN, .revents = 0};

  while (running && display->dispatch() != -1) {
    display->flush();
    if (poll(fds, 3, -1) < 0)
      break;

    if (fds[0].revents & POLLIN) {
      if (display->dispatch() == -1)
        break;
    }

    if (fds[1].revents & POLLIN) {
      seat->handle_repeat();
    }

    if (fds[2].revents & POLLIN) {
      ipc_server->process_messages();
    }
  }
}

void Application::stop() {
  keyboard->clear();
  running = false;
}

void Application::resize(int width, int height) {
  buffer.create(registry->shm, width, height);
  render_frame();
}

void Application::render_frame() {
  if (!layer_surface->is_configured() || !buffer.is_valid())
    return;

  Core::Window::Render::RenderState state;
  state.search_text = keyboard->get_text();
  state.caret_position = keyboard->get_caret();
  state.input_selected = keyboard->is_input_selected();
  state.results = current_results;
  state.selected_index = keyboard->get_selected_index();
  state.scroll_offset = scroll_offset;

  renderer.render(buffer.get_context(), buffer.get_width(), buffer.get_height(),
                  config_manager.Get(), state);

  layer_surface->commit(buffer.get_wl_buffer());
}

void Application::on_keyboard_update() {
  std::string text = keyboard->get_text();
  if (text.empty()) {
    current_results.clear();
    scroll_offset = 0;
    keyboard->set_result_count(0);
    render_frame();
    return;
  }

  current_results = search_engine->query(text);
  on_search_results(current_results);
}

void Application::on_search_results(
    const std::vector<Core::Search::SearchResult> &results) {
  current_results = results;
  scroll_offset = 0;
  keyboard->set_result_count(results.size());

  for (size_t i = 0; i < results.size(); ++i) {
    keyboard->set_results_command(i, results[i].command);
  }

  render_frame();
}

void Application::on_keyboard_execute(std::string cmd) {
  if (!cmd.empty()) {
    const auto &cfg = config_manager.Get();
    std::string final_cmd = cmd;
    if (!cfg.launch_prefix.empty()) {
      final_cmd = cfg.launch_prefix + " " + cmd;
    }
    Proc::exec_detached(final_cmd);
    stop();
  }
}

void Application::on_keyboard_stop() { stop(); }

void Application::on_keyboard_render() {
  int sel = keyboard->get_selected_index();

  int visible_count = renderer.get_visible_count(layer_surface->get_height(),
                                                 config_manager.Get());

  if (sel < scroll_offset) {
    scroll_offset = sel;
  } else if (sel >= scroll_offset + visible_count) {
    scroll_offset = sel - visible_count + 1;
  }

  render_frame();
}

void Application::on_pointer_scroll(double delta) {
  const auto &cfg = config_manager.Get();
  int visible_count =
      renderer.get_visible_count(layer_surface->get_height(), cfg);
  int total = (int)current_results.size();

  if (cfg.results_scroll_mode == "fixed") {
    int sel = keyboard->get_selected_index();

    if (delta > 0) { // scroll down
      if (sel < total - 1) {
        keyboard->set_selected_index(sel + 1);
        if (sel + 1 >= scroll_offset + visible_count) {
          scroll_offset++;
        }
      }
    } else { // scroll up
      if (sel > 0) {
        keyboard->set_selected_index(sel - 1);
        if (sel - 1 < scroll_offset) {
          scroll_offset--;
        }
      }
    }
  } else {
    if (delta > 0) { // scroll down
      if (scroll_offset + visible_count < total)
        scroll_offset++;
    } else {
      if (scroll_offset > 0)
        scroll_offset--;
    }
  }
  render_frame();
}

} // namespace Lawnch::App
