#include "generated/xdg-shell-protocol.h"
#include <wayland-client.h>

struct wl_globals;

struct wl_config {
  uint32_t width;
  uint32_t height;
  uint32_t bytes_per_pixel;
  uint32_t format;
};

typedef void (*draw_fn)(struct wl_globals *globals, void *buffer);

struct wl_globals {
  struct wl_compositor *wl_compositor;
  struct wl_shm *wl_shm;
  struct xdg_wm_base *xdg_wm_base;
  struct wl_surface *wl_surface;
  struct wl_config wl_config;
  void *data;
  draw_fn draw;
};

int wl_main(char *title, struct wl_config wl_config, void *data, draw_fn draw);
