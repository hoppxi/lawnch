#include "image_cache.hpp"
#include "fs.hpp"
#include "string.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>

namespace Lawnch::ImageCache {

ImageCache &ImageCache::Instance() {
  static ImageCache inst;
  return inst;
}

ImageCache::ImageCache() {
  cache_dir = Lawnch::Fs::get_cache_home() / "lawnch" / "previews";
  if (!std::filesystem::exists(cache_dir)) {
    std::filesystem::create_directories(cache_dir);
  }

  worker = std::thread(&ImageCache::worker_loop, this);
}

ImageCache::~ImageCache() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    running = false;
  }
  queue_cv.notify_all();

  if (worker.joinable()) {
    worker.join();
  }
}

void ImageCache::set_render_callback(std::function<void()> cb) {
  std::lock_guard<std::mutex> lock(queue_mutex);
  render_callback = std::move(cb);
}

std::string ImageCache::get_cache_key(const std::string &path, int w, int h) {
  return std::to_string(Lawnch::Str::hash(path)) + "_" + std::to_string(w) +
         "x" + std::to_string(h) + ".png";
}

std::filesystem::path ImageCache::get_disk_cache_path(const std::string &key) {
  return cache_dir / key;
}

std::filesystem::path ImageCache::get_image(const std::string &path, int w,
                                            int h) {
  if (path.empty())
    return {};

  std::string key = get_cache_key(path, w, h);
  std::filesystem::path disk_path = get_disk_cache_path(key);

  if (std::filesystem::exists(disk_path)) {
    return disk_path;
  }

  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (pending_keys.find(key) == pending_keys.end()) {
      pending_keys.insert(key);
      job_queue.push_back({path, w, h});
      queue_cv.notify_one();
    }
  }

  return {};
}

void ImageCache::worker_loop() {
  while (running) {
    Job current_job;

    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      queue_cv.wait(lock, [this] { return !job_queue.empty() || !running; });

      if (!running)
        return;

      current_job = job_queue.front();
      job_queue.pop_front();
    }

    process_image(current_job.path, current_job.w, current_job.h);

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      std::string key =
          get_cache_key(current_job.path, current_job.w, current_job.h);
      pending_keys.erase(key);
      if (render_callback)
        render_callback();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void ImageCache::process_image(const std::string &path, int w, int h) {
  std::string key = get_cache_key(path, w, h);
  std::filesystem::path disk_path = get_disk_cache_path(key);

  if (std::filesystem::exists(disk_path))
    return;

  BLImage original;
  if (original.read_from_file(path.c_str()) != BL_SUCCESS) {
    return;
  }

  int img_w = original.width();
  int img_h = original.height();
  double scale =
      std::min(static_cast<double>(w) / img_w, static_cast<double>(h) / img_h);

  int target_w = static_cast<int>(img_w * scale);
  int target_h = static_cast<int>(img_h * scale);

  if (target_w <= 0 || target_h <= 0) {
    original.reset();
    return;
  }

  BLImage current = std::move(original);
  int cur_w = img_w;
  int cur_h = img_h;

  while (cur_w > target_w * 2 && cur_h > target_h * 2) {
    int next_w = cur_w / 2;
    int next_h = cur_h / 2;

    BLImage next(next_w, next_h, BL_FORMAT_PRGB32);
    BLContext ctx(next);
    ctx.set_comp_op(BL_COMP_OP_SRC_COPY);
    ctx.blit_image(BLRect(0, 0, next_w, next_h), current,
                   BLRectI(0, 0, cur_w, cur_h));
    ctx.end();

    current.reset();
    current = std::move(next);
    cur_w = next_w;
    cur_h = next_h;
  }

  BLImage result(target_w, target_h, BL_FORMAT_PRGB32);
  BLContext final_ctx(result);
  final_ctx.set_comp_op(BL_COMP_OP_SRC_COPY);
  final_ctx.blit_image(BLRect(0, 0, target_w, target_h), current,
                       BLRectI(0, 0, cur_w, cur_h));
  final_ctx.end();
  current.reset();

  result.write_to_file(disk_path.string().c_str());
  result.reset();
}

} // namespace Lawnch::ImageCache
