#include "manager.hpp"
#include "../../helpers/logger.hpp"
#include "../config/manager.hpp"
#include <algorithm>
#include <cmath>
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

bool Manager::load_icon_image(const std::string &path,
                              const std::string &cache_key, double size) {

  if (path.find(".png") != std::string::npos ||
      path.find(".jpg") != std::string::npos ||
      path.find(".jpeg") != std::string::npos) {
    BLImage img;
    BLResult err = img.read_from_file(path.c_str());
    if (err == BL_SUCCESS) {
      icon_cache[cache_key] = img;
      return true;
    }
    Lawnch::Logger::log("IconManager", Lawnch::Logger::LogLevel::ERROR,
                        "Blend2D failed to load: " + path);
    return false;
  }

  if (path.find(".svg") != std::string::npos) {
    NSVGimage *image = nullptr;

    if (cache.find(path) != cache.end()) {
      image = cache[path];
    } else {
      image = nsvgParseFromFile(path.c_str(), "px", 96.0f);
      if (image)
        cache[path] = image;
    }

    if (!image)
      return false;

    NSVGrasterizer *rast = nsvgCreateRasterizer();
    if (rast == NULL)
      return false;

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

    icon_cache[cache_key] = img;
    return true;
  }

  return false;
}

void Manager::render_icon(BLContext &ctx, const std::string &icon_name,
                          double x, double y, double size) {
  ensure_initialized();

  if (icon_name.empty())
    return;

  std::string cache_key = icon_name + "_" + std::to_string((int)size);

  if (icon_cache.find(cache_key) != icon_cache.end()) {
    ctx.blit_image(BLPoint(std::floor(x), std::floor(y)),
                   icon_cache[cache_key]);
    return;
  }

  std::string path = theme_loader.lookup_icon(icon_name);
  if (path.empty())
    return;

  if (load_icon_image(path, cache_key, size)) {
    ctx.blit_image(BLPoint(std::floor(x), std::floor(y)),
                   icon_cache[cache_key]);
  }
}

} // namespace Lawnch::Core::Icons
