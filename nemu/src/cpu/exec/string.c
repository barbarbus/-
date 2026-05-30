#include "cpu/exec.h"

#define EFLAGS_DF 0x400u

make_EHelper(stosb) {
  rtl_li(&t0, cpu.eax & 0xff);
  rtl_sm(&cpu.edi, 1, &t0);
  if (cpu.eflags & EFLAGS_DF) {
    rtl_subi(&cpu.edi, &cpu.edi, 1);
  } else {
    rtl_addi(&cpu.edi, &cpu.edi, 1);
  }
  print_asm("stosb");
}

/* 0xAB: STOSW if 16-bit operand size, else STOSD */
make_EHelper(stos) {
  if (decoding.is_operand_size_16) {
    rtl_li(&t0, cpu.eax & 0xffff);
    rtl_sm(&cpu.edi, 2, &t0);
    if (cpu.eflags & EFLAGS_DF) {
      rtl_subi(&cpu.edi, &cpu.edi, 2);
    } else {
      rtl_addi(&cpu.edi, &cpu.edi, 2);
    }
    print_asm("stosw");
  } else {
    rtl_sm(&cpu.edi, 4, &cpu.eax);
    if (cpu.eflags & EFLAGS_DF) {
      rtl_subi(&cpu.edi, &cpu.edi, 4);
    } else {
      rtl_addi(&cpu.edi, &cpu.edi, 4);
    }
    print_asm("stosd");
  }
}
