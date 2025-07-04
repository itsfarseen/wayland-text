#include "wayland/wayland.h"
#include <stdio.h>
#include <wayland-client.h>

void draw(struct twl_window *win, void *frame) {
  uint32_t *data = frame;
  uint32_t width = win->config.width;
  uint32_t height = win->config.height;
  printf("WxH: %ux%u\n", width, height);

  printf("HERE 4\n");
  /* Draw checkerboxed background */
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if ((x + y / 8 * 8) % 16 < 8)
        data[y * width + x] = 0xFF666666;
      else
        data[y * width + x] = 0xFFEEEEEE;
    }
  }
  printf("HERE 4.1\n");
}

int main(int argc, char *argv[]) {
  struct twl_window_config config = {
      .width = 800,
      .height = 600,
      .bytes_per_pixel = 4,
      .format = WL_SHM_FORMAT_XRGB8888,
  };
  twl_main("Hello, new world!", config, draw, NULL);
  return 0;
}
