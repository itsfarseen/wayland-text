#include "../wayland-protocols/xdg-shell-protocol.h"
#include "./utils/fzn_mmap.h"
#include <wayland-client.h>

struct twl_context {
  // Wayland Display
  struct wl_display *wl_display;
  // Registry Objects
  struct wl_compositor *wl_compositor;
  struct wl_shm *wl_shm;
  struct xdg_wm_base *xdg_wm_base;
};

struct twl_window;

typedef void (*draw_fn)(struct twl_window *win, void *buffer);

struct twl_window_config {
  uint32_t width;
  uint32_t height;
  uint32_t is_maximized;
  uint32_t is_fullscreen;
  uint32_t is_resizing;
  uint32_t is_activated;
};

struct twl_window_constraints {
  uint32_t default_width;
  uint32_t default_height;
};

struct twl_buffer_pool {
  int fd;
  uint32_t size;
  struct wl_shm_pool *wl_shm_pool;
};

struct twl_buffer {
  struct fzn_mmap_handle mmap;
  struct wl_buffer *wl_buffer;
  int in_use;
};

struct twl_window {
  // Parent context
  struct twl_context ctx;
  // Wayland objects
  struct wl_surface *wl_surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  // Buffers
  struct twl_buffer_pool pool;
  struct twl_buffer buffer;
  // Config
  struct twl_window_constraints constraints;
  struct twl_window_config config;
  struct twl_window_config config_pending;
  uint32_t should_close;
  // User draw hook
  draw_fn draw_fn;
  void *user_data;
};

int twl_init(struct twl_context *ctx);
int twl_main(char *title, struct twl_window_constraints *constraints, draw_fn draw, void *data);
