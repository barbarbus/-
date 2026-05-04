#include "cpu/exec.h"

void diff_test_skip_qemu();
void diff_test_skip_nemu();

make_EHelper(lidt) {
  uint32_t addr = id_dest->addr;
  /* 6-byte pseudo-descriptor: limit (16), base (32), little-endian. */
  cpu.idtr.limit = vaddr_read(addr, 2);
  cpu.idtr.base = vaddr_read(addr + 2, 4);

  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  TODO();

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  TODO();

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(int) {
  uint8_t NO = id_dest->val & 0xff;
  raise_intr(NO, decoding.seq_eip);

  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  rtl_pop(&decoding.jmp_eip);
  rtl_pop(&t0);
  cpu.cs = t0 & 0xffffu;
  rtl_pop(&cpu.eflags);
  decoding.is_jmp = 1;

  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  ioaddr_t port = (ioaddr_t)id_src->val;
  uint32_t data = pio_read(port, id_dest->width);
  rtl_li(&t2, data);
  operand_write(id_dest, &t2);

  print_asm_template2(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {
  ioaddr_t port = (ioaddr_t)id_dest->val;
  pio_write(port, id_src->width, id_src->val);

  print_asm_template2(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}
