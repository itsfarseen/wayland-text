#include "wayland/wayland.h"
#include <stdio.h>
#include <wayland-client.h>

void draw(struct wl_globals *globals, void *frame) {
  uint32_t *data = frame;
  uint32_t width = globals->wl_config.width;
  uint32_t height = globals->wl_config.height;

  /* Draw checkerboxed background */
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if ((x + y / 8 * 8) % 16 < 8)
        data[y * width + x] = 0xFF666666;
      else
        data[y * width + x] = 0xFFEEEEEE;
    }
  }
}

int main(int argc, char *argv[]) {
  struct wl_config wl_config = {
      .width = 800,
      .height = 600,
      .bytes_per_pixel = 4,
      .format = WL_SHM_FORMAT_XRGB8888,
  };
  wl_main("Hello, new world!", wl_config, NULL, draw);
  return 0;
}
