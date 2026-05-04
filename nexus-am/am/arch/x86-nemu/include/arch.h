#ifndef __ARCH_H__
#define __ARCH_H__

#include <am.h>

#define PMEM_SIZE (128 * 1024 * 1024)
#define PGSIZE    4096    // Bytes mapped by a page

struct _RegSet {
  uintptr_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uintptr_t irq;
  uintptr_t err;
};

#define SYSCALL_ARG1(r) (((struct _RegSet *)(r))->eax)
#define SYSCALL_ARG2(r) (((struct _RegSet *)(r))->ebx)
#define SYSCALL_ARG3(r) (((struct _RegSet *)(r))->ecx)
#define SYSCALL_ARG4(r) (((struct _RegSet *)(r))->edx)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif
