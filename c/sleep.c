/* sleep.c : sleep device (assignment 2) */

#include <xeroskernel.h>

/* Your code goes here */
// 10 ms per tick
#define MS_PER_TICK 10

static struct pcb *sleep_queue;

void sleep(struct pcb *process, unsigned int ms)
{
  unsigned int ticks = ms / MS_PER_TICK;
  ticks += (ms % MS_PER_TICK) ? 0 : 1;
  process->state = SLEPT;
  process->sleep_duration = ticks;
  process->next = 0;

  if (!sleep_queue) {
    sleep_queue = process;
  } else if (sleep_queue->sleep_duration > ticks) {
    // add to the front of the sleep queue
    process->next = sleep_queue;
    sleep_queue = process;
  } else {
    struct pcb *iterator;
    for (iterator = sleep_queue; iterator->next; iterator = iterator->next) {
      if (iterator->sleep_duration <= ticks && iterator->next->sleep_duration > ticks) {
        // add to the middle of the queue
        process->next = iterator->next;
        iterator->next = process;
        return;
      }
    }

    // add to the end of the queue
    iterator->next = process;
  }
}

void tick(void)
{
  if (!sleep_queue) {
    return;
  }

  struct pcb *iterator;
  for (iterator = sleep_queue; iterator; iterator = iterator->next) {
    iterator->sleep_duration--;
  }

  while (sleep_queue && !sleep_queue->sleep_duration) {
    struct pcb *process = sleep_queue;
    sleep_queue = sleep_queue->next;
    process->ret = 0;
    ready(process);
  }
}
