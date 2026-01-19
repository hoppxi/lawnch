#pragma once

#include <atomic>
#include <blend2d.h>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

namespace Lawnch::ImageCache {

class ImageCache {
public:
  static ImageCache &Instance();
  std::filesystem::path get_image(const std::string &path, int w, int h);
  void set_render_callback(std::function<void()> cb);

private:
  ImageCache();
  ~ImageCache();
  ImageCache(const ImageCache &) = delete;
  ImageCache &operator=(const ImageCache &) = delete;

  void worker_loop();
  void process_image(const std::string &path, int w, int h);

  std::string get_cache_key(const std::string &path, int w, int h);
  std::filesystem::path get_disk_cache_path(const std::string &key);

  std::filesystem::path cache_dir;
  std::function<void()> render_callback;

  std::thread worker;
  std::atomic<bool> running{true};
  std::mutex queue_mutex;
  std::condition_variable queue_cv;

  struct Job {
    std::string path;
    int w;
    int h;
  };

  std::deque<Job> job_queue;
  std::unordered_set<std::string> pending_keys;
};

} // namespace Lawnch::ImageCache
