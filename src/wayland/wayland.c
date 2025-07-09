#include "wayland.h"
#include "../wayland-protocols/xdg-shell-protocol.h"
#include "utils/shm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <wayland-client.h>

#define panic(text) { printf(text); exit(-1); }
#define try_or_panic(val, text) { if((val) != 0) panic(text) }
#define zero_init(var, type) memset(var, 0, sizeof(type))

// Functions
// =========

// Wayland Callbacks
static void cb_wl_registry_global_add(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version);
static void cb_wl_registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name);
static void cb_xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
static void cb_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
static void cb_wl_buffer_release(void *data, struct wl_buffer *wl_buffer);
static void cb_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
static void cb_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel);
static void cb_xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height);
static void cb_xdg_toplevel_wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities);

// Library
static void configure_buffers(struct twl_window *win);
static void draw_frame(struct twl_window *win);

// Wayland Listeners
// =================

static const struct wl_registry_listener wl_registry_listener = {
    .global = cb_wl_registry_global_add,
    .global_remove = cb_wl_registry_global_remove,
};

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = cb_xdg_wm_base_ping,
};

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = cb_wl_buffer_release,
};

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = cb_xdg_surface_configure,
};

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = cb_xdg_toplevel_configure,
    .close = cb_xdg_toplevel_close,
    .configure_bounds = cb_xdg_toplevel_configure_bounds,
    .wm_capabilities = cb_xdg_toplevel_wm_capabilities,
};

// Implementation: Wayland Callbacks
// =================================

static void cb_wl_registry_global_add(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
  struct twl_context *ctx = data;

  if (strcmp(interface, wl_shm_interface.name) == 0) {
    ctx->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    ctx->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
  } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
    ctx->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
  }
}

static void cb_wl_registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) { /* todo */ }

static void cb_xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
  xdg_wm_base_pong(xdg_wm_base, serial); //
}

static void cb_wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
  struct twl_window *win = data;
  win->buffer.in_use = 0;
}

static void cb_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
  struct twl_window *win = data;
  win->config = win->config_pending;

  xdg_surface_ack_configure(xdg_surface, serial);

  configure_buffers(win);

  draw_frame(win);
}

static void cb_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states) {
  struct twl_window *win = data;
  zero_init(&win->config_pending, struct twl_window_config);

  if (width == 0)
    width = win->constraints.default_width;
  if (height == 0)
    height = win->constraints.default_height;

  win->config_pending.width = width;
  win->config_pending.height = height;

  uint32_t *state;
  wl_array_for_each(state, states) {
    switch (*state) {
    case XDG_TOPLEVEL_STATE_MAXIMIZED:
      win->config_pending.is_maximized = 1;
      break;
    case XDG_TOPLEVEL_STATE_ACTIVATED:
      win->config_pending.is_activated = 1;
      break;
    case XDG_TOPLEVEL_STATE_RESIZING:
      win->config_pending.is_resizing = 1;
      break;
    case XDG_TOPLEVEL_STATE_FULLSCREEN:
      win->config_pending.is_fullscreen = 1;
      break;
      // TODO: Suspended
    }
  }
}

static void cb_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
  struct twl_window *win = data;
  win->should_close = 1;
}

static void cb_xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height) {
  // TODO
}

static void cb_xdg_toplevel_wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities) {
  // TODO
}

// Implementation: Library
// =======================

static size_t align_to_pagesize(size_t size) {
  size_t page_size = getpagesize();
  size = (size + page_size - 1) & ~(page_size - 1); // Round up to page boundary
  return size;
}

static void configure_buffers(struct twl_window *win) {
  uint32_t width = win->config.width;
  uint32_t height = win->config.height;
  uint32_t stride = width * 4;
  uint32_t format = WL_SHM_FORMAT_XRGB8888;

  uint32_t buffer_size = stride * height;
  uint32_t num_buffers = 2;

  buffer_size = align_to_pagesize(buffer_size);

  uint32_t pool_size = buffer_size * num_buffers;

  if (!win->pool.fd) {
    int fd = twl_shm_allocate(pool_size);
    if (fd < 0) {
      panic("SHM allocate failed\n");
    }
    win->pool.fd = fd;
    win->pool.size = pool_size;

    struct wl_shm_pool *pool = wl_shm_create_pool(win->ctx.wl_shm, fd, pool_size);
    win->pool.wl_shm_pool = pool;
  } else if (pool_size > win->pool.size) {
    int ok = twl_shm_resize(win->pool.fd, pool_size);
    if (ok != 0) {
      panic("SHM resize failed\n");
    }
    win->pool.size = pool_size;
    wl_shm_pool_resize(win->pool.wl_shm_pool, pool_size);
  }

  if (win->buffer.wl_buffer)
    wl_buffer_destroy(win->buffer.wl_buffer);
  try_or_panic(fzn_mmap_unmap(&win->buffer.mmap), "munmap back buffer");

  int fd = win->pool.fd;
  const fzn_mmap_config buffer_mmap_config = {
      .size = buffer_size,
      .prot = PROT_READ | PROT_WRITE,
      .flags = MAP_SHARED,
      .fd = fd,
      .offset = 0,
  };
  fzn_mmap buffer_mmap;
  fzn_mmap_new(&buffer_mmap, &buffer_mmap_config);
  if (buffer_mmap.addr == NULL) {
    panic("Failed to mmap buffer\n");
  }
  win->buffer.mmap = buffer_mmap;

  struct wl_buffer *wl_buffer = wl_shm_pool_create_buffer(win->pool.wl_shm_pool, 0, width, height, stride, format);
  wl_buffer_add_listener(wl_buffer, &wl_buffer_listener, win);
  win->buffer.wl_buffer = wl_buffer;
  win->buffer.in_use = 0;
}

int twl_init(struct twl_context *ctx) {
  zero_init(ctx, struct twl_context);

  struct wl_display *display = wl_display_connect(NULL);

  if (!display) {
    panic("Failed to connect to the display\n");
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
  zero_init(win, struct twl_window);

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
  xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, win);

  win->should_close = 0;

  return 0;
}

int twl_main(char *title, struct twl_window_constraints *constraints, draw_fn draw, void *data) {
  struct twl_context ctx;
  struct twl_window win;

  if (twl_init(&ctx) != 0) {
    return -1;
  }
  if (twl_window_init(&ctx, &win, title, draw, data) != 0) {
    return -1;
  }
  win.constraints = *constraints;

  wl_surface_commit(win.wl_surface);

  while (wl_display_dispatch(ctx.wl_display) != -1 && !win.should_close) {
    usleep(160);
    draw_frame(&win);
  }

  wl_display_disconnect(ctx.wl_display);
  return 0;
}

static void draw_frame(struct twl_window *win) {
  void *buffer_data = win->buffer.mmap.addr;

  (win->draw_fn)(win, buffer_data);

  wl_surface_attach(win->wl_surface, win->buffer.wl_buffer, 0, 0);
  wl_surface_damage_buffer(win->wl_surface, 0, 0, win->config.width, win->config.height);
  wl_surface_commit(win->wl_surface);
  win->buffer.in_use = 1;
}
