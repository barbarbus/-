#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

static void skip_spaces(char **args) {
  if (args == NULL || *args == NULL) return;
  while (**args == ' ') {
    (*args)++;
  }
}

static bool eval_and_print(char *expr_str, uint32_t *result) {
  bool success = true;
  *result = expr(expr_str, &success);
  if (!success) {
    printf("Bad expression: %s\n", expr_str);
    return false;
  }
  return true;
}

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_si(char *args) {
  uint64_t n = 1;
  if (args != NULL) {
    skip_spaces(&args);
    if (*args != '\0') {
      n = strtoull(args, NULL, 10);
    }
  }
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  skip_spaces(&args);
  if (args == NULL || *args == '\0') {
    printf("Usage: info r/w\n");
    return 0;
  }

  char *subcmd = strtok(args, " ");
  if (subcmd == NULL) {
    printf("Usage: info r/w\n");
    return 0;
  }

  if (strcmp(subcmd, "r") == 0) {
    for (int i = 0; i < 8; i ++) {
      printf("%-4s 0x%08x\n", regsl[i], reg_l(i));
    }
    printf("eip  0x%08x\n", cpu.eip);
  }
  else if (strcmp(subcmd, "w") == 0) {
    print_watchpoints();
  }
  else {
    printf("Unknown info subcommand '%s'\n", subcmd);
  }
  return 0;
}

static int cmd_x(char *args) {
  if (args == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  skip_spaces(&args);
  char *count_str = strtok(args, " ");
  if (count_str == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  char *expr_str = count_str + strlen(count_str) + 1;
  if (expr_str == NULL || *expr_str == '\0') {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  skip_spaces(&expr_str);

  uint32_t addr;
  if (!eval_and_print(expr_str, &addr)) {
    return 0;
  }

  uint64_t n = strtoull(count_str, NULL, 10);
  for (uint64_t i = 0; i < n; i ++) {
    printf("0x%08x: 0x%08x\n", (uint32_t)(addr + i * 4), vaddr_read(addr + i * 4, 4));
  }
  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }

  skip_spaces(&args);
  if (*args == '\0') {
    printf("Usage: p EXPR\n");
    return 0;
  }

  uint32_t result;
  if (eval_and_print(args, &result)) {
    printf("0x%x (%u)\n", result, result);
  }
  return 0;
}

static int cmd_w(char *args) {
  if (args == NULL) {
    printf("Usage: w EXPR\n");
    return 0;
  }

  skip_spaces(&args);
  if (*args == '\0') {
    printf("Usage: w EXPR\n");
    return 0;
  }

  new_wp(args);
  return 0;
}

static int cmd_d(char *args) {
  if (args == NULL) {
    printf("Usage: d N\n");
    return 0;
  }

  skip_spaces(&args);
  if (*args == '\0') {
    printf("Usage: d N\n");
    return 0;
  }

  int no = strtol(args, NULL, 10);
  free_wp(no);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "si", "Step instruction N times", cmd_si },
  { "info", "Print register or watchpoint information", cmd_info },
  { "x", "Scan memory", cmd_x },
  { "p", "Evaluate expression", cmd_p },
  { "w", "Set watchpoint", cmd_w },
  { "d", "Delete watchpoint", cmd_d },
  { "q", "Exit NEMU", cmd_q },

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  skip_spaces(&args);
  char *arg = NULL;
  if (args != NULL && *args != '\0') {
    arg = strtok(args, " ");
  }
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
