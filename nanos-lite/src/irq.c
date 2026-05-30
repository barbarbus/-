#include "common.h"
#include "proc.h"

_RegSet *do_syscall(_RegSet *r);

static _RegSet *do_event(_Event e, _RegSet *r) {
  switch (e.event) {
    case _EVENT_SYSCALL:
      return do_syscall(r);
    case _EVENT_TRAP:
      return schedule(r);
    case _EVENT_IRQ_TIME: {
      static int irq_time_tick;
      irq_time_tick++;
      /* 前几下必打，之后每 10 次打一行，避免 100Hz 刷屏又容易肉眼确认 */
      if (irq_time_tick <= 5 || (irq_time_tick % 10) == 0) {
        Log("EVENT_IRQ_TIME tick %d -> schedule()", irq_time_tick);
      }
      return schedule(r);
    }
    default:
      panic("Unhandled event ID = %d", e.event);
  }

  return NULL;
}

void init_irq(void) {
  _asye_init(do_event);
}
