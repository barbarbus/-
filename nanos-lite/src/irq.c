#include "common.h"
#include "proc.h"

/* 临时关闭时钟抢占调度（events 等 PA3 验收用）；恢复 PA4 分时请改回 0 */
#define TEMP_DISABLE_IRQ_SCHED 1

_RegSet *do_syscall(_RegSet *r);

static _RegSet *do_event(_Event e, _RegSet *r) {
  switch (e.event) {
    case _EVENT_SYSCALL:
      return do_syscall(r);
    case _EVENT_TRAP:
      return schedule(r);
    case _EVENT_IRQ_TIME:
#if TEMP_DISABLE_IRQ_SCHED
      return r;
#else
    {
      static int irq_time_tick;
      irq_time_tick++;
      if (irq_time_tick <= 5 || (irq_time_tick % 10) == 0) {
        Log("EVENT_IRQ_TIME tick %d -> schedule()", irq_time_tick);
      }
      return schedule(r);
    }
#endif
    default:
      panic("Unhandled event ID = %d", e.event);
  }

  return NULL;
}

void init_irq(void) {
#if TEMP_DISABLE_IRQ_SCHED
  Log("TEMP_DISABLE_IRQ_SCHED: timer IRQ will not call schedule()");
#endif
  _asye_init(do_event);
}
