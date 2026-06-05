#include "FLOAT.h"
#include <stdint.h>
#include <string.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  return (FLOAT)(((int64_t)a * b) >> 16);
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  /* Avoid int64 division: x86-nemu has no __divdi3 in the link set. */
  int sign = 1;
  if (a < 0) { a = -a; sign = -sign; }
  if (b < 0) { b = -b; sign = -sign; }
  if (b == 0) {
    return 0;
  }

  uint32_t x = (uint32_t)a;
  uint32_t y = (uint32_t)b;
  uint32_t int_part = x / y;
  uint32_t rem = x % y;
  uint32_t frac_part = 0;

  for (int i = 0; i < 16; i++) {
    rem <<= 1;
    if (rem >= y) {
      rem -= y;
      frac_part = (frac_part << 1) | 1;
    } else {
      frac_part <<= 1;
    }
  }

  return sign * (FLOAT)((int_part << 16) + frac_part);
}

FLOAT f2F(float a) {
  union {
    float f;
    uint32_t u;
  } uf;
  uf.f = a;

  uint32_t bits = uf.u;
  uint32_t sign = bits >> 31;
  int exp = (int)((bits >> 23) & 0xff) - 127;
  uint32_t frac = bits & 0x7fffffu;

  if (exp == 128) {
    return 0;
  }

  int64_t val;
  if (exp < 0) {
    if (exp < -16) {
      return 0;
    }
    val = (int64_t)frac >> (23 - exp - 16);
  } else {
    frac |= 0x800000u;
    int sh = exp + 16 - 23;
    if (sh >= 0) {
      val = (int64_t)frac << sh;
    } else {
      val = (int64_t)frac >> (-sh);
    }
  }

  if (sign) {
    val = -val;
  }
  return (FLOAT)val;
}

FLOAT Fabs(FLOAT a) {
  return a < 0 ? -a : a;
}

/* Functions below are already implemented */

FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while (Fabs(dt) > f2F(1e-4f));

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = F_div_int((F_div_F(x, t2) - t), 3);
    t += dt;
  } while (Fabs(dt) > f2F(1e-4f));

  return t;
}
