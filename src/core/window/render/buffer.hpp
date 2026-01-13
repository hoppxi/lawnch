#pragma once

#include <blend2d.h>
#include <wayland-client.h>

namespace Lawnch::Core::Window::Render {

class Buffer {
public:
  Buffer();
  ~Buffer();

  void create(struct wl_shm *shm, int width, int height);
  void destroy();

  struct wl_buffer *get_wl_buffer() const { return wl_buffer; }
  BLContext &get_context() { return context; }
  BLImage &get_image() { return image; }
  int get_width() const { return width; }
  int get_height() const { return height; }

  bool is_valid() const { return wl_buffer != nullptr; }

private:
  struct wl_buffer *wl_buffer = nullptr;
  BLImage image;
  BLContext context;
  void *shm_data = nullptr;
  size_t shm_size = 0;
  int width = 0;
  int height = 0;
};

} // namespace Lawnch::Core::Window::Render
