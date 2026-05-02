#include "game.h"

static int real_fps;
static fly_t head = NULL;
static int hit = 0, miss = 0;

// A tiny RNG that avoids div/mod; deterministic per boot.
static uint32_t rng_state = 1;
static inline uint32_t rng_next() {
  // xorshift32
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state = x ? x : 1;
  return x;
}

void set_fps(int value) {
  real_fps = value;
}

int get_fps() {
  return real_fps;
}

int main (){
  _ioe_init();
  unsigned long start = _uptime();
  rng_state = (uint32_t)(start ^ 0x9e3779b9u) | 1u;

  // Make the first frame immediately visible.
  create_new_letter();

  // Logic/update tick: 100 Hz (10ms). Render tick: 30 FPS (~33ms).
  const unsigned long LOGIC_DT_MS = 1000 / UPDATE_PER_SECOND; // 10
  const unsigned long RENDER_DT_MS = 1000 / FPS;              // 33

  unsigned long last_time = start;
  unsigned long next_logic = start;
  unsigned long next_render = start;

  // Spawn pacing (characters per second) using an accumulator in milli-characters.
  // spawn_acc += dt_ms * CHARACTER_PER_SECOND; when >= 1000, spawn one.
  int spawn_acc = 0;

  int num_draw = 0;
  while (1) {
    unsigned long now = _uptime();

    // Drive logic in fixed steps; never depends on %/div at runtime.
    while ((long)(now - next_logic) >= 0) {
      unsigned long dt = next_logic - last_time;
      last_time = next_logic;
      next_logic += LOGIC_DT_MS;

      // Input handling
      while (keyboard_event());
      while (update_keypress());

      // Spawn based on elapsed time
      spawn_acc += (int)(dt * CHARACTER_PER_SECOND);
      while (spawn_acc >= 1000) {
        create_new_letter();
        spawn_acc -= 1000;
      }

      // Move letters
      update_letter_pos();
    }

    // Render
    if ((long)(now - next_render) >= 0) {
      next_render += RENDER_DT_MS;
      num_draw++;
      if (now != 0) set_fps((int)(num_draw * 1000 / now));
      redraw_screen();
    }
  }
}

LINKLIST_IMPL(fly, 1000)

int get_hit(){
  return hit;
}

int get_miss(){
  return miss;
}

fly_t characters(){
  return head;
}

void create_new_letter(){
  if(head == NULL){
    head = fly_new();
  }
  else{
    fly_t now = fly_new();
    fly_insert(NULL,head,now);
    head = now;
  }

  uint32_t r = rng_next();
  head->y = 0;
  head->x = (((r & 31) + 1) * 8);
  if (head->x > W - 8) head->x = W - 8;
  head->v = (int)(((r >> 5) & 3) + 1);
  head->text = (int)((r >> 8) & 31);
  if (head->text >= 26) head->text -= 26;
  if (head->text >= 26) head->text -= 26;
  release_key(head->text);
}

void update_letter_pos() {
  fly_t it;
  for(it = head;it != NULL;){
    fly_t next = it->_next;
    it->y += it->v;
    if (it->y < 0 || it->y + 8 > H){
      if(it->y < 0)
        hit++;
      else
        miss++;
      fly_remove(it);
      fly_free(it);
      if(it == head)
        head = next;
    }
    it = next;
  }
}

bool update_keypress() {
  fly_t it,target = NULL;
  int min = -100;
  for(it = head; it != NULL; it = it->_next){
    if(it->v > 0 && it->y > min && query_key(it->text)){
      min = it->y;
      target = it;
    }
  }
  if(target != NULL){
    release_key(target->text);
    target->v = -3;
    return true;
  }
  return false;
}
