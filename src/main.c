#include "wayland/wayland.h"
#include <stdio.h>
#include <wayland-client.h>

void draw(struct twl_window *win, void *frame) {
  uint32_t *data = frame;
  uint32_t width = win->config.width;
  uint32_t height = win->config.height;

  /* Draw checkerboxed background */
  uint32_t color = 0xFF666666;
  if (win->config.is_resizing)
    color |= 0x000000FF;
  if (win->config.is_activated)
    color |= 0x00FF0000;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if ((x + y / 8 * 8) % 16 < 8)
        data[y * width + x] = color;
      else
        data[y * width + x] = 0xFFEEEEEE;
    }
  }
}

int main(int argc, char *argv[]) {
  struct twl_window_constraints constraints = {
      .default_width = 800,
      .default_height = 600,
  };
  twl_main("Hello, new world!", &constraints, draw, NULL);
  return 0;
}
