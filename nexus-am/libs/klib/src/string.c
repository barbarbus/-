#include <klib.h>
#include <stdbool.h>

void *memset(void *v, int c, size_t n) {
  unsigned char *p = (unsigned char *)v;
  for (size_t i = 0; i < n; i++) p[i] = (unsigned char)c;
  return v;
}

void *memcpy(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  for (size_t i = 0; i < n; i++) d[i] = s[i];
  return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  if (d == s || n == 0) return dst;
  if (d < s) {
    for (size_t i = 0; i < n; i++) d[i] = s[i];
  } else {
    for (size_t i = n; i != 0; i--) d[i - 1] = s[i - 1];
  }
  return dst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *a = (const unsigned char *)s1;
  const unsigned char *b = (const unsigned char *)s2;
  for (size_t i = 0; i < n; i++) {
    if (a[i] != b[i]) return (int)a[i] - (int)b[i];
  }
  return 0;
}

size_t strlen(const char *s) {
  size_t n = 0;
  while (s && s[n]) n++;
  return n;
}

char *strcpy(char *dst, const char *src) {
  char *d = dst;
  while ((*d++ = *src++) != '\0');
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i]; i++) dst[i] = src[i];
  for (; i < n; i++) dst[i] = '\0';
  return dst;
}

char *strcat(char *dst, const char *src) {
  char *d = dst;
  while (*d) d++;
  while ((*d++ = *src++) != '\0');
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) { s1++; s2++; }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    unsigned char a = (unsigned char)s1[i];
    unsigned char b = (unsigned char)s2[i];
    if (a != b) return a - b;
    if (a == 0) return 0;
  }
  return 0;
}

const char *strchr(const char *s, int c) {
  char ch = (char)c;
  while (*s) {
    if (*s == ch) return s;
    s++;
  }
  return (ch == '\0') ? s : NULL;
}

char *strstr(const char *haystack, const char *needle) {
  if (!*needle) return (char *)haystack;
  size_t nlen = strlen(needle);
  for (const char *p = haystack; *p; p++) {
    if (*p == *needle && strncmp(p, needle, nlen) == 0) return (char *)p;
  }
  return NULL;
}

char *strtok(char *s, const char *delim) {
  static char *save;
  if (s) save = s;
  if (!save) return NULL;

  // skip delimiters
  char *p = save;
  while (*p) {
    const char *d = delim;
    bool is_delim = false;
    while (*d) { if (*p == *d) { is_delim = true; break; } d++; }
    if (!is_delim) break;
    p++;
  }
  if (*p == '\0') { save = NULL; return NULL; }

  char *start = p;
  while (*p) {
    const char *d = delim;
    while (*d) {
      if (*p == *d) {
        *p = '\0';
        save = p + 1;
        return start;
      }
      d++;
    }
    p++;
  }
  save = NULL;
  return start;
}

