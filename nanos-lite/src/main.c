#include "common.h"
#include "proc.h"

/* Uncomment these macros to enable corresponding functionality. */
#define HAS_ASYE
#define HAS_PTE

void init_mm(void);
void init_ramdisk(void);
void init_device(void);
void init_irq(void);
void init_fs(void);

int main() {
#ifdef HAS_PTE
  init_mm();
#endif

  Log("'Hello World!' from Nanos-lite");
  Log("Build time: %s, %s", __TIME__, __DATE__);

  init_ramdisk();

  init_device();

#ifdef HAS_ASYE
  Log("Initializing interrupt/exception handler...");
  init_irq();
#endif

  init_fs();

  Log("Loading user programs...");
  /* PA3-3 验收 /dev/events：请先点击 NEMU 模拟器窗口再按键（勿在终端打字） */
  Log(">>> /dev/events test: click NEMU window, then press keys (Return/A/...) <<<");
  load_prog("/bin/events");
  // load_prog("/bin/pal");
  // load_prog("/bin/hello");
  // load_prog("/bin/videotest");

  /* Enter the first user process; later switches happen in schedule(). */
  init_proc();
}
