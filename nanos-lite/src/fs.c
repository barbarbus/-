#include "fs.h"

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

#define invalid_fd(fd) ((fd) < 0 || (fd) >= NR_FILES)

void ramdisk_read(void *buf, off_t offset, size_t len);
void ramdisk_write(const void *buf, off_t offset, size_t len);
size_t events_read(void *buf, size_t len);
void dispinfo_read(void *buf, off_t offset, size_t len);
void fb_write(const void *buf, off_t offset, size_t len);

static size_t serial_write(const void *buf, size_t len) {
  for (size_t i = 0; i < len; i ++) {
    _putc(((const char *)buf)[i]);
  }
  return len;
}

void init_fs() {
  file_table[FD_FB].size = _screen.width * _screen.height * sizeof(uint32_t);
  for (int i = 0; i < NR_FILES; i ++) {
    file_table[i].open_offset = 0;
  }
}

int fs_open(const char *pathname, int flags, int mode) {
  (void)flags;
  (void)mode;
  for (int i = 0; i < NR_FILES; i ++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {
  assert(!invalid_fd(fd));
  Finfo *f = &file_table[fd];
  assert(f->open_offset >= 0 && f->open_offset <= f->size);

  switch (fd) {
    case FD_STDIN: return 0;
    case FD_EVENTS: return events_read(buf, len);
    case FD_DISPINFO:
      if (f->open_offset + len > f->size) {
        len = f->size - f->open_offset;
      }
      dispinfo_read(buf, f->open_offset, len);
      f->open_offset += len;
      return len;
    default:
      if (f->open_offset + len > f->size) {
        len = f->size - f->open_offset;
      }
      ramdisk_read(buf, f->disk_offset + f->open_offset, len);
      f->open_offset += len;
      return len;
  }
}

size_t fs_write(int fd, const void *buf, size_t len) {
  assert(!invalid_fd(fd));
  Finfo *f = &file_table[fd];
  assert(f->open_offset >= 0 && f->open_offset <= f->size);

  switch (fd) {
    case FD_STDOUT:
    case FD_STDERR: return serial_write(buf, len);
    case FD_FB:
      if (f->open_offset + len > f->size) {
        len = f->size - f->open_offset;
      }
      fb_write(buf, f->open_offset, len);
      f->open_offset += len;
      return len;
    default:
      if (f->open_offset + len > f->size) {
        len = f->size - f->open_offset;
      }
      ramdisk_write(buf, f->disk_offset + f->open_offset, len);
      f->open_offset += len;
      return len;
  }
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  assert(!invalid_fd(fd));
  Finfo *f = &file_table[fd];

  switch (whence) {
    case SEEK_SET: f->open_offset = offset; break;
    case SEEK_CUR: f->open_offset += offset; break;
    case SEEK_END: f->open_offset = f->size + offset; break;
    default: assert(0);
  }

  assert(f->open_offset >= 0 && f->open_offset <= f->size);
  return f->open_offset;
}

int fs_close(int fd) {
  assert(!invalid_fd(fd));
  return 0;
}
