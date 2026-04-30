#include <klib.h>

static inline void swap_bytes(uint8_t *a, uint8_t *b, size_t n) {
  while (n--) {
    uint8_t t = *a;
    *a++ = *b;
    *b++ = t;
  }
}

static void qsort_impl(uint8_t *base, size_t nmemb, size_t size,
                       int (*compar)(const void *, const void *)) {
  if (nmemb < 2) return;

  uint8_t *pivot = base + (nmemb / 2) * size;
  size_t i = 0, j = nmemb - 1;

  while (1) {
    while (compar(base + i * size, pivot) < 0) i++;
    while (compar(base + j * size, pivot) > 0) j--;
    if (i >= j) break;
    swap_bytes(base + i * size, base + j * size, size);
    if (pivot == base + i * size) pivot = base + j * size;
    else if (pivot == base + j * size) pivot = base + i * size;
    i++; if (j > 0) j--;
  }

  qsort_impl(base, i, size, compar);
  qsort_impl(base + i * size, nmemb - i, size, compar);
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
  if (!base || !compar || size == 0) return;
  qsort_impl((uint8_t *)base, nmemb, size, compar);
}

