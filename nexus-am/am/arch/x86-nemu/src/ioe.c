#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  unsigned long now = inl(RTC_PORT);
  return now - boot_time;
}

uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void* memcpy(void *, const void *, int);

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  if (pixels == NULL || w <= 0 || h <= 0) return;

  // For this PA, callers provide x/y within the screen.
  // Still clip on the right/bottom edges defensively.
  int cw = w, ch = h;
  if (x + cw > _screen.width)  cw = _screen.width  - x;
  if (y + ch > _screen.height) ch = _screen.height - y;
  if (cw <= 0 || ch <= 0) return;

  for (int j = 0; j < ch; j++) {
    memcpy(&fb[(y + j) * _screen.width + x], &pixels[j * w], cw * sizeof(uint32_t));
  }
}

void _draw_sync() {
  asm volatile("" ::: "memory");
}

int _read_key() {
  const uint16_t I8042_DATA_PORT = 0x60;
  const uint16_t I8042_STATUS_PORT = 0x64;
  const uint8_t I8042_STATUS_HASKEY_MASK = 0x1;

  uint8_t status = inb(I8042_STATUS_PORT);
  if ((status & I8042_STATUS_HASKEY_MASK) == 0) {
    return _KEY_NONE;
  }
  return inl(I8042_DATA_PORT);
}
