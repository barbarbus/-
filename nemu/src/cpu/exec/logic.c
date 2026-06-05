#include "cpu/exec.h"

make_EHelper(test) {
  rtl_and(&t2, &id_dest->val, &id_src->val);
  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_li(&t0, 0);
  rtl_set_CF(&t0);
  rtl_set_OF(&t0);

  print_asm_template2(test);
}

make_EHelper(and) {
  rtl_and(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_li(&t0, 0);
  rtl_set_CF(&t0);
  rtl_set_OF(&t0);

  print_asm_template2(and);
}

make_EHelper(xor) {
  rtl_xor(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_li(&t0, 0);
  rtl_set_CF(&t0);
  rtl_set_OF(&t0);

  print_asm_template2(xor);
}

make_EHelper(or) {
  rtl_or(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_li(&t0, 0);
  rtl_set_CF(&t0);
  rtl_set_OF(&t0);

  print_asm_template2(or);
}

make_EHelper(sar) {
  rtl_sari(&t2, &id_dest->val, id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(sar);
}

make_EHelper(shl) {
  rtl_shli(&t2, &id_dest->val, id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shl);
}

make_EHelper(shr) {
  rtl_shri(&t2, &id_dest->val, id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shr);
}

/* SHRD dest, src2, count:
 *   dest = (dest >> count) | (src2 << (bits - count))
 * Used by gcc to implement (int64 >> n), e.g. the fixed-point FLOAT mul
 * `((int64_t)a * b) >> 16`. Shift count is masked to 5 bits like real x86. */
make_EHelper(shrd) {
  uint32_t bits = id_dest->width * 8;
  uint32_t cnt = id_src->val & 0x1f;
  rtlreg_t res = id_dest->val;

  if (cnt != 0 && cnt < bits) {
    res = (id_dest->val >> cnt) | (id_src2->val << (bits - cnt));
  }

  rtl_li(&t2, res);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  print_asm_template3(shrd);
}

/* SHLD dest, src2, count:
 *   dest = (dest << count) | (src2 >> (bits - count)) */
make_EHelper(shld) {
  uint32_t bits = id_dest->width * 8;
  uint32_t cnt = id_src->val & 0x1f;
  rtlreg_t res = id_dest->val;

  if (cnt != 0 && cnt < bits) {
    res = (id_dest->val << cnt) | (id_src2->val >> (bits - cnt));
  }

  rtl_li(&t2, res);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  print_asm_template3(shld);
}

make_EHelper(rol) {
  uint32_t bits = id_dest->width * 8;
  uint32_t mask = (id_dest->width == 4 ? 31 : bits - 1);
  uint32_t cnt = id_src->val & mask;
  uint32_t val = id_dest->val & (~0u >> ((4 - id_dest->width) << 3));

  if (cnt != 0) {
    uint32_t res = ((val << cnt) | (val >> (bits - cnt))) & (~0u >> ((4 - id_dest->width) << 3));
    rtl_li(&t2, res);
    operand_write(id_dest, &t2);

    // CF is the least significant bit of the result.
    rtl_li(&t0, res & 1);
    rtl_set_CF(&t0);
    // OF for rotate-by-1 is MSB(result) xor CF.
    if (cnt == 1) {
      uint32_t msb = (res >> (bits - 1)) & 1;
      rtl_li(&t0, msb ^ (res & 1));
      rtl_set_OF(&t0);
    }
  }

  print_asm_template2(rol);
}

make_EHelper(ror) {
  uint32_t bits = id_dest->width * 8;
  uint32_t mask = (id_dest->width == 4 ? 31 : bits - 1);
  uint32_t cnt = id_src->val & mask;
  uint32_t val = id_dest->val & (~0u >> ((4 - id_dest->width) << 3));

  if (cnt != 0) {
    uint32_t res = ((val >> cnt) | (val << (bits - cnt))) & (~0u >> ((4 - id_dest->width) << 3));
    rtl_li(&t2, res);
    operand_write(id_dest, &t2);

    // CF is the most significant bit of the result.
    rtl_li(&t0, (res >> (bits - 1)) & 1);
    rtl_set_CF(&t0);
    // OF for rotate-by-1 is MSB(result) xor second-MSB(result).
    if (cnt == 1) {
      uint32_t msb = (res >> (bits - 1)) & 1;
      uint32_t msb2 = (res >> (bits - 2)) & 1;
      rtl_li(&t0, msb ^ msb2);
      rtl_set_OF(&t0);
    }
  }

  print_asm_template2(ror);
}

make_EHelper(setcc) {
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not) {
  rtl_not(&id_dest->val);
  operand_write(id_dest, &id_dest->val);

  print_asm_template1(not);
}
