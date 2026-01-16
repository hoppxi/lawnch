#include "manager.hpp"
#include "../../helpers/fs.hpp"
#include "../../helpers/logger.hpp"
#include "../config/manager.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>
#include <vector>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

namespace Lawnch::Core::Icons {

Manager &Manager::Instance() {
  static Manager instance;
  return instance;
}

// init LRU with capacity of 8MB aggressively low
Manager::Manager() : icon_cache(8 * 1024 * 1024) {
  int num_workers = 1;
  for (int i = 0; i < num_workers; ++i) {
    workers.emplace_back(&Manager::worker_loop, this);
  }
}

Manager::~Manager() {
  stop_workers = true;
  queue_cv.notify_all();
  for (auto &worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void Manager::init() {
  auto &config = Config::Manager::Instance().Get();
  // absurd
  if (!config.general_icon_theme.empty()) {
    theme_loader.set_custom_theme(config.general_icon_theme);
  }

  if (theme_loader.init()) {
    Lawnch::Logger::log("IconManager", Lawnch::Logger::LogLevel::INFO,
                        "Initialized Icon Manager with theme: " +
                            (config.general_icon_theme.empty()
                                 ? "System"
                                 : config.general_icon_theme));
  } else {
    Lawnch::Logger::log("IconManager", Lawnch::Logger::LogLevel::ERROR,
                        "Failed to init theme loader");
  }
  initialized = true;
}

void Manager::ensure_initialized() {
  if (!initialized) {
    init();
  }
}

BLImage Manager::load_icon_image(const std::string &path, double size) {
  BLImage final_image;
  bool loaded = false;

  if (path.find(".png") != std::string::npos ||
      path.find(".jpg") != std::string::npos ||
      path.find(".jpeg") != std::string::npos ||
      path.find(".webp") != std::string::npos ||
      path.find(".bmp") != std::string::npos) {
    BLImage img;
    BLResult err = img.read_from_file(path.c_str());
    if (err == BL_SUCCESS) {
      if (img.width() != size || img.height() != size) {
        // resize
        int is = (int)size;
        BLImage resized(is, is, BL_FORMAT_PRGB32);
        BLContext ctx(resized);
        ctx.clear_all();

        double scale =
            std::min(size / (double)img.width(), size / (double)img.height());
        double w = img.width() * scale;
        double h = img.height() * scale;
        double x = (size - w) / 2.0;
        double y = (size - h) / 2.0;

        ctx.blit_image(BLRect(x, y, w, h), img);
        ctx.end();
        final_image = resized;
        loaded = true;
      } else {
        final_image = img;
        loaded = true;
      }
    } else {
      Lawnch::Logger::log("IconManager", Lawnch::Logger::LogLevel::ERROR,
                          "Blend2D failed to load: " + path);
    }
  }

  else if (path.find(".svg") != std::string::npos) {
    NSVGimage *image = nsvgParseFromFile(path.c_str(), "px", 96.0f);

    if (image) {
      NSVGrasterizer *rast = nsvgCreateRasterizer();
      if (rast) {
        int w = (int)size;
        int h = (int)size;

        float scaleX = (float)size / image->width;
        float scaleY = (float)size / image->height;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;

        float tx = std::round((size - (image->width * scale)) * 0.5f);
        float ty = std::round((size - (image->height * scale)) * 0.5f);

        int stride = w * 4;
        std::vector<unsigned char> temp_buffer(stride * h, 0);

        nsvgRasterize(rast, image, tx, ty, scale, temp_buffer.data(), w, h,
                      stride);
        nsvgDeleteRasterizer(rast);

        final_image = BLImage(w, h, BL_FORMAT_PRGB32);
        BLImageData imgData;
        final_image.make_mutable(&imgData);

        unsigned char *blData = (unsigned char *)imgData.pixel_data;
        int blStride = imgData.stride;
        const unsigned char *srcData = temp_buffer.data();

        for (int iy = 0; iy < h; ++iy) {
          const unsigned char *srcRow = srcData + iy * stride;
          unsigned char *dstRow = blData + iy * blStride;
          for (int ix = 0; ix < w; ++ix) {
            unsigned int r = srcRow[ix * 4 + 0];
            unsigned int g = srcRow[ix * 4 + 1];
            unsigned int b = srcRow[ix * 4 + 2];
            unsigned int a = srcRow[ix * 4 + 3];

            if (a > 0) {
              // pre-multiply alpha for Blend2D
              r = (r * a + 127) / 255;
              g = (g * a + 127) / 255;
              b = (b * a + 127) / 255;
              dstRow[ix * 4 + 0] = (unsigned char)b; // b
              dstRow[ix * 4 + 1] = (unsigned char)g; // g
              dstRow[ix * 4 + 2] = (unsigned char)r; // r
              dstRow[ix * 4 + 3] = (unsigned char)a; // a
            } else {
              *((uint32_t *)&dstRow[ix * 4]) = 0;
            }
          }
        }
        loaded = true;
      }
      nsvgDelete(image);
    }
  }

  if (loaded && !final_image.is_empty()) {
    return final_image;
  }

  return BLImage();
}

void Manager::render_icon(BLContext &ctx, const std::string &icon_name,
                          double x, double y, double size) {
  ensure_initialized();

  if (icon_name.empty())
    return;

  std::string cache_key = icon_name + "_" + std::to_string((int)size);

  {
    auto cached = icon_cache.get(cache_key);
    if (cached.has_value()) {
      ctx.blit_image(BLPoint(std::floor(x), std::floor(y)), cached.value());
      return;
    }

    std::lock_guard<std::mutex> lock(cache_mutex);
    if (pending_loads.count(cache_key)) {
      return;
    }
    pending_loads.insert(cache_key);
  }

  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    tasks.push({++task_counter, icon_name, cache_key, size});
  }
  queue_cv.notify_one();
}

void Manager::worker_loop() {
  while (true) {
    Task task;
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      queue_cv.wait(lock, [this] { return !tasks.empty() || stop_workers; });

      if (stop_workers && tasks.empty()) {
        return;
      }

      task = tasks.top();
      tasks.pop();
    }

    std::string icon_name = task.icon_name;
    std::string cache_key = task.cache_key;
    double size = task.size;

    std::string path;
    bool abs_handled = false;

    if (icon_name.front() == '/') {
      std::string ext = "";
      size_t dot_pos = icon_name.find_last_of('.');
      if (dot_pos != std::string::npos) {
        ext = icon_name.substr(dot_pos);
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return std::tolower(c); });
      }

      if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".svg" ||
          ext == ".bmp" || ext == ".webp") {
        if (std::filesystem::exists(icon_name)) {
          path = icon_name;
          abs_handled = true;
        }
      }
    }

    if (!abs_handled) {
      path = this->theme_loader.lookup_icon(icon_name);
    }

    BLImage img;
    if (!path.empty()) {
      img = this->load_icon_image(path, size);
    }

    {
      std::lock_guard<std::mutex> lock(cache_mutex);
      if (!img.is_empty()) {
        size_t cost = img.width() * img.height() * 4;
        icon_cache.put(cache_key, img, cost);
      }
      pending_loads.erase(cache_key);
    }

    if (this->on_update) {
      Lawnch::Logger::log("IconManager", Lawnch::Logger::LogLevel::DEBUG,
                          "Notifying update for " + cache_key);
      this->on_update();
    }
  }
}
} // namespace Lawnch::Core::Icons
