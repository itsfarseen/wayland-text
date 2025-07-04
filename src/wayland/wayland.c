#include "wayland.h"
#include "../wayland-protocols/xdg-shell-protocol.h"
#include "utils/shm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <wayland-client.h>

// Wayland Listeners
// =================

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
static const struct xdg_wm_base_listener xdg_wm_base_listener = {.ping = xdg_wm_base_ping};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
  xdg_wm_base_pong(xdg_wm_base, serial); //
}

static struct wl_buffer *draw_frame(struct twl_window *win);

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
static const struct xdg_surface_listener xdg_surface_listener = {.configure = xdg_surface_configure};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
  struct twl_window *win = data;
  xdg_surface_ack_configure(xdg_surface, serial);

  struct wl_buffer *wl_buffer = draw_frame(win);

  wl_surface_attach(win->wl_surface, wl_buffer, 0, 0);
  wl_surface_commit(win->wl_surface);
}

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer);
static const struct wl_buffer_listener wl_buffer_listener = {.release = wl_buffer_release};

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
  wl_buffer_destroy(wl_buffer); //
}

static void wl_registry_global_add(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version);
static void wl_registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name);
static const struct wl_registry_listener wl_registry_listener = {.global = wl_registry_global_add, .global_remove = wl_registry_global_remove};

static void wl_registry_global_add(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
  struct twl_context *ctx = data;

  if (strcmp(interface, wl_shm_interface.name) == 0) {
    ctx->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    ctx->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
  } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
    ctx->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
  }
}

static void wl_registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) { /* todo */ }

// Library
// =======

int twl_init(struct twl_context *ctx) {
  struct wl_display *display = wl_display_connect(NULL);

  if (!display) {
    printf("Failed to connect to the display\n");
    return -1;
  }

  ctx->wl_display = display;
  struct wl_registry *registry = wl_display_get_registry(display);

  wl_registry_add_listener(registry, &wl_registry_listener, ctx);
  wl_display_roundtrip(display);

  assert(ctx->wl_compositor);
  assert(ctx->wl_shm);
  assert(ctx->xdg_wm_base);

  xdg_wm_base_add_listener(ctx->xdg_wm_base, &xdg_wm_base_listener, NULL);

  return 0;
}

int twl_window_init(struct twl_context *ctx, struct twl_window *win, const char *title, draw_fn draw_fn, void *user_data) {
  win->ctx = *ctx;
  win->draw_fn = draw_fn;
  win->user_data = user_data;

  struct wl_surface *wl_surface = wl_compositor_create_surface(ctx->wl_compositor);
  win->wl_surface = wl_surface;

  struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(ctx->xdg_wm_base, wl_surface);
  win->xdg_surface = xdg_surface;
  xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, win);

  struct xdg_toplevel *xdg_toplevel = xdg_surface_get_toplevel(win->xdg_surface);
  win->xdg_toplevel = xdg_toplevel;
  xdg_toplevel_set_title(xdg_toplevel, title);

  return 0;
}

int twl_main(char *title, struct twl_window_config config, draw_fn draw, void *data) {
  struct twl_context ctx;
  struct twl_window win;

  if (twl_init(&ctx) != 0) {
    return -1;
  }
  if (twl_window_init(&ctx, &win, title, draw, data) != 0) {
    return -1;
  }
  win.config = config;

  wl_surface_commit(win.wl_surface);

  while (wl_display_dispatch(ctx.wl_display) != -1) {
  }

  wl_display_disconnect(ctx.wl_display);
  return 0;
}

struct wl_buffer *draw_frame(struct twl_window *win) {
  struct twl_window_config twl_config = win->config;
  uint32_t width = twl_config.width;
  uint32_t height = twl_config.height;
  uint32_t stride = width * twl_config.bytes_per_pixel;
  uint32_t size = stride * height;
  uint32_t format = twl_config.format;

  int fd = allocate_shm_file(size);
  if (fd == -1) {
    return NULL;
  }

  void *buffer_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (buffer_data == MAP_FAILED) {
    close(fd);
    return NULL;
  }

  struct wl_shm_pool *pool = wl_shm_create_pool(win->ctx.wl_shm, fd, size);
  struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, format);
  wl_shm_pool_destroy(pool);
  close(fd);

  (win->draw_fn)(win, buffer_data);

  munmap(buffer_data, size);
  wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
  return buffer;
}
