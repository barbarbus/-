#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "common.h"
#include "cpu/reg.h"
#ifdef HAS_IOE
#include "device/mmio.h"
#endif

#define PMEM_SIZE (128 * 1024 * 1024)

extern uint8_t pmem[];

/* convert the guest physical address in the guest program to host virtual address in NEMU */
#define guest_to_host(p) ((void *)(pmem + (unsigned)p))
/* convert the host virtual address in NEMU to guest physical address in the guest program */
#define host_to_guest(p) ((paddr_t)((void *)p - (void *)pmem))

/* ---------------------------------------------------------------------------
 * Physical-memory access. perf shows the vaddr_read/paddr_read/is_mmio chain
 * is ~34% of runtime, almost all of it cross-translation-unit call overhead.
 * These are defined `static inline` here so they collapse into each caller
 * (instr_fetch, rtl_lm/rtl_sm, ...). RAM accesses (the overwhelming majority)
 * skip the is_mmio() call entirely via the MMIO bounding-box check.
 * ------------------------------------------------------------------------- */
static inline uint32_t paddr_read(paddr_t addr, int len) {
#ifdef HAS_IOE
  if (addr >= mmio_lbound && addr <= mmio_ubound) {
    int map_NO = is_mmio(addr);
    if (map_NO != -1) { return mmio_read(addr, len, map_NO); }
  }
#endif
  Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr);
  return (*(uint32_t *)guest_to_host(addr)) & (~0u >> ((4 - len) << 3));
}

static inline void paddr_write(paddr_t addr, int len, uint32_t data) {
#ifdef HAS_IOE
  if (addr >= mmio_lbound && addr <= mmio_ubound) {
    int map_NO = is_mmio(addr);
    if (map_NO != -1) { mmio_write(addr, len, data, map_NO); return; }
  }
#endif
  Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr);
  memcpy(guest_to_host(addr), &data, len);
}

/* Slow paths (paging enabled): two-level page walk lives in memory.c. */
uint32_t vaddr_read_slow(vaddr_t addr, int len);
void vaddr_write_slow(vaddr_t addr, int len, uint32_t data);

static inline uint32_t vaddr_read(vaddr_t addr, int len) {
  if (cpu.cr0.paging) { return vaddr_read_slow(addr, len); }
  return paddr_read(addr, len);
}

static inline void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if (cpu.cr0.paging) { vaddr_write_slow(addr, len, data); return; }
  paddr_write(addr, len, data);
}

#endif
