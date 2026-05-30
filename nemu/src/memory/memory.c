#include "nemu.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#ifdef HAS_IOE
#include "device/mmio.h"
#endif

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
#ifdef HAS_IOE
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    return mmio_read(addr, len, map_NO);
  }
#endif
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
#ifdef HAS_IOE
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    mmio_write(addr, len, data, map_NO);
    return;
  }
#endif
  memcpy(guest_to_host(addr), &data, len);
}

static inline paddr_t page_translate(vaddr_t addr, int len, bool is_write) {
  /* i386 paging: 4KB pages, 2-level page tables */
  uint32_t dir = (addr >> 22) & 0x3ffu;
  uint32_t page = (addr >> 12) & 0x3ffu;
  uint32_t off = addr & PAGE_MASK;

  /* Cross-page access is not supported in this PA. */
  assert(off + (uint32_t)len <= PAGE_SIZE);

  paddr_t pgdir_base = (paddr_t)(cpu.cr3.page_directory_base << 12);
  PDE pde;
  pde.val = paddr_read(pgdir_base + dir * 4, 4);
  assert(pde.present);

  PTE pte;
  paddr_t pgtab_base = (paddr_t)(pde.page_frame << 12);
  pte.val = paddr_read(pgtab_base + page * 4, 4);
  assert(pte.present);

  /* Permission checks (simplified): if in user mode, require U/S=1 on both.
   * This NEMU does not model CPL; treat cs==0x8 as kernel, others as user. */
  bool user = (cpu.cs != 0x8);
  if (user) {
    assert(pde.user_supervisor && pte.user_supervisor);
    if (is_write) {
      assert(pde.read_write && pte.read_write);
    }
  }

  paddr_t pa = (paddr_t)(pte.page_frame << 12) | off;
  return pa;
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if (cpu.cr0.paging) {
    uint32_t off = addr & PAGE_MASK;
    if (off + (uint32_t)len <= PAGE_SIZE) {
      paddr_t pa = page_translate(addr, len, false);
      return paddr_read(pa, len);
    }

    /* Cross-page access: split into two reads. */
    int len1 = (int)(PAGE_SIZE - off);
    int len2 = len - len1;
    paddr_t pa1 = page_translate(addr, len1, false);
    paddr_t pa2 = page_translate(addr + (vaddr_t)len1, len2, false);
    uint32_t low = paddr_read(pa1, len1);
    uint32_t high = paddr_read(pa2, len2);
    return low | (high << (len1 * 8));
  }
  return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if (cpu.cr0.paging) {
    uint32_t off = addr & PAGE_MASK;
    if (off + (uint32_t)len <= PAGE_SIZE) {
      paddr_t pa = page_translate(addr, len, true);
      paddr_write(pa, len, data);
      return;
    }

    /* Cross-page access: split into two writes. */
    int len1 = (int)(PAGE_SIZE - off);
    int len2 = len - len1;
    paddr_t pa1 = page_translate(addr, len1, true);
    paddr_t pa2 = page_translate(addr + (vaddr_t)len1, len2, true);
    uint32_t mask1 = (len1 == 4 ? 0xffffffffu : ((1u << (len1 * 8)) - 1u));
    uint32_t data1 = data & mask1;
    uint32_t data2 = data >> (len1 * 8);
    paddr_write(pa1, len1, data1);
    paddr_write(pa2, len2, data2);
    return;
  }
  paddr_write(addr, len, data);
}
