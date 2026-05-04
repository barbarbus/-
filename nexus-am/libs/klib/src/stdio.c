#include <klib.h>
#include <stdbool.h>

static inline void out_char(char **out, size_t *remain, char ch) {
  if (out && *out) {
    if (*remain > 1) {
      **out = ch;
      (*out)++;
      (*remain)--;
    }
  } else {
    _putc(ch);
  }
}

static inline void out_str(char **out, size_t *remain, const char *s) {
  if (!s) s = "(null)";
  while (*s) out_char(out, remain, *s++);
}

static inline void out_uint(char **out, size_t *remain, unsigned int v, unsigned base, bool upper) {
  char buf[32];
  int i = 0;
  if (v == 0) buf[i++] = '0';
  while (v) {
    unsigned d = v % base;
    v /= base;
    if (d < 10) buf[i++] = '0' + d;
    else buf[i++] = (upper ? 'A' : 'a') + (d - 10);
  }
  while (i--) out_char(out, remain, buf[i]);
}

static inline void out_int(char **out, size_t *remain, int v) {
  if (v < 0) {
    out_char(out, remain, '-');
    out_uint(out, remain, (unsigned int)(-v), 10, false);
  } else {
    out_uint(out, remain, (unsigned int)v, 10, false);
  }
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  char *out = str;
  size_t remain = (size == 0 ? 0 : size);

  const char *p = format;
  while (p && *p) {
    if (*p != '%') {
      out_char(str ? &out : NULL, &remain, *p++);
      continue;
    }
    p++; // skip '%'
    if (*p == '%') { out_char(str ? &out : NULL, &remain, '%'); p++; continue; }

    switch (*p) {
      case 'c': {
        int ch = va_arg(ap, int);
        out_char(str ? &out : NULL, &remain, (char)ch);
        p++;
        break;
      }
      case 's': {
        const char *s = va_arg(ap, const char *);
        out_str(str ? &out : NULL, &remain, s);
        p++;
        break;
      }
      case 'd': {
        int v = va_arg(ap, int);
        out_int(str ? &out : NULL, &remain, v);
        p++;
        break;
      }
      case 'u': {
        unsigned int v = va_arg(ap, unsigned int);
        out_uint(str ? &out : NULL, &remain, v, 10, false);
        p++;
        break;
      }
      case 'x': {
        unsigned int v = va_arg(ap, unsigned int);
        out_uint(str ? &out : NULL, &remain, v, 16, false);
        p++;
        break;
      }
      case 'X': {
        unsigned int v = va_arg(ap, unsigned int);
        out_uint(str ? &out : NULL, &remain, v, 16, true);
        p++;
        break;
      }
      case 'p': {
        uintptr_t v = (uintptr_t)va_arg(ap, void *);
        out_str(str ? &out : NULL, &remain, "0x");
        out_uint(str ? &out : NULL, &remain, (unsigned int)v, 16, false);
        p++;
        break;
      }
      default: {
        // Unsupported, print literally
        out_char(str ? &out : NULL, &remain, '%');
        out_char(str ? &out : NULL, &remain, *p ? *p : '?');
        if (*p) p++;
        break;
      }
    }
  }

  if (str && size > 0) {
    *out = '\0';
  }
  // Return value is the number of chars that would have been written (best effort)
  return (int)(out - str);
}

int vsprintf(char *str, const char *format, va_list ap) {
  return vsnprintf(str, (size_t)-1, format, ap);
}

int snprintf(char *s, size_t n, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsnprintf(s, n, format, ap);
  va_end(ap);
  return ret;
}

int sprintf(char *out, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsnprintf(out, (size_t)-1, format, ap);
  va_end(ap);
  return ret;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  return ret;
}

int sscanf(const char *str, const char *format, ...) {
  (void)str; (void)format;
  return 0;
}

