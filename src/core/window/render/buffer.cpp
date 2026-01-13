#include "buffer.hpp"
#include "../../../helpers/logger.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

namespace Lawnch::Core::Window::Render {

Buffer::Buffer() {}

Buffer::~Buffer() { destroy(); }

static int create_shm_file(off_t size) {
  int fd = memfd_create("lawnch-buffer", MFD_CLOEXEC);
  if (fd < 0)
    return -1;

  int ret = ftruncate(fd, size);
  if (ret < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

void Buffer::create(struct wl_shm *shm, int w, int h) {
  destroy(); // Setup fresh

  width = w;
  height = h;
  int stride = width * 4; // ARGB32
  shm_size = stride * height;

  int fd = create_shm_file(shm_size);
  if (fd < 0) {
    Lawnch::Logger::log("Renderer", Lawnch::Logger::LogLevel::ERROR,
                        "Failed to create SHM file");
    return;
  }

  shm_data = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (shm_data == MAP_FAILED) {
    close(fd);
    Lawnch::Logger::log("Renderer", Lawnch::Logger::LogLevel::ERROR,
                        "Failed to mmap");
    shm_data = nullptr;
    return;
  }

  struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, shm_size);
  wl_buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
                                        WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);

  image.create_from_data(width, height, BL_FORMAT_PRGB32, shm_data, stride);
  context.begin(image);
}

void Buffer::destroy() {
  if (wl_buffer) {
    wl_buffer_destroy(wl_buffer);
    wl_buffer = nullptr;
  }

  context.end();
  image.reset();

  if (shm_data) {
    munmap(shm_data, shm_size);
    shm_data = nullptr;
  }
}

} // namespace Lawnch::Core::Window::Render
