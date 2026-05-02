#include <klib.h>

static unsigned int rng_state = 1;

int atoi(const char *nptr) {
  if (!nptr) return 0;
  int sign = 1;
  if (*nptr == '-') { sign = -1; nptr++; }
  int v = 0;
  while (*nptr >= '0' && *nptr <= '9') {
    v = v * 10 + (*nptr - '0');
    nptr++;
  }
  return sign * v;
}

int abs(int x) { return x < 0 ? -x : x; }

unsigned long time() {
  return _uptime() / 1000;
}

void srand(unsigned int seed) {
  rng_state = seed ? seed : 1;
}

int rand() {
  // LCG parameters from ANSI C
  rng_state = rng_state * 1103515245u + 12345u;
  return (int)((rng_state >> 16) & 0x7fff);
}

