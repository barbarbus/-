#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NEQ, TK_AND, TK_OR,
  TK_NUM, TK_HEX, TK_REG, TK_DEREF, TK_NEG, TK_NOT,
  TK_LPAREN, TK_RPAREN

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},
  {"0[xX][0-9a-fA-F]+", TK_HEX},
  {"[0-9]+", TK_NUM},
  {"\\$?[a-zA-Z][a-zA-Z0-9]*", TK_REG},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR},
  {"==", TK_EQ},
  {"!=", TK_NEQ},
  {"\\+", '+'},
  {"-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"!", TK_NOT},
  {"\\(", TK_LPAREN},
  {"\\)", TK_RPAREN}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[64];
} Token;

static Token tokens[256];
int nr_token;

static bool is_binary_op(int type) {
  return type == TK_EQ || type == TK_NEQ || type == TK_AND || type == TK_OR ||
         type == '+' || type == '-' || type == '*' || type == '/';
}

static bool is_unary_op(int type) {
  return type == TK_DEREF || type == TK_NEG || type == TK_NOT;
}

static bool is_left_paren(int type) {
  return type == TK_LPAREN;
}

static bool is_right_paren(int type) {
  return type == TK_RPAREN;
}

static bool is_unary_candidate(int prev_type) {
  return prev_type == TK_NOTYPE || is_binary_op(prev_type) ||
         is_left_paren(prev_type) || is_unary_op(prev_type);
}

static uint32_t get_reg_value(const char *s, bool *success) {
  const char *name = (s[0] == '$') ? s + 1 : s;

  if (strcmp(name, "eip") == 0) {
    return cpu.eip;
  }

  for (int i = 0; i < 8; i ++) {
    if (strcmp(name, regsl[i]) == 0) return reg_l(i);
    if (strcmp(name, regsw[i]) == 0) return reg_w(i);
  }
  for (int i = 0; i < 8; i ++) {
    if (strcmp(name, regsb[i]) == 0) return reg_b(i);
  }

  *success = false;
  return 0;
}

static int token_precedence(int type) {
  switch (type) {
    case TK_OR: return 1;
    case TK_AND: return 2;
    case TK_EQ:
    case TK_NEQ: return 3;
    case '+':
    case '-': return 4;
    case '*':
    case '/': return 5;
    default: return -1;
  }
}

static bool check_parentheses(int p, int q) {
  if (!is_left_paren(tokens[p].type) || !is_right_paren(tokens[q].type)) {
    return false;
  }

  int depth = 0;
  for (int i = p; i <= q; i ++) {
    if (is_left_paren(tokens[i].type)) {
      depth ++;
    }
    else if (is_right_paren(tokens[i].type)) {
      depth --;
      if (depth == 0 && i < q) {
        return false;
      }
    }
  }

  return depth == 0;
}

static int find_dominant_op(int p, int q) {
  int op = -1;
  int min_prec = 100;
  int depth = 0;

  for (int i = p; i <= q; i ++) {
    int type = tokens[i].type;

    if (is_left_paren(type)) {
      depth ++;
      continue;
    }
    if (is_right_paren(type)) {
      depth --;
      continue;
    }
    if (depth != 0) {
      continue;
    }

    if (!is_binary_op(type)) {
      continue;
    }

    int prec = token_precedence(type);
    if (prec < min_prec) {
      min_prec = prec;
      op = i;
    }
  }

  return op;
}

static uint32_t eval(int p, int q, bool *success);

static uint32_t eval_unary(int type, uint32_t val, bool *success) {
  switch (type) {
    case TK_NEG: return (uint32_t)(-(int32_t)val);
    case TK_NOT: return !val;
    case TK_DEREF: return vaddr_read(val, 4);
    default: *success = false; return 0;
  }
}

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_NUM:
          case TK_HEX:
          case TK_REG:
            Assert(nr_token < 256, "too many tokens");
            Assert(substr_len < (int)sizeof(tokens[nr_token].str), "token too long");
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            tokens[nr_token].type = rules[i].token_type;
            nr_token ++;
            break;
          default:
            Assert(nr_token < 256, "too many tokens");
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[0] = substr_start[0];
            tokens[nr_token].str[1] = '\0';
            nr_token ++;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

      for (int i = 0; i < nr_token; i ++) {
        if (tokens[i].type == '*' && (i == 0 || is_unary_candidate(tokens[i - 1].type))) {
          tokens[i].type = TK_DEREF;
        }
        else if (tokens[i].type == '-' && (i == 0 || is_unary_candidate(tokens[i - 1].type))) {
          tokens[i].type = TK_NEG;
        }
        else if (tokens[i].type == TK_NOT && (i == 0 || is_unary_candidate(tokens[i - 1].type))) {
          tokens[i].type = TK_NOT;
        }
      }

  return true;
}

    static uint32_t eval(int p, int q, bool *success) {
      if (!*success) return 0;

      if (p > q) {
        *success = false;
        return 0;
      }

      if (p == q) {
        switch (tokens[p].type) {
          case TK_NUM:
            return strtoul(tokens[p].str, NULL, 10);
          case TK_HEX:
            return strtoul(tokens[p].str, NULL, 0);
          case TK_REG:
            return get_reg_value(tokens[p].str, success);
          default:
            *success = false;
            return 0;
        }
      }

      if (check_parentheses(p, q)) {
        return eval(p + 1, q - 1, success);
      }

      int op = find_dominant_op(p, q);
      if (op < 0) {
        if (tokens[p].type == TK_NEG || tokens[p].type == TK_NOT || tokens[p].type == TK_DEREF) {
          return eval_unary(tokens[p].type, eval(p + 1, q, success), success);
        }
        *success = false;
        return 0;
      }

      uint32_t val1 = eval(p, op - 1, success);
      uint32_t val2 = eval(op + 1, q, success);
      if (!*success) return 0;

      switch (tokens[op].type) {
        case '+': return val1 + val2;
        case '-': return val1 - val2;
        case '*': return val1 * val2;
        case '/':
          if (val2 == 0) {
            *success = false;
            return 0;
          }
          return val1 / val2;
        case TK_EQ: return val1 == val2;
        case TK_NEQ: return val1 != val2;
        case TK_AND: return val1 && val2;
        case TK_OR: return val1 || val2;
        default:
          *success = false;
          return 0;
      }
    }

uint32_t expr(char *e, bool *success) {
      *success = true;
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

      if (nr_token == 0) {
        *success = false;
        return 0;
      }

      return eval(0, nr_token - 1, success);
}
