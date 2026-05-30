#include "common.h"
#include "proc.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  static char evtbuf[64];
  static size_t evt_len = 0, evt_pos = 0;

  if (len == 0) return 0;

  if (evt_pos >= evt_len) {
    int key = _read_key();
    if (key != _KEY_NONE) {
      bool is_keydown = (key & 0x8000) != 0;
      int keycode = key & 0x7fff;
      if (is_keydown && keycode == _KEY_F12) {
        switch_game();
      }
      snprintf(evtbuf, sizeof(evtbuf), "%s %s\n",
          is_keydown ? "kd" : "ku", keyname[keycode]);
    } else {
      /* _read_key() is non-blocking; emit a timer line so NDL_WaitEvent can
       * drive the game loop / rendering. */
      snprintf(evtbuf, sizeof(evtbuf), "t %u\n", (unsigned)_uptime());
    }
    /* klib's snprintf return value can include the trailing '\0'; use
     * strlen so we never leak a NUL byte through /dev/events. */
    evt_len = strlen(evtbuf);
    evt_pos = 0;
  }

  size_t n = evt_len - evt_pos;
  if (n > len) n = len;
  memcpy(buf, evtbuf + evt_pos, n);
  evt_pos += n;
  return n;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  int width = _screen.width;
  int height = _screen.height;

  assert(offset % 4 == 0 && len % 4 == 0);

  const uint32_t *pixels = (const uint32_t *)buf;
  size_t total = len / sizeof(uint32_t);
  int pos = (int)(offset / sizeof(uint32_t));

  while (total > 0) {
    int x = pos % width;
    int y = pos / width;
    if (y >= height) {
      break;
    }

    /* Contiguous full-width rows: one _draw_rect (matches PAL/SDL bulk blits). */
    if (x == 0 && total >= (size_t)width) {
      int max_rows = height - y;
      size_t rows = total / (size_t)width;
      if (rows > (size_t)max_rows) {
        rows = (size_t)max_rows;
      }
      if (rows > 0) {
        _draw_rect(pixels, 0, y, width, (int)rows);
        size_t chunk = rows * (size_t)width;
        pixels += chunk;
        pos += (int)chunk;
        total -= chunk;
        continue;
      }
    }

    int line = width - x;
    if ((size_t)line > total) {
      line = (int)total;
    }
    _draw_rect(pixels, x, y, line, 1);
    pixels += line;
    pos += line;
    total -= (size_t)line;
  }

  _draw_sync();
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  snprintf(dispinfo, sizeof(dispinfo), "WIDTH:%d\nHEIGHT:%d\n",
      _screen.width, _screen.height);
}
