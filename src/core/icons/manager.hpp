#pragma once

#include "lru_cache.hpp"
#include "theme_loader.hpp"
#include <blend2d.h>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

struct NSVGimage;

namespace Lawnch::Core::Icons {

class Manager {
public:
  static Manager &Instance();

  void render_icon(BLContext &ctx, const std::string &icon_name, double x,
                   double y, double size);

  void set_update_callback(std::function<void()> cb) { on_update = cb; }

private:
  Manager();
  ~Manager();

  void init();
  void ensure_initialized();

  BLImage load_icon_image(const std::string &path, double size);

  bool initialized = false;
  ThemeLoader theme_loader;

  LRUCache<std::string, BLImage> icon_cache;

  std::mutex cache_mutex;
  std::set<std::string> pending_loads;
  std::function<void()> on_update;

  struct Task {
    uint64_t id;
    std::string icon_name;
    std::string cache_key;
    double size;

    bool operator<(const Task &other) const { return id < other.id; }
  };

  std::priority_queue<Task> tasks;
  std::mutex queue_mutex;
  std::condition_variable queue_cv;
  std::vector<std::thread> workers;
  std::atomic<bool> stop_workers{false};
  std::atomic<uint64_t> task_counter{0};

  void worker_loop();
};

} // namespace Lawnch::Core::Icons
