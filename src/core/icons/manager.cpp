#include "manager.hpp"
#include "../../helpers/logger.hpp"
#include "../config/manager.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
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

Manager::Manager() {}

Manager::~Manager() {
  for (auto &pair : cache) {
    if (pair.second)
      nsvgDelete(pair.second);
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
        return resized;
      }
      return img;
    }
    Lawnch::Logger::log("IconManager", Lawnch::Logger::LogLevel::ERROR,
                        "Blend2D failed to load: " + path);
    return BLImage();
  }

  if (path.find(".svg") != std::string::npos) {
    NSVGimage *image = nullptr;

    {
      std::lock_guard<std::mutex> lock(cache_mutex);
      if (cache.count(path)) {
        image = cache[path];
      } else {
        image = nsvgParseFromFile(path.c_str(), "px", 96.0f);
        if (image)
          cache[path] = image;
      }
    }

    if (!image)
      return BLImage();

    NSVGrasterizer *rast = nsvgCreateRasterizer();
    if (rast == NULL)
      return BLImage();

    int w = (int)size;
    int h = (int)size;

    float scaleX = (float)size / image->width;
    float scaleY = (float)size / image->height;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    float tx = std::round((size - (image->width * scale)) * 0.5f);
    float ty = std::round((size - (image->height * scale)) * 0.5f);

    int stride = w * 4;
    std::vector<unsigned char> temp_buffer(stride * h, 0);

    nsvgRasterize(rast, image, tx, ty, scale, temp_buffer.data(), w, h, stride);
    nsvgDeleteRasterizer(rast);

    BLImage img(w, h, BL_FORMAT_PRGB32);
    BLImageData imgData;
    img.make_mutable(&imgData);

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

    return img;
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
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (icon_cache.count(cache_key)) {
      ctx.blit_image(BLPoint(std::floor(x), std::floor(y)),
                     icon_cache[cache_key]);
      return;
    }

    if (pending_loads.count(cache_key)) {
      return;
    }
    pending_loads.insert(cache_key);
  }

  std::thread([this, icon_name, cache_key, size]() {
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
        icon_cache[cache_key] = img;
      }
      pending_loads.erase(cache_key);
    }
  }).detach();
}

} // namespace Lawnch::Core::Icons
