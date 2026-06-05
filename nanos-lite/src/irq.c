#include "common.h"
#include "proc.h"

_RegSet *do_syscall(_RegSet *r);

static _RegSet *do_event(_Event e, _RegSet *r) {
  switch (e.event) {
    case _EVENT_SYSCALL:
      return do_syscall(r);
    case _EVENT_TRAP:
      return schedule(r);
    case _EVENT_IRQ_TIME:
      /* 单进程跑 PAL 时不抢占调度，避免无意义切换与终端刷屏 */
      return r;
    default:
      panic("Unhandled event ID = %d", e.event);
  }

  return NULL;
}

void init_irq(void) {
  _asye_init(do_event);
}
