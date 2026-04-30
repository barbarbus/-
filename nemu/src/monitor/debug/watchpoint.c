#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#include <stdio.h>
#include <string.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

static bool eval_watch_expr(const char *expr_str, uint32_t *value) {
  bool success = true;
  char buf[WP_EXPR_MAX];
  strncpy(buf, expr_str, WP_EXPR_MAX - 1);
  buf[WP_EXPR_MAX - 1] = '\0';
  *value = expr(buf, &success);
  return success;
}

WP *new_wp(char *expr_str) {
  if (free_ == NULL) {
    printf("No free watchpoint!\n");
    return NULL;
  }

  WP *wp = free_;
  free_ = free_->next;

  wp->next = head;
  head = wp;

  strncpy(wp->expr, expr_str, WP_EXPR_MAX - 1);
  wp->expr[WP_EXPR_MAX - 1] = '\0';

  if (!eval_watch_expr(wp->expr, &wp->last_val)) {
    printf("Bad expression: %s\n", wp->expr);
    head = wp->next;
    wp->next = free_;
    free_ = wp;
    return NULL;
  }

  printf("Watchpoint %d: %s = 0x%x\n", wp->NO, wp->expr, wp->last_val);
  return wp;
}

bool free_wp(int NO) {
  WP *prev = NULL;
  WP *curr = head;

  while (curr != NULL && curr->NO != NO) {
    prev = curr;
    curr = curr->next;
  }

  if (curr == NULL) {
    printf("No watchpoint %d\n", NO);
    return false;
  }

  if (prev == NULL) {
    head = curr->next;
  }
  else {
    prev->next = curr->next;
  }

  curr->next = free_;
  free_ = curr;
  printf("Watchpoint %d deleted\n", NO);
  return true;
}

bool check_wp() {
  WP *curr = head;

  while (curr != NULL) {
    uint32_t new_val;
    if (!eval_watch_expr(curr->expr, &new_val)) {
      printf("Watchpoint %d expression error: %s\n", curr->NO, curr->expr);
      return true;
    }

    if (new_val != curr->last_val) {
      printf("Watchpoint %d triggered: %s\n", curr->NO, curr->expr);
      printf("Old value = 0x%x, New value = 0x%x\n", curr->last_val, new_val);
      curr->last_val = new_val;
      return true;
    }
    curr = curr->next;
  }

  return false;
}

void print_watchpoints() {
  WP *curr = head;

  if (curr == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  while (curr != NULL) {
    printf("Watchpoint %d: %s = 0x%x\n", curr->NO, curr->expr, curr->last_val);
    curr = curr->next;
  }
}


