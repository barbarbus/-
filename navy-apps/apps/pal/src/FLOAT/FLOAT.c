#include "FLOAT.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

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

/* ============================================================
 * Temporary self-test (PA5-1). Call FLOAT_test() once at startup to
 * verify the binary-scaling implementation (and the NEMU SHRD/SHLD
 * instructions used by F_mul_F) without having to reach a battle.
 * Remove the call (and this block) before the final submission.
 * ============================================================ */
static int float_check(const char *name, FLOAT got, FLOAT expect, FLOAT tol) {
  FLOAT d = got - expect;
  if (d < 0) d = -d;
  int ok = (d <= tol);
  fprintf(stderr, "  %-26s = 0x%08x  expect 0x%08x  [%s]\n",
      name, (unsigned)got, (unsigned)expect, ok ? "PASS" : "FAIL");
  return ok;
}

void FLOAT_test(void) {
  int pass = 0, total = 0;

  fprintf(stderr, "==== FLOAT self-test (begin) ====\n");

#define CHK(expr, exp, tol) do {                                   \
    total++; pass += float_check(#expr, (FLOAT)(expr), (FLOAT)(exp), (FLOAT)(tol)); \
  } while (0)

  /* conversions */
  CHK(int2F(3),                    0x00030000, 0);       /* 3.0   */
  CHK(int2F(7),                    0x00070000, 0);       /* 7.0   */
  CHK(f2F(1.5),                    0x00018000, 0);       /* 1.5   */
  CHK(f2F(-1.5),                  -0x00018000, 0);       /* -1.5  */

  /* multiply -- exercises the 64-bit >>16 (SHRD) path */
  CHK(F_mul_F(f2F(1.5), f2F(1.5)), 0x00024000, 0);       /* 2.25  */
  CHK(F_mul_F(f2F(2.0), f2F(2.5)), 0x00050000, 0);       /* 5.0   */

  /* divide */
  CHK(F_div_F(int2F(7), int2F(2)), 0x00038000, 0);       /* 3.5   */
  CHK(F_div_F(int2F(1), int2F(8)), 0x00002000, 0);       /* 0.125 */

  /* int-scaled mul/div */
  CHK(F_mul_int(f2F(2.5), 3),      0x00078000, 0);       /* 7.5   */
  CHK(F_div_int(int2F(9), 2),      0x00048000, 0);       /* 4.5   */

  /* abs */
  CHK(Fabs(f2F(-1.5)),             0x00018000, 0);       /* 1.5   */

  /* sqrt / pow (Newton iteration, allow small tolerance) */
  CHK(Fsqrt(int2F(2)),             0x00016A09, 0x0300);  /* ~1.4142 */
  CHK(Fpow(int2F(8), f2F(0.333)),  0x00020000, 0x1000);  /* cbrt(8)=2 */

  /* F2int round-trip */
  total++;
  {
    int v = F2int(int2F(123));
    int ok = (v == 123);
    pass += ok;
    fprintf(stderr, "  %-26s = %d  expect %d  [%s]\n",
        "F2int(int2F(123))", v, 123, ok ? "PASS" : "FAIL");
  }

#undef CHK

  fprintf(stderr, "==== FLOAT self-test: %d/%d passed ====\n", pass, total);
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
