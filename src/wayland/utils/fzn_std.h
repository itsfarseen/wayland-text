#ifndef __TWL_MMAP_H__
#define __TWL_MMAP_H__

#include <stdlib.h>

typedef enum {
  FZN_SUCCESS = 0,
  FZN_STR_ALLOC_FAILED = 100,
  FZN_MMAP_FAILED = 200,
  FZN_MMAP_UNMAP_FAILED,
} fzn_err;

#define BAIL_ON_ERR(res) if(res != FZN_SUCCESS) return res;

// str

typedef struct {
  void *data;
  size_t len;
  size_t capacity;
  int is_owned;
} fzn_str;

fzn_str fzn_str_empty();
fzn_str fzn_str_new(const char *str);
fzn_err fzn_str_make_owned(fzn_str *str);
fzn_err fzn_str_append(fzn_str *str, const char *s);
fzn_err fzn_str_free(fzn_str *str);
const char *fzn_str_charp(const fzn_str *str);
fzn_err fzn_str_reserve(fzn_str *str, size_t new_capacity);

fzn_str fzn_errno_str();

// mmap

typedef struct {
  void *addr;
  size_t size;
} fzn_mmap;

typedef struct {
  size_t size;
  int prot;
  int flags;
  int fd;
  off_t offset;
} fzn_mmap_config;

fzn_err fzn_mmap_new(fzn_mmap *out, const fzn_mmap_config *config);
fzn_err fzn_mmap_unmap(fzn_mmap *x);

#endif
