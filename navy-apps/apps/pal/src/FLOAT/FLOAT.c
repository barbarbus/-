#include "FLOAT.h"
#include <stdint.h>
#include <string.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  return (FLOAT)(((int64_t)a * b) >> 16);
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  return (FLOAT)(((int64_t)a << 16) / b);
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
