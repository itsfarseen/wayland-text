#include "fzn_mmap.h"
#include <stdio.h>
#include <sys/mman.h>

struct fzn_mmap_handle fzn_mmap(size_t size, int prot, int flags, int fd, off_t offset) {
  struct fzn_mmap_handle result = {.addr = NULL, .size = 0};

  void *addr = mmap(NULL, size, prot, flags, fd, offset);
  if (addr == MAP_FAILED) {
    perror("fzn_mmap");
    return result;
  }

  result.addr = addr;
  result.size = size;
  return result;
}

int fzn_munmap(struct fzn_mmap_handle *handle) {
  if (handle->addr == NULL)
    return 0;
  int res = munmap(handle->addr, handle->size);
  handle->addr = NULL;
  return res;
}
