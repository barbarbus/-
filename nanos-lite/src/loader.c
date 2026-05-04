#include "common.h"
#include "fs.h"

#define USER_ENTRY 0x4000000u

uintptr_t loader(_Protect *as, const char *filename) {
  (void)as;
  int fd = fs_open(filename, 0, 0);
  assert(fd >= 0);
  size_t sz = fs_lseek(fd, 0, SEEK_END);
  fs_lseek(fd, 0, SEEK_SET);
  fs_read(fd, (void *)USER_ENTRY, sz);
  fs_close(fd);
  return USER_ENTRY;
}
