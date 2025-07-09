#include "fzn_std.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))

// str

fzn_str fzn_str_empty() {
  fzn_str s = {.data = NULL, .len = 0, .capacity = 0, .is_owned = 0};
  return s;
}

fzn_str fzn_str_new(const char *str) {
  size_t len = strlen(str);
  fzn_str s = {
      // this is fine because we don't mutate it as long as is_owned = 0.
      .data = (void *)str,
      .len = len,
      .capacity = 0,
      .is_owned = 0,
  };
  return s;
}

// This function should be idempotent.
// Also, it should be valid to call fzn_str_to_owned(s, s).
fzn_err fzn_str_make_owned(fzn_str *str) {
  if (str->is_owned) {
    return FZN_SUCCESS;
  }

  void *data = malloc(str->len);
  if (data == NULL) {
    return FZN_STR_ALLOC_FAILED;
  }

  memcpy(data, str->data, str->len);

  str->data = data;
  str->len = str->len;
  str->capacity = str->len;
  str->is_owned = 1;

  return FZN_SUCCESS;
}

fzn_err fzn_str_append(fzn_str *str, const char *s) {
  int res;
  res = fzn_str_make_owned(str);
  BAIL_ON_ERR(res);

  size_t n = strlen(s);

  res = fzn_str_reserve(str, str->len + n + 1);
  BAIL_ON_ERR(res);

  memmove(str->data + str->len, s, n);
  str->len += n;
  char *end = str->data + str->len;
  *end = 0;

  return FZN_SUCCESS;
}

fzn_err fzn_str_free(fzn_str *str) {
  free(str->data);
  str->data = NULL;
  str->len = 0;
  str->capacity = 0;
  str->is_owned = 0;

  return FZN_SUCCESS;
}

const char *fzn_str_charp(const fzn_str *str) {
  if (str->data == NULL)
    return "";
  return str->data;
}

fzn_err fzn_str_reserve(fzn_str *str, size_t new_capacity) {
  if (str->capacity > new_capacity)
    return FZN_SUCCESS;

  new_capacity = MAX(new_capacity, str->capacity * 2);

  void *data = realloc(str->data, new_capacity);
  if (data == NULL) {
    return FZN_STR_ALLOC_FAILED;
  }

  str->data = data;
  str->capacity = new_capacity;

  return FZN_SUCCESS;
}

// mmap

fzn_err fzn_mmap_new(fzn_mmap *out, const fzn_mmap_config *config) {
  void *addr = mmap(NULL, config->size, config->prot, config->flags, config->fd, config->offset);
  if (addr == MAP_FAILED) {
    return FZN_MMAP_FAILED;
  }

  out->addr = addr;
  out->size = config->size;
  return FZN_SUCCESS;
}

fzn_err fzn_mmap_unmap(fzn_mmap *x) {
  if (x->addr == NULL)
    return 0;

  int res = munmap(x->addr, x->size);
  if (res != 0) {
    return FZN_MMAP_UNMAP_FAILED;
  }

  x->addr = NULL;

  return FZN_SUCCESS;
}
