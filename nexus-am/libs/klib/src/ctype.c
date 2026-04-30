#include <klib.h>

int toupper(int c) {
  if (c >= 'a' && c <= 'z') return c - 'a' + 'A';
  return c;
}

int tolower(int c) {
  if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
  return c;
}

static inline int is_between(int c, int lo, int hi) { return c >= lo && c <= hi; }

int isalnum(int c) { return isalpha(c) || isdigit(c); }
int isalpha(int c) { return is_between(c, 'A', 'Z') || is_between(c, 'a', 'z'); }
int iscntrl(int c) { return (c >= 0 && c < 0x20) || c == 0x7f; }
int isdigit(int c) { return is_between(c, '0', '9'); }
int isgraph(int c) { return c > 0x20 && c < 0x7f; }
int islower(int c) { return is_between(c, 'a', 'z'); }
int isprint(int c) { return c >= 0x20 && c < 0x7f; }
int ispunct(int c) { return isprint(c) && !isalnum(c) && !isspace(c); }
int isspace(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
int isupper(int c) { return is_between(c, 'A', 'Z'); }
int isxdigit(int c) { return isdigit(c) || is_between(c, 'a', 'f') || is_between(c, 'A', 'F'); }

