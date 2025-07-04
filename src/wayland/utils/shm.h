#include <unistd.h>

// returns fd
int twl_shm_allocate(size_t size);
int twl_shm_resize(int fd, size_t size);
