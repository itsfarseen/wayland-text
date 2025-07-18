#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

// TODO: Switch to memfd
// TODO: Cleanup. I don't fully understand what's going on with the loop in allocate and resize.

static void randname(char *buf) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  long r = ts.tv_nsec;
  for (int i = 0; i < 6; ++i) {
    buf[i] = 'A' + (r & 15) + (r & 16) * 2;
    r >>= 5;
  }
}

static int create_shm_file(void) {
  int retries = 100;
  do {
    char name[] = "/wl_shm-XXXXXX";
    randname(name + sizeof(name) - 7);
    --retries;
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
      shm_unlink(name);
      return fd;
    }
  } while (retries > 0 && errno == EEXIST);
  return -1;
}

int twl_shm_allocate(size_t size) {
  int fd = create_shm_file();
  if (fd < 0)
    return -1;
  int ret;
  do {
    ret = ftruncate(fd, size);
  } while (ret < 0 && errno == EINTR);
  if (ret < 0) {
    perror("ftruncate in twl_shm_allocate");
    close(fd);
    return -1;
  }
  return fd;
}

int twl_shm_resize(int fd, size_t size) {
  int ret;
  do {
    ret = ftruncate(fd, size);
    if (ret != 0) {
      perror("failed to resize shm");
    }
  } while (ret < 0 && errno == EINTR);

  if (ret < 0) {
    close(fd);
    return -1;
  }
  return 0;
}

int twl_shm_close(int fd) { return close(fd); }
