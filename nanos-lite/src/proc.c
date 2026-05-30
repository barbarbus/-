#include "proc.h"
#include "memory.h"

#define MAX_NR_PROC 4
#define USER_STACK_TOP 0xc0000000u

/* pcb[0]=pal, pcb[1]=hello, pcb[2]=videotest；current_game 与 hello 分时 */
#define SCHED_GAME_SLICES_BEFORE_HELLO 3

static PCB pcb[MAX_NR_PROC];
static int game_slices_left = SCHED_GAME_SLICES_BEFORE_HELLO - 1;
static int nr_proc = 0;
PCB *current = NULL;
static PCB *current_game = NULL;

uintptr_t loader(_Protect *as, const char *filename, uintptr_t *p_brk);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);

  uintptr_t brk = 0;
  uintptr_t entry = loader(&pcb[i].as, filename, &brk);
  pcb[i].cur_brk = brk;
  pcb[i].max_brk = PGROUNDUP(brk);
  Log("load_prog: pcb[%d] %s entry = %p", i, filename, (void *)entry);

  uintptr_t ustack_lo = USER_STACK_TOP - STACK_SIZE;
  for (uintptr_t va = ustack_lo; va < USER_STACK_TOP; va += PGSIZE) {
    _map(&pcb[i].as, (void *)va, new_page());
  }

  _Area ustack = {(void *)ustack_lo, (void *)USER_STACK_TOP};
  _Area kstack;
  kstack.start = pcb[i].stack;
  kstack.end = kstack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, ustack, kstack, (void *)entry, NULL, NULL);
}

void switch_game(void) {
  assert(nr_proc >= 3);
  if (current_game == &pcb[0]) {
    current_game = &pcb[2];
    Log("switch_game: F12 -> videotest (pcb[2])");
  } else {
    current_game = &pcb[0];
    Log("switch_game: F12 -> pal (pcb[0])");
  }
}

_RegSet *schedule(_RegSet *prev) {
  if (current != NULL) {
    current->tf = prev;
  }

  if (nr_proc == 0) {
    return prev;
  }

  /* 三进程：current_game 多片 / hello 少片 */
  if (nr_proc == 3 && current_game != NULL) {
    if (current == &pcb[1]) {
      current = current_game;
      game_slices_left = SCHED_GAME_SLICES_BEFORE_HELLO - 1;
    } else if (current == current_game) {
      if (game_slices_left > 0) {
        game_slices_left--;
        current = current_game;
      } else {
        current = &pcb[1];
      }
    } else {
      /* 非当前游戏进程（切换 F12 后可能仍挂在旧游戏上） */
      current = current_game;
      game_slices_left = SCHED_GAME_SLICES_BEFORE_HELLO - 1;
    }
    _switch(&current->as);
    return current->tf;
  }

  int next = 0;
  if (current != NULL) {
    next = (int)(current - pcb);
    next = (next + 1) % nr_proc;
  }

  current = &pcb[next];
  _switch(&current->as);
  return current->tf;
}

void init_proc(void) {
  assert(nr_proc > 0);
  current_game = &pcb[0];
  current = &pcb[0];
  _switch(&current->as);
  restore_tf(current->tf);
}
