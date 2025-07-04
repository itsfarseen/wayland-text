#include "../wayland-protocols/xdg-shell-protocol.h"
#include <wayland-client.h>

struct twl_window;

struct twl_window_config {
  uint32_t width;
  uint32_t height;
  uint32_t bytes_per_pixel;
  uint32_t format;
};

typedef void (*draw_fn)(struct twl_window *win, void *buffer);

struct twl_context {
  // Wayland Display
  struct wl_display *wl_display;
  // Registry Objects
  struct wl_compositor *wl_compositor;
  struct wl_shm *wl_shm;
  struct xdg_wm_base *xdg_wm_base;
};

int twl_init(struct twl_context *ctx);

struct twl_window {
  // Parent context
  struct twl_context ctx;
  // Wayland objects
  struct wl_surface *wl_surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  // Config
  struct twl_window_config config;
  draw_fn draw_fn;
  void *user_data;
};

int twl_main(char *title, struct twl_window_config config, draw_fn draw, void *data);
