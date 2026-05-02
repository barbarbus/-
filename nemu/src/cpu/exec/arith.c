#include "cpu/exec.h"

make_EHelper(add) {
  rtl_add(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  // CF: carry out of the most significant bit (unsigned overflow)
  rtl_sltu(&t0, &t2, &id_dest->val);
  rtl_set_CF(&t0);

  // OF: signed overflow
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_not(&t0);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(add);
}

make_EHelper(sub) {
  rtl_sub(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  // CF: borrow (unsigned underflow) => dest < src
  rtl_sltu(&t0, &id_dest->val, &id_src->val);
  rtl_set_CF(&t0);

  // OF: signed overflow for subtraction
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(sub);
}

make_EHelper(cmp) {
  rtl_sub(&t2, &id_dest->val, &id_src->val);
  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_sltu(&t0, &id_dest->val, &id_src->val);
  rtl_set_CF(&t0);

  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(cmp);
}

make_EHelper(inc) {
  rtlreg_t one = 1;
  rtl_add(&t2, &id_dest->val, &one);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  print_asm_template1(inc);
}

make_EHelper(dec) {
  rtlreg_t one = 1;
  rtl_sub(&t2, &id_dest->val, &one);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  print_asm_template1(dec);
}

make_EHelper(neg) {
  rtlreg_t zero = 0;
  rtl_sub(&t2, &zero, &id_dest->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  print_asm_template1(neg);
}

make_EHelper(adc) {
  rtl_add(&t2, &id_dest->val, &id_src->val);
  rtl_sltu(&t3, &t2, &id_dest->val);
  rtl_get_CF(&t1);
  rtl_add(&t2, &t2, &t1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_sltu(&t0, &t2, &id_dest->val);
  rtl_or(&t0, &t3, &t0);
  rtl_set_CF(&t0);

  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_not(&t0);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(adc);
}

make_EHelper(sbb) {
  rtl_sub(&t2, &id_dest->val, &id_src->val);
  rtl_sltu(&t3, &id_dest->val, &t2);
  rtl_get_CF(&t1);
  rtl_sub(&t2, &t2, &t1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_sltu(&t0, &id_dest->val, &t2);
  rtl_or(&t0, &t3, &t0);
  rtl_set_CF(&t0);

  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(sbb);
}

make_EHelper(mul) {
  rtl_lr(&t0, R_EAX, id_dest->width);
  rtl_mul(&t0, &t1, &id_dest->val, &t0);

  switch (id_dest->width) {
    case 1:
      rtl_sr_w(R_AX, &t1);
      break;
    case 2:
      rtl_sr_w(R_AX, &t1);
      rtl_shri(&t1, &t1, 16);
      rtl_sr_w(R_DX, &t1);
      break;
    case 4:
      rtl_sr_l(R_EDX, &t0);
      rtl_sr_l(R_EAX, &t1);
      break;
    default: assert(0);
  }

  print_asm_template1(mul);
}

// imul with one operand
make_EHelper(imul1) {
  rtl_lr(&t0, R_EAX, id_dest->width);
  rtl_imul(&t0, &t1, &id_dest->val, &t0);

  switch (id_dest->width) {
    case 1:
      rtl_sr_w(R_AX, &t1);
      break;
    case 2:
      rtl_sr_w(R_AX, &t1);
      rtl_shri(&t1, &t1, 16);
      rtl_sr_w(R_DX, &t1);
      break;
    case 4:
      rtl_sr_l(R_EDX, &t0);
      rtl_sr_l(R_EAX, &t1);
      break;
    default: assert(0);
  }

  print_asm_template1(imul);
}

// imul with two operands
make_EHelper(imul2) {
  rtl_sext(&id_src->val, &id_src->val, id_src->width);
  rtl_sext(&id_dest->val, &id_dest->val, id_dest->width);

  rtl_imul(&t0, &t1, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t1);

  print_asm_template2(imul);
}

// imul with three operands
make_EHelper(imul3) {
  rtl_sext(&id_src->val, &id_src->val, id_src->width);
  rtl_sext(&id_src2->val, &id_src2->val, id_src->width);
  rtl_sext(&id_dest->val, &id_dest->val, id_dest->width);

  rtl_imul(&t0, &t1, &id_src2->val, &id_src->val);
  operand_write(id_dest, &t1);

  print_asm_template3(imul);
}

make_EHelper(div) {
  switch (id_dest->width) {
    case 1: {
      uint16_t dividend = reg_w(R_AX);
      uint8_t divisor = (uint8_t)id_dest->val;
      assert(divisor != 0);
      uint16_t q = dividend / divisor;
      uint16_t r = dividend % divisor;
      rtl_li(&t2, q);
      rtl_li(&t3, r);
      rtl_sr(R_EAX, 1, &t2);   // AL
      rtl_sr_b(R_AH, &t3);     // AH
      break;
    }
    case 2: {
      uint32_t dividend = ((uint32_t)reg_w(R_DX) << 16) | reg_w(R_AX);
      uint16_t divisor = (uint16_t)id_dest->val;
      assert(divisor != 0);
      uint32_t q = dividend / divisor;
      uint32_t r = dividend % divisor;
      rtl_li(&t2, q);
      rtl_li(&t3, r);
      rtl_sr(R_EAX, 2, &t2);   // AX
      rtl_sr(R_EDX, 2, &t3);   // DX
      break;
    }
    case 4: {
      uint64_t dividend = ((uint64_t)cpu.edx << 32) | cpu.eax;
      uint32_t divisor = (uint32_t)id_dest->val;
      assert(divisor != 0);
      uint32_t q = (uint32_t)(dividend / divisor);
      uint32_t r = (uint32_t)(dividend % divisor);
      rtl_li(&t2, q);
      rtl_li(&t3, r);
      rtl_sr(R_EAX, 4, &t2);   // EAX
      rtl_sr(R_EDX, 4, &t3);   // EDX
      break;
    }
    default: assert(0);
  }

  print_asm_template1(div);
}

make_EHelper(idiv) {
  switch (id_dest->width) {
    case 1: {
      int16_t dividend = (int16_t)reg_w(R_AX);
      int8_t divisor = (int8_t)id_dest->val;
      assert(divisor != 0);
      int16_t q = dividend / divisor;
      int16_t r = dividend % divisor;
      rtl_li(&t2, (uint8_t)q);
      rtl_li(&t3, (uint8_t)r);
      rtl_sr(R_EAX, 1, &t2);   // AL
      rtl_sr_b(R_AH, &t3);     // AH
      break;
    }
    case 2: {
      int32_t dividend = ((int32_t)(int16_t)reg_w(R_DX) << 16) | (uint16_t)reg_w(R_AX);
      int16_t divisor = (int16_t)id_dest->val;
      assert(divisor != 0);
      int32_t q = dividend / divisor;
      int32_t r = dividend % divisor;
      rtl_li(&t2, (uint16_t)q);
      rtl_li(&t3, (uint16_t)r);
      rtl_sr(R_EAX, 2, &t2);   // AX
      rtl_sr(R_EDX, 2, &t3);   // DX
      break;
    }
    case 4: {
      int64_t dividend = ((int64_t)(int32_t)cpu.edx << 32) | (uint32_t)cpu.eax;
      int32_t divisor = (int32_t)id_dest->val;
      assert(divisor != 0);
      int32_t q = (int32_t)(dividend / divisor);
      int32_t r = (int32_t)(dividend % divisor);
      rtl_li(&t2, (uint32_t)q);
      rtl_li(&t3, (uint32_t)r);
      rtl_sr(R_EAX, 4, &t2);   // EAX
      rtl_sr(R_EDX, 4, &t3);   // EDX
      break;
    }
    default: assert(0);
  }

  print_asm_template1(idiv);
}
