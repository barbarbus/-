#include <ndl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

extern int _open(const char *, int, mode_t);
extern int _read(int, void *, size_t);

static int has_nwm = 0;
static uint32_t *canvas;
static uint32_t *screen_buf;
static FILE *fbdev, *evtdev;
/* Raw fd for /dev/events. Going through fopen+getc has been unreliable
 * (newlib stdio buffer sizing depends on _fstat which we don't fill in,
 * and one short read can wedge the stream's EOF flag). Use raw _read so
 * each NDL_WaitEvent corresponds to one syscall round-trip. */
static int evtfd = -1;

static void get_display_info();
static int canvas_w, canvas_h, screen_w, screen_h, pad_x, pad_y;

int NDL_OpenDisplay(int w, int h) {
  if (!canvas) {
    NDL_CloseDisplay();
  }

  canvas_w = w;
  canvas_h = h;
  canvas = malloc(sizeof(uint32_t) * w * h);
  assert(canvas);

  if (getenv("NWM_APP")) {
    has_nwm = 1;
  } else {
    has_nwm = 0;
  }

  if (has_nwm) {
    printf("\033[X%d;%ds", w, h); fflush(stdout);
    evtdev = stdin;
  } else {
    get_display_info();
    assert(screen_w >= canvas_w);
    assert(screen_h >= canvas_h);
    pad_x = (screen_w - canvas_w) / 2;
    pad_y = (screen_h - canvas_h) / 2;
    screen_buf = malloc(sizeof(uint32_t) * screen_w * screen_h);
    assert(screen_buf);
    memset(screen_buf, 0, sizeof(uint32_t) * screen_w * screen_h);
    fbdev = fopen("/dev/fb", "w"); assert(fbdev);
    evtfd = _open("/dev/events", O_RDONLY, 0);
    assert(evtfd >= 0);
    /* evtdev kept for legacy printf path; not used for input. */
    evtdev = NULL;
  }
}

int NDL_CloseDisplay() {
  if (canvas) {
    free(canvas);
    canvas = NULL;
  }
  if (screen_buf) {
    free(screen_buf);
    screen_buf = NULL;
  }
  return 0;
}

int NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  if (has_nwm) {
    for (int i = 0; i < h; i ++) {
      printf("\033[X%d;%d", x, y + i);
      for (int j = 0; j < w; j ++) {
        putchar(';');
        fwrite(&pixels[i * w + j], 1, 4, stdout);
      }
      printf("d\n");
    }
  } else {
    for (int i = 0; i < h; i ++) {
      for (int j = 0; j < w; j ++) {
        canvas[(i + y) * canvas_w + (j + x)] = pixels[i * w + j];
      }
    }
  }
}

int NDL_Render() {
  if (has_nwm) {
    fflush(stdout);
  } else {
    for (int i = 0; i < canvas_h; i ++) {
      memcpy(&screen_buf[(i + pad_y) * screen_w + pad_x],
          &canvas[i * canvas_w], sizeof(uint32_t) * canvas_w);
    }
    fseek(fbdev, 0, SEEK_SET);
    fwrite(screen_buf, sizeof(uint32_t), screen_w * screen_h, fbdev);
    fflush(fbdev);
  }
}

#define keyname(k) #k,

static const char *keys[] = {
  "NONE",
  _KEYS(keyname)
};

#define numkeys ( sizeof(keys) / sizeof(keys[0]) )

int NDL_WaitEvent(NDL_Event *event) {
  /* Persistent line buffer: a single _read may return part of one event
   * line and the start of another. Accumulate bytes here until '\n' is
   * seen, then parse one line at a time. */
  static char line[128];
  static int line_len = 0;

  while (1) {
    /* Defensive: strip any leading NULs that may have been injected by
     * the kernel side (klib's snprintf return-value quirk). */
    int skip = 0;
    while (skip < line_len && line[skip] == '\0') skip++;
    if (skip > 0) {
      int rest0 = line_len - skip;
      if (rest0 > 0) memmove(line, line + skip, rest0);
      line_len = rest0;
    }

    int nl = -1;
    for (int i = 0; i < line_len; i++) {
      if (line[i] == '\n') { nl = i; break; }
    }

    if (nl < 0) {
      /* Read directly from /dev/events. libc stdio buffering ate events
       * because _fstat doesn't fill st_blksize; use raw _read instead. */
      char chunk[64];
      int n = _read(evtfd, chunk, sizeof(chunk));
      if (n <= 0) continue;
      if (line_len + n > (int)sizeof(line)) line_len = 0;
      memcpy(line + line_len, chunk, n);
      line_len += n;
      continue;
    }

    char buf[128];
    int len = nl;
    if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, line, len);
    buf[len] = '\0';

    int rest = line_len - (nl + 1);
    if (rest > 0) memmove(line, line + nl + 1, rest);
    line_len = rest;

    if (buf[0] != 'k' && buf[0] != 't') continue;

    if (buf[0] == 'k') {
      char keyname[32] = {0};
      event->type = (buf[1] == 'd') ? NDL_EVENT_KEYDOWN : NDL_EVENT_KEYUP;
      event->data = -1;
      sscanf(buf + 3, "%31s", keyname);
      for (int i = 0; i < (int)numkeys; i++) {
        if (strcmp(keys[i], keyname) == 0) {
          event->data = i;
          break;
        }
      }
      if (event->data < 1) continue;
      return 0;
    }

    int tsc = 0;
    sscanf(buf + 2, "%d", &tsc);
    event->type = NDL_EVENT_TIMER;
    event->data = tsc;
    return 0;
  }
}

static void get_display_info() {
  FILE *dispinfo = fopen("/proc/dispinfo", "r");
  assert(dispinfo);
  screen_w = screen_h = 0;
  char buf[128], key[128], value[128], *delim;
  while (fgets(buf, 128, dispinfo)) {
    *(delim = strchr(buf, ':')) = '\0';
    sscanf(buf, "%s", key);
    sscanf(delim + 1, "%s", value);
    if (strcmp(key, "WIDTH") == 0) sscanf(value, "%d", &screen_w);
    if (strcmp(key, "HEIGHT") == 0) sscanf(value, "%d", &screen_h);
  }
  fclose(dispinfo);
  assert(screen_w > 0 && screen_h > 0);
}

