#include "wayland.h"
#include "shm.h"
#include "wayland/generated/xdg-shell-protocol.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <wayland-client.h>

static void global_add(void *data, struct wl_registry *wl_registry,
                       uint32_t name, const char *interface, uint32_t version);
static void global_remove(void *data, struct wl_registry *wl_registry,
                          uint32_t name);

static struct wl_buffer *draw_frame(struct wl_globals *globals);

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
                                  uint32_t serial);

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
                             uint32_t serial);

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer);

static const struct wl_registry_listener wl_registry_listener = {
    .global = global_add,
    .global_remove = global_remove,
};

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

int wl_main(char *title, struct wl_config wl_config, void *data, draw_fn draw) {
  struct wl_globals globals;
  globals.wl_config = wl_config;
  globals.data = data;
  globals.draw = draw;

  struct wl_display *display = wl_display_connect(NULL);

  if (!display) {
    printf("Failed to connect to the display\n");
    return -1;
  }

  struct wl_registry *registry = wl_display_get_registry(display);

  wl_registry_add_listener(registry, &wl_registry_listener, &globals);
  wl_display_roundtrip(display);

  xdg_wm_base_add_listener(globals.xdg_wm_base, &xdg_wm_base_listener,
                           &globals);

  struct wl_surface *wl_surface =
      wl_compositor_create_surface(globals.wl_compositor);

  globals.wl_surface = wl_surface;

  struct xdg_surface *xdg_surface =
      xdg_wm_base_get_xdg_surface(globals.xdg_wm_base, wl_surface);

  xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, &globals);

  struct xdg_toplevel *xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
  xdg_toplevel_set_title(xdg_toplevel, title);

  wl_surface_commit(wl_surface);

  while (wl_display_dispatch(display) != -1) {
  }

  wl_display_disconnect(display);
  return 0;
}

/* struct wl_globals */

void global_add(void *data, struct wl_registry *wl_registry, uint32_t name,
                const char *interface, uint32_t version) {
  struct wl_globals *globals = data;

  if (strcmp(interface, wl_shm_interface.name) == 0) {
    globals->wl_shm =
        wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    globals->xdg_wm_base =
        wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
  } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
    globals->wl_compositor =
        wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
  }
}

void global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
  /* todo */
}

/* XDG Shell */

void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
                      uint32_t serial) {
  xdg_wm_base_pong(xdg_wm_base, serial);
}

void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
                           uint32_t serial) {
  struct wl_globals *globals = data;
  xdg_surface_ack_configure(xdg_surface, serial);

  struct wl_buffer *wl_buffer = draw_frame(globals);

  wl_surface_attach(globals->wl_surface, wl_buffer, 0, 0);
  wl_surface_commit(globals->wl_surface);
}

struct wl_buffer *draw_frame(struct wl_globals *globals) {
  struct wl_config wl_config = globals->wl_config;
  uint32_t width = wl_config.width;
  uint32_t height = wl_config.height;
  uint32_t stride = width * wl_config.bytes_per_pixel;
  uint32_t size = stride * height;
  uint32_t format = wl_config.format;

  int fd = allocate_shm_file(size);
  if (fd == -1) {
    return NULL;
  }

  void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    return NULL;
  }

  struct wl_shm_pool *pool = wl_shm_create_pool(globals->wl_shm, fd, size);
  struct wl_buffer *buffer =
      wl_shm_pool_create_buffer(pool, 0, width, height, stride, format);
  wl_shm_pool_destroy(pool);
  close(fd);

  globals->draw(globals, data);

  munmap(data, size);
  wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
  return buffer;
}

void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
  wl_buffer_destroy(wl_buffer);
}
