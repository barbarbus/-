#include <x86.h>
#include <string.h>

#define PG_ALIGN __attribute((aligned(PGSIZE)))

static PDE kpdirs[NR_PDE] PG_ALIGN;
static PTE kptabs[PMEM_SIZE / PGSIZE] PG_ALIGN;
static void* (*palloc_f)();
static void (*pfree_f)(void*);

_Area segments[] = {      // Kernel memory mappings
  {.start = (void*)0,          .end = (void*)PMEM_SIZE}
};

#define NR_KSEG_MAP (sizeof(segments) / sizeof(segments[0]))

void _pte_init(void* (*palloc)(), void (*pfree)(void*)) {
  palloc_f = palloc;
  pfree_f = pfree;

  int i;

  // make all PDEs invalid
  for (i = 0; i < NR_PDE; i ++) {
    kpdirs[i] = 0;
  }

  PTE *ptab = kptabs;
  for (i = 0; i < NR_KSEG_MAP; i ++) {
    uint32_t pdir_idx = (uintptr_t)segments[i].start / (PGSIZE * NR_PTE);
    uint32_t pdir_idx_end = (uintptr_t)segments[i].end / (PGSIZE * NR_PTE);
    for (; pdir_idx < pdir_idx_end; pdir_idx ++) {
      /*
       * NEMU page_translate() treats cs!=0x8 as user and requires U/S=1 on
       * both PDE and PTE. Identity-map the kernel with U+W so user-mode
       * accesses to kernel-linked addresses (e.g. pcb stack VA) do not fail
       * the assert. Standard ICS PA simplification.
       */
      kpdirs[pdir_idx] = (uintptr_t)ptab | PTE_P | PTE_W | PTE_U;

      // fill PTE
      PTE pte = PGADDR(pdir_idx, 0, 0) | PTE_P | PTE_W | PTE_U;
      PTE pte_end = PGADDR(pdir_idx + 1, 0, 0) | PTE_P | PTE_W | PTE_U;
      for (; pte < pte_end; pte += PGSIZE) {
        *ptab = pte;
        ptab ++;
      }
    }
  }

  set_cr3(kpdirs);
  set_cr0(get_cr0() | CR0_PG);
}

void _protect(_Protect *p) {
  PDE *updir = (PDE *)(palloc_f());
  memset(updir, 0, PGSIZE);
  p->ptr = updir;

  /* Deep-copy kernel page tables so per-process PTE edits (user image @ LOAD_BASE)
   * do not alias: shallow `updir[i]=kpdirs[i]` made all PCBs share the same PT
   * pages, so the last loader wiped earlier programs' mappings. */
  for (int i = 0; i < NR_PDE; i++) {
    PDE k = kpdirs[i];
    if ((k & PTE_P) == 0) {
      updir[i] = 0;
      continue;
    }
    PTE *src = (PTE *)PTE_ADDR(k);
    PTE *dst = (PTE *)palloc_f();
    memcpy(dst, src, PGSIZE);
    updir[i] = ((uintptr_t)dst & ~0xfffu) | (k & 0xfffu);
  }

  p->area.start = (void *)0x8000000;
  p->area.end = (void *)0xc0000000;
}

void _release(_Protect *p) {
}

void _switch(_Protect *p) {
  set_cr3(p->ptr);
}

void _map(_Protect *p, void *va, void *pa) {
  PDE *pdir = (PDE *)p->ptr;
  uint32_t pdx = PDX(va);
  uint32_t ptx = PTX(va);

  PDE pde = pdir[pdx];
  PTE *ptab;
  if ((pde & PTE_P) == 0) {
    ptab = (PTE *)palloc_f();
    memset(ptab, 0, PGSIZE);
    pdir[pdx] = (uintptr_t)ptab | PTE_P | PTE_W | PTE_U;
  } else {
    ptab = (PTE *)PTE_ADDR(pde);
  }

  ptab[ptx] = ((uintptr_t)pa & ~0xfffu) | PTE_P | PTE_W | PTE_U;
  /* User-mode fetch/load needs U/S on the page directory entry too. */
  pdir[pdx] |= PTE_U;
}

void _unmap(_Protect *p, void *va) {
  PDE *pdir = (PDE *)p->ptr;
  uint32_t pdx = PDX(va);
  uint32_t ptx = PTX(va);

  PDE pde = pdir[pdx];
  if ((pde & PTE_P) == 0) return;
  PTE *ptab = (PTE *)PTE_ADDR(pde);
  ptab[ptx] = 0;
}

_RegSet *_umake(_Protect *p, _Area ustack, _Area kstack, void *entry, char *const argv[], char *const envp[]) {
  (void)p;
  (void)argv;
  (void)envp;

  /* Trap frame on kernel stack (pcb stack); user GPR esp points into mapped user stack. */
  uintptr_t sp = (uintptr_t)kstack.end;
  sp -= sizeof(_RegSet);
  sp &= ~0xfu;

  _RegSet *tf = (_RegSet *)sp;
  memset(tf, 0, sizeof(*tf));

  tf->eip = (uintptr_t)entry;
  tf->cs = USEL(SEG_UCODE);
  tf->eflags = 0x2 | FL_IF;
  tf->irq = (uintptr_t)-1;
  tf->err = 0;
  tf->esp = (uintptr_t)ustack.end - 16;
  return tf;
}
