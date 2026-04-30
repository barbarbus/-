#include <klib.h>

static uintptr_t heap_ptr = 0;

void *kalloc(size_t sz) {
  if (sz == 0) return NULL;
  if (heap_ptr == 0) heap_ptr = (uintptr_t)_heap.start;
  // 8-byte align
  heap_ptr = (heap_ptr + 7) & ~((uintptr_t)7);
  uintptr_t next = heap_ptr + sz;
  if (next > (uintptr_t)_heap.end) return NULL;
  void *ret = (void *)heap_ptr;
  heap_ptr = next;
  return ret;
}

void kfree(void *p) {
  (void)p;
  // No-op bump allocator
}

