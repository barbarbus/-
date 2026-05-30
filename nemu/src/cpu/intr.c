#include "cpu/exec.h"
#include "memory/mmu.h"

#define FL_IF_BIT 0x200u

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  assert((NO << 3) + 7 <= cpu.idtr.limit);

  vaddr_t addr = cpu.idtr.base + (NO << 3);
  /* Gate descriptor: offset[15:0] @ +0, offset[31:16] @ +6 (Intel order). */
  uint32_t offset =
      vaddr_read(addr, 2) | (vaddr_read(addr + 6, 2) << 16);

  rtl_push(&cpu.eflags);
  rtl_push(&cpu.cs);
  rtl_push(&ret_addr);

  /* Disable further interrupts while in handler (no nesting on INTR line). */
  cpu.eflags &= ~FL_IF_BIT;

  decoding.jmp_eip = offset;
  decoding.is_jmp = 1;
}

void dev_raise_intr(void) {
  cpu.INTR = true;
}
