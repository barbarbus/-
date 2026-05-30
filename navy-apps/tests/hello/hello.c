#include <unistd.h>
#include <stdio.h>

static inline void yield_to_scheduler(void) {
  asm volatile("int $0x81");
}

int main() {
  write(1, "[hello] run\n", 12);
  write(1, "Hello World!\n", 13);
  int i = 2;
  volatile int j = 0;
  while (1) {
    j ++;
    if (j == 10000) {
      printf("Hello World for the %dth time\n", i ++);
      yield_to_scheduler();
      j = 0;
    }
  }
  return 0;
}
