#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

#define WP_EXPR_MAX 128

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

  char expr[WP_EXPR_MAX];
  uint32_t last_val;


} WP;

void init_wp_pool();
WP *new_wp(char *expr);
bool free_wp(int NO);
bool check_wp();
void print_watchpoints();

#endif
