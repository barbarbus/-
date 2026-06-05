#ifndef __MMIO_H__
#define __MMIO_H__

#include "common.h"

typedef void(*mmio_callback_t)(paddr_t, int, bool);

void* add_mmio_map(paddr_t, int, mmio_callback_t);
int is_mmio(paddr_t);

uint32_t mmio_read(paddr_t, int, int);
void mmio_write(paddr_t, int, uint32_t, int);

/* Bounding box of all MMIO regions; used by the inlined paddr_read/write fast
 * path in memory.h to short-circuit RAM accesses without calling is_mmio(). */
extern paddr_t mmio_lbound, mmio_ubound;

#endif
