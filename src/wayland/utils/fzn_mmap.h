#ifndef __TWL_MMAP_H__
#define __TWL_MMAP_H__

#include <stdlib.h>

struct fzn_mmap_handle {
  void *addr;
  size_t size;
};

struct fzn_mmap_handle fzn_mmap(size_t size, int prot, int flags, int fd, off_t offset);
int fzn_munmap(struct fzn_mmap_handle *handle);

#endif
