#include "cpu/exec.h"

make_EHelper(mov) {
  operand_write(id_dest, &id_src->val);
  print_asm_template2(mov);
}

make_EHelper(push) {
  // All decode helpers for `push` in this PA fill `id_dest`.
  rtl_push(&id_dest->val);

  print_asm_template1(push);
}

make_EHelper(pop) {
  rtl_pop(&t2);
  operand_write(id_dest, &t2);

  print_asm_template1(pop);
}

make_EHelper(pusha) {
  rtlreg_t temp_esp = cpu.esp;
  rtl_push(&cpu.eax);
  rtl_push(&cpu.ecx);
  rtl_push(&cpu.edx);
  rtl_push(&cpu.ebx);
  rtl_push(&temp_esp);
  rtl_push(&cpu.ebp);
  rtl_push(&cpu.esi);
  rtl_push(&cpu.edi);

  print_asm("pusha");
}

make_EHelper(popa) {
  rtl_pop(&cpu.edi);
  rtl_pop(&cpu.esi);
  rtl_pop(&cpu.ebp);
  // POPA semantics: discard the saved ESP slot, do NOT load it into ESP.
  rtl_addi(&cpu.esp, &cpu.esp, 4);
  rtl_pop(&cpu.ebx);
  rtl_pop(&cpu.edx);
  rtl_pop(&cpu.ecx);
  rtl_pop(&cpu.eax);

  print_asm("popa");
}

make_EHelper(leave) {
  // mov esp, ebp; pop ebp
  rtlreg_t new_esp = cpu.ebp;
  rtl_lm(&cpu.ebp, &new_esp, 4);
  cpu.esp = new_esp + 4;

  print_asm("leave");
}

make_EHelper(cltd) {
  if (decoding.is_operand_size_16) {
    // CWD: extend AX to DX:AX
    rtlreg_t t;
    rtl_sext(&t, &cpu.eax, 2);
    cpu.edx = t >> 16;
  }
  else {
    // CDQ: extend EAX to EDX:EAX
    if ((cpu.eax & 0x80000000) != 0) {
      cpu.edx = 0xFFFFFFFF;
    } else {
      cpu.edx = 0;
    }
  }

  print_asm(decoding.is_operand_size_16 ? "cwd" : "cdq");
}

make_EHelper(cwtl) {
  if (decoding.is_operand_size_16) {
    // CBW: extend AL to AX
    rtl_sext(&cpu.eax, &cpu.eax, 1);
  }
  else {
    // CWDE: extend AX to EAX
    rtl_sext(&cpu.eax, &cpu.eax, 2);
  }

  print_asm(decoding.is_operand_size_16 ? "cbw" : "cwde");
}

make_EHelper(movsx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  rtl_sext(&t2, &id_src->val, id_src->width);
  operand_write(id_dest, &t2);
  print_asm_template2(movsx);
}

make_EHelper(movzx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  operand_write(id_dest, &id_src->val);
  print_asm_template2(movzx);
}

make_EHelper(lea) {
  rtl_li(&t2, id_src->addr);
  operand_write(id_dest, &t2);
  print_asm_template2(lea);
}
