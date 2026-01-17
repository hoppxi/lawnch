#pragma once

#include "theme_loader.hpp"
#include <blend2d.h>
#include <map>
#include <memory>
#include <string>

struct NSVGimage;

namespace Lawnch::Core::Icons {

class Manager {
public:
  static Manager &Instance();

  void render_icon(BLContext &ctx, const std::string &icon_name, double x,
                   double y, double size);

private:
  Manager();
  ~Manager();

  void init();
  void ensure_initialized();

  bool load_icon_image(const std::string &path, const std::string &cache_key,
                       double size);

  bool initialized = false;
  ThemeLoader theme_loader;

  std::map<std::string, NSVGimage *> cache;
  std::map<std::string, BLImage> icon_cache;
};

} // namespace Lawnch::Core::Icons
