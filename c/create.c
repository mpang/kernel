/* create.c : create a process */

#include <xeroskernel.h>

/* Your code goes here. */
#define MAX_PID 65536 // 2^16
#define STARTING_EFLAGS 0x00003200
#define SAFETY_MARGIN 2 * 2 * 2 * 2 * 2 * 2 * 2 // safety margin of allocated stack

unsigned short getCS(void);

static int next_pid = 0;

int create(void (*func)(void), int stack)
{
  struct pcb *new_process = 0;
  struct process_context *context_frame = 0;
  int i;

  // find next available pcb
  // first spot in the pcb table is reserved for the idle process
  if (func != &idleproc) {
    for (i = 1; i < PCB_TABLE_SIZE; i++) {
      if (pcb_table[i].state == STOPPED) {
        new_process = &pcb_table[i];
        break;
      }
    }
  } else {
    new_process = &pcb_table[IDLE_PROC_PID];
  }

  if (!new_process) {
    return -1;
  }

  // initialize context_frame
  context_frame = kmalloc(stack);
  if (!context_frame) {
    return -1;
  }

  // adjust context position
  context_frame = (struct process_context *) ((unsigned int) context_frame + stack - sizeof(struct process_context) - SAFETY_MARGIN);

  // process stack content
  context_frame->edi = 0;
  context_frame->esi = 0;
  context_frame->ebp = 0;
  context_frame->esp = 0;
  context_frame->ebx = 0;
  context_frame->edx = 0;
  context_frame->ecx = 0;
  context_frame->eax = 0;
  context_frame->iret_cs = getCS();
  context_frame->iret_eip = (unsigned int) func;
  context_frame->eflags = STARTING_EFLAGS;

  // process return address
  unsigned int *ret_addr = (unsigned int *) (context_frame + 1);
  *ret_addr = (unsigned int) &sysstop;
  // assign to the pcb
  new_process->sp = (unsigned int) context_frame;

  // pid generation
  if (func == &idleproc) {
    new_process->pid = IDLE_PROC_PID;
  } else {
    // split pid into 2 2-byte numbers. first one is the sequence number which is between 0 and MAX_PID - 1.
    // second one is the actual index to the pcb table which is in range of 1 and PCB_TABLE_SIZE - 1.
    int sequence = next_pid++ % MAX_PID;
    // note that the resulting pid could never be 0
    int pid = (sequence << 16) | i;
    new_process->pid = pid;
  }

  // misc stuff
  new_process->ret = 0;
  new_process->sender_queue = 0;
  new_process->sender_queue_next = 0;
  new_process->args = 0;
  new_process->sleep_duration = 0;
  new_process->sig_pending_mask = 0;
  new_process->sig_handler_mask = 0;
  // we're willing to accept all signals at the beginning
  new_process->sig_ignore_mask = 0xffffffff;

  int j;
  for (j = 0; j < FD_TABLE_SIZE; j++) {
    new_process->fd_table[j] = -1;
  }

  for (j = 0; j < SIG_TABLE_SIZE; j++) {
    new_process->sig_handler_table[j] = 0;
  }

  ready(new_process);
  return new_process->pid;
}
