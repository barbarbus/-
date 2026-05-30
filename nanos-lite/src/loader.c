#include "common.h"
#include "fs.h"
#include "memory.h"

#define EI_NIDENT 16
#define PT_LOAD 1
#define ET_EXEC 2
#define EM_386 3
#define LOAD_BASE 0x04000000u

#define MAX_LOAD_PAGES 512

typedef struct {
  uint8_t e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
} Elf32_Phdr;

static inline uint32_t roundup(uint32_t x, uint32_t a) {
  return (x + a - 1) & ~(a - 1);
}

static inline uint32_t rounddown(uint32_t x, uint32_t a) {
  return x & ~(a - 1);
}

static inline int looks_like_elf32(const Elf32_Ehdr *eh) {
  return eh->e_ident[0] == 0x7f && eh->e_ident[1] == 'E' &&
         eh->e_ident[2] == 'L' && eh->e_ident[3] == 'F' &&
         eh->e_ident[4] == 1 && eh->e_ident[5] == 1 &&
         eh->e_machine == EM_386 && eh->e_type == ET_EXEC &&
         eh->e_phoff != 0 && eh->e_phnum != 0 &&
         eh->e_phentsize == sizeof(Elf32_Phdr);
}

/*
 * Map [va0, va1) with new physical pages; fill from file offset 0..filesz into
 * LOAD_BASE+... (flat) using the per-VA page list — never memcpy() to user VA
 * while CR3 is kernel: user PTEs may point at different frames than identity map.
 */
static uintptr_t load_flat_binary(_Protect *as, int fd, uintptr_t *p_brk) {
  size_t filesz = fs_filesz(fd);
  fs_lseek(fd, 0, SEEK_SET);
  assert(filesz > 0);
  uint32_t va0 = rounddown(LOAD_BASE, PGSIZE);
  uint32_t va1 = roundup(LOAD_BASE + filesz, PGSIZE);
  int np = 0;
  void *pages[MAX_LOAD_PAGES];

  assert((va1 - va0) / PGSIZE <= MAX_LOAD_PAGES);
  for (uint32_t va = va0; va < va1; va += PGSIZE) {
    void *pa = new_page();
    pages[np++] = pa;
    _map(as, (void *)(uintptr_t)va, pa);
    memset(pa, 0, PGSIZE);
  }

  uint32_t off = 0;
  while (off < filesz) {
    uint8_t buf[512];
    uint32_t chunk = off + sizeof(buf) <= filesz ? sizeof(buf) : filesz - off;
    fs_lseek(fd, off, SEEK_SET);
    size_t n = fs_read(fd, buf, chunk);
    assert(n == chunk);

    uint32_t v = LOAD_BASE + off;
    uint32_t page_base = rounddown(v, PGSIZE);
    int pg = (int)((page_base - va0) / PGSIZE);
    uint32_t inpage = v - page_base;
    memcpy((uint8_t *)pages[pg] + inpage, buf, chunk);
    off += chunk;
  }

  if (p_brk) {
    *p_brk = (uintptr_t)LOAD_BASE + (uintptr_t)filesz;
  }
  return LOAD_BASE;
}

static void map_phdr(_Protect *as, int fd, Elf32_Phdr *ph, void **pages, int *npage,
                     int maxpage) {
  uint32_t va0 = rounddown(ph->p_vaddr, PGSIZE);
  uint32_t va1 = roundup(ph->p_vaddr + ph->p_memsz, PGSIZE);
  int need = (int)((va1 - va0) / PGSIZE);
  assert(*npage + need <= maxpage);

  int base = *npage;
  for (uint32_t va = va0; va < va1; va += PGSIZE) {
    void *pa = new_page();
    pages[(*npage)++] = pa;
    _map(as, (void *)(uintptr_t)va, pa);
    memset(pa, 0, PGSIZE);
  }

  uint32_t off = 0;
  while (off < ph->p_filesz) {
    uint8_t buf[512];
    uint32_t chunk = off + sizeof(buf) <= ph->p_filesz ? sizeof(buf) : ph->p_filesz - off;
    fs_lseek(fd, ph->p_offset + off, SEEK_SET);
    size_t n = fs_read(fd, buf, chunk);
    assert(n == chunk);

    uint32_t v = ph->p_vaddr + off;
    uint32_t page_base = rounddown(v, PGSIZE);
    int pg = base + (int)((page_base - va0) / PGSIZE);
    uint32_t inpage = v - page_base;
    memcpy((uint8_t *)pages[pg] + inpage, buf, chunk);
    off += chunk;
  }
}

static uintptr_t loader_elf(_Protect *as, int fd, Elf32_Ehdr *eh, uintptr_t *p_brk) {
  void *pages[MAX_LOAD_PAGES];
  int npage = 0;
  uint32_t max_end = 0;

  for (int i = 0; i < eh->e_phnum; i++) {
    Elf32_Phdr ph;
    fs_lseek(fd, eh->e_phoff + (off_t)i * sizeof(Elf32_Phdr), SEEK_SET);
    size_t n = fs_read(fd, &ph, sizeof(ph));
    assert(n == sizeof(ph));
    if (ph.p_type != PT_LOAD) {
      continue;
    }
    map_phdr(as, fd, &ph, pages, &npage, MAX_LOAD_PAGES);
    uint32_t end = ph.p_vaddr + ph.p_memsz;
    if (end > max_end) {
      max_end = end;
    }
  }

  if (p_brk) {
    *p_brk = (uintptr_t)max_end;
  }
  return eh->e_entry;
}

uintptr_t loader(_Protect *as, const char *filename, uintptr_t *p_brk) {
  int fd = fs_open(filename, 0, 0);
  assert(fd >= 0);

  Elf32_Ehdr eh;
  memset(&eh, 0, sizeof(eh));
  fs_lseek(fd, 0, SEEK_SET);
  size_t n = fs_read(fd, &eh, sizeof(eh));

  uintptr_t entry;
  if (n != sizeof(eh) || !looks_like_elf32(&eh)) {
    entry = load_flat_binary(as, fd, p_brk);
  } else {
    entry = loader_elf(as, fd, &eh, p_brk);
  }

  fs_close(fd);
  return entry;
}
