#include "application.hpp"
#include "../helpers/fs.hpp"
#include "../helpers/locale.hpp"
#include "../helpers/logger.hpp"
#include "../helpers/process.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <poll.h>
#include <sys/eventfd.h>
#include <thread>
#include <unistd.h>

namespace Lawnch::App {

Application::Application(std::unique_ptr<IPC::Server> server,
                         std::optional<std::string> config_path_override,
                         std::optional<std::string> merge_config_path,
                         int log_verbosity)
    : ipc_server(std::move(server)),
      config_manager(Core::Config::Manager::Instance()),
      icon_manager(Core::Icons::Manager::Instance()),
      image_cache(ImageCache::ImageCache::Instance()),
      last_render_time(std::chrono::steady_clock::now()) {

  std::filesystem::path log_path = Fs::get_log_path("lawnch");
  Logger::init(log_path.string(),
               static_cast<Logger::LogVerbosity>(log_verbosity));

  Logger::log("App", Logger::LogLevel::INFO, "Initializing Application...");
  Logger::log("Logger", Logger::LogLevel::INFO,
              "verbosity level " + std::to_string(log_verbosity));

  wakeup_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  image_cache.set_render_callback([this]() {
    uint64_t u = 1;
    if (write(this->wakeup_fd, &u, sizeof(u)) == -1) {
      Logger::log("App", Logger::LogLevel::ERROR,
                  "Failed to write to wakeup_fd to schedule render");
    }
  });

  ipc_server->set_on_kill([this]() { this->stop(); });
  try {
    ipc_server->init();
  } catch (const std::exception &e) {
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

  if (merge_config_path.has_value()) {
    config_manager.Merge(merge_config_path.value());
  }

  plugin_manager =
      std::make_unique<Core::Search::Plugins::Manager>(config_manager.Get());

  Locale::set_override(config_manager.Get().general_locale);
  history.set_max_size(config_manager.Get().general_history_max_size);

  search_engine = std::make_unique<Core::Search::Engine>(*plugin_manager);
  if (!config_manager.Get().launch_start_with.empty()) {
    search_engine->set_forced_mode(config_manager.Get().launch_start_with);
  }
  search_engine->set_async_callback(
      [this](auto res) { this->on_search_results(res); });

  Core::Window::Input::KeyboardCallbacks kb_cb;
  kb_cb.on_update = [this]() { this->on_keyboard_update(); };
  kb_cb.on_execute = [this](std::string s) { this->on_keyboard_execute(s); };
  kb_cb.on_stop = [this]() { this->on_keyboard_stop(); };
  kb_cb.on_render = [this]() { this->on_keyboard_render(); };

  keyboard = std::make_unique<Core::Window::Input::Keyboard>(history, kb_cb);
  const auto &cfg = Core::Config::Manager::Instance().Get();
  if (cfg.results_reverse) {
    keyboard->set_reverse_navigation(true);
  }

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

  if (!config_manager.Get().launch_start_with.empty()) {
    on_search_results(search_engine->query(""));
  }

  Logger::log("App", Logger::LogLevel::INFO, "Initialization Complete");
}

Application::~Application() {
  if (wakeup_fd != -1) {
    close(wakeup_fd);
  }
}

void Application::run() {
  running = true;
  while (running && display->dispatch() != -1) {
    struct pollfd fds[4];
    fds[0] = {.fd = display->get_fd(), .events = POLLIN, .revents = 0};
    fds[1] = {
        .fd = seat->get_repeat_timer_fd(), .events = POLLIN, .revents = 0};
    fds[2] = {.fd = ipc_server->get_fd(), .events = POLLIN, .revents = 0};
    fds[3] = {.fd = wakeup_fd, .events = POLLIN, .revents = 0};

    display->flush();
    if (poll(fds, 4, -1) < 0)
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

    if (fds[3].revents & POLLIN) {
      uint64_t u;
      if (read(wakeup_fd, &u, sizeof(u)) > 0) {
        Logger::log("App", Logger::LogLevel::DEBUG,
                    "Wakeup received, rendering frame.");
        render_frame();
      }
    }
  }
}

void Application::stop() {
  keyboard->clear();
  running = false;
}

void Application::resize(int width, int height) {
  buffer.create(registry->shm, width, height);
  if (layer_surface->is_configured() && buffer.is_valid()) {
    std::lock_guard<std::mutex> lock(render_mutex);
    render_pending = false;
    last_render_time = std::chrono::steady_clock::now() - min_frame_time_ms;
    render_frame_impl();
  }
}

void Application::render_frame() {
  if (!layer_surface->is_configured() || !buffer.is_valid())
    return;

  auto now = std::chrono::steady_clock::now();
  auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - last_render_time);

  if (time_since_last >= min_frame_time_ms) {
    std::lock_guard<std::mutex> lock(render_mutex);
    render_pending = false;
    render_frame_impl();
    return;
  }

  if (!render_pending) {
    render_pending = true;
    auto delay = min_frame_time_ms - time_since_last;
    std::thread([this, delay]() {
      std::this_thread::sleep_for(delay);
      std::lock_guard<std::mutex> lock(render_mutex);
      if (render_pending) {
        render_pending = false;
        if (layer_surface->is_configured() && buffer.is_valid()) {
          render_frame_impl();
        }
      }
    }).detach();
  }
}

void Application::render_frame_impl() {
  if (!layer_surface->is_configured() || !buffer.is_valid()) {
    return;
  }

  if (buffer.get_width() <= 0 || buffer.get_height() <= 0) {
    return;
  }

  last_render_time = std::chrono::steady_clock::now();

  Core::Window::Render::RenderState state;
  state.search_text = keyboard->get_text();
  state.caret_position = keyboard->get_caret();
  state.input_selected = keyboard->is_input_selected();
  state.results = current_results;
  state.selected_index = keyboard->get_selected_index();
  state.scroll_offset = scroll_offset;

  renderer.render(buffer.get_context(), buffer.get_width(), buffer.get_height(),
                  config_manager.Get(), state);

  if (buffer.is_valid() && layer_surface->is_configured()) {
    layer_surface->commit(buffer.get_wl_buffer());
  }
}

void Application::on_keyboard_update() {
  std::string text = keyboard->get_text();
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
    search_engine->record_usage(cmd);
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
