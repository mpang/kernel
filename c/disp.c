/* disp.c : dispatcher */

#include <xeroskernel.h>
#include <kbd.h>
#include <stdarg.h>
#include <i386.h>

/* Your code goes here */
int process_in_sender_queue(struct pcb *process, struct pcb *sender_queue); // implemented in msg.c
void remove_process_from_sender_queue(struct pcb *sender, struct pcb *receiver);  // implemented in msg.c
void free_senders_to(struct pcb *process);
void free_receivers_from(struct pcb *process);
void check_signals(struct pcb *process);

void dispinit(void)
{
  // initializing pcb table
  int i;
  for (i = 0; i < PCB_TABLE_SIZE; i++) {
    pcb_table[i].pid = 0;
    pcb_table[i].state = STOPPED;
    pcb_table[i].ret = 0;
    pcb_table[i].args = 0;
    pcb_table[i].sp = 0;
    pcb_table[i].sleep_duration = 0;
    pcb_table[i].sender_queue = 0;
    pcb_table[i].sender_queue_next = 0;
    pcb_table[i].next = 0;
    pcb_table[i].sig_pending_mask = 0;
    pcb_table[i].sig_handler_mask = 0;
    // we're willing to accept all signals at the beginning
    pcb_table[i].sig_ignore_mask = 0xffffffff;

    // initialize file descriptor table
    int j;
    for (j = 0; j < FD_TABLE_SIZE; j++) {
      pcb_table[i].fd_table[j] = -1;
    }

    // initialize signal handler table
    for (j = 0; j < SIG_TABLE_SIZE; j++) {
      pcb_table[i].sig_handler_table[j] = 0;
    }
  }

  // initializing ready queue
  ready_queue = 0;
  read_blocked_process = 0;

  kprintf("dispatcher initialized\n");
}

void dispatch(void)
{
  struct pcb *process = next();
  va_list args;

  while (1) {
    int request = contextswitch(process); // implemented in ctsw.c
    
    switch (request) {
      case CREATE:
        args = (va_list) process->args;
        void (*func)(void) = (void (*)(void)) va_arg(args, unsigned int);
        int stack = va_arg(args, int);
        process->ret = create(func, stack); // store pid in ret in pcb
        break;
      case YIELD:
        ready(process);
        process = next();
        break;
      case STOP:
        free_senders_to(process);
        free_receivers_from(process);
        process->state = STOPPED;
        kfree((void*) process->sp);
        process = next();
        break;
      case GET_PID:
        process->ret = process->pid;
        break;
      case PUTS:
        args = (va_list) process->args;
        char *str = va_arg(args, char*);
        kprintf("%s", str);
        break;
      case SEND:
        args = (va_list) process->args;
        unsigned int dest_pid = va_arg(args, unsigned int);
        void *sender_buffer = va_arg(args, void*);
        int sender_buffer_len = va_arg(args, int);
        process->ret = send(dest_pid, sender_buffer, sender_buffer_len, process);

        if (process->state != BLOCKED) {
          ready(process);
        }
        process = next();
        break;
      case RECEIVE:
        args = (va_list) process->args;
        unsigned int *from_pid = va_arg(args, unsigned int*);
        void *receiver_buffer = va_arg(args, void*);
        int receiver_buffer_len = va_arg(args, int);
        process->ret = recv(from_pid, receiver_buffer, receiver_buffer_len, process);

        if (process->state != BLOCKED) {
          ready(process);
        }
        process = next();
        break;
      case TIMER:
        tick();
        ready(process);
        process = next();
        end_of_intr();
        break;
      case SLEEP:
        args = (va_list) process->args;
        unsigned int ms = va_arg(args, unsigned int);
        sleep(process, ms);
        process = next();
        break;
      case SIGHANDLER:
        args = (va_list) process->args;
        int sig_no = va_arg(args, int);
        void (*newhandler)(void*) = (void (*)(void*)) va_arg(args, unsigned int);
        void (**oldhandler)(void*) = (void (**)(void*)) va_arg(args, unsigned int);

        if (sig_no < 0 || sig_no >= SIG_TABLE_SIZE) {
          process->ret = -1;
        } else {
          *oldhandler = process->sig_handler_table[sig_no];
          process->sig_handler_table[sig_no] = newhandler;
          if (newhandler) {
            process->sig_handler_mask |= (1 << sig_no);
          } else {
            // null pointer is passed in as the handler, ignore this signal
            process->sig_handler_mask &= ~(1 << sig_no);
          }
          process->ret = 0;
        }
        break;
      case SIGRETURN:
        args = (va_list) process->args;
        void *osp = va_arg(args, void*);
        process->sp = (unsigned int) osp;
        // recover old sig_ignore_mask
        struct sig_handler_context *shc = (struct sig_handler_context *) (process->sp - sizeof(struct sig_handler_context));
        process->sig_ignore_mask = shc->old_sig_ignore_mask;
        break;
      case SIGKILL:
        args = (va_list) process->args;
        int pid = va_arg(args, int);
        int sig = va_arg(args, int);

        int sigkill_result = signal(pid, sig);
        if (sigkill_result == -1) {
          // invalid pid
          process->ret = -33;
        } else if (sigkill_result == -2) {
          // invalid signal
          process->ret = -12;
        } else {
          process->ret = 0;
        }
        break;
      case SIGWAIT:
        process->state = SIGBLOCKED;
        process = next();
        break;
      case KEYBOARD:
        kbd_int_handler();
        end_of_intr();
      case OPEN:
        args = (va_list) process->args;
        int device_no = va_arg(args, int);
        process->ret = di_open(process, device_no);
        break;
      case CLOSE:
        args = (va_list) process->args;
        int close_fd = va_arg(args, int);
        process->ret = di_close(process, close_fd);
        break;
      case WRITE:
        args = (va_list) process->args;
        int write_fd = va_arg(args, int);
        void *write_buffer = va_arg(args, void*);
        int write_buffer_len = va_arg(args, int);
        process->ret = di_write(process, write_fd, write_buffer, write_buffer_len);
        break;
      case READ:
        args = (va_list) process->args;
        int read_fd = va_arg(args, int);
        void *read_buffer = va_arg(args, void*);
        int read_buffer_len = va_arg(args, int);

        int read_result = di_read(process, read_fd, read_buffer, read_buffer_len);
        if (read_result == -2) {
          // -2 means process should be blocked
          process->state = BLOCKED;
          read_blocked_process = process;
          process = next();
        } else {
          // other results: 0 means eof, -1 means error, + number means bytes read
          process->ret = read_result;
        }
        break;
      case IOCTL:
        args = (va_list) process->args;
        int ioctl_fd = va_arg(args, int);
        unsigned long command = va_arg(args, unsigned long);
        unsigned int ioctl_args = va_arg(args, unsigned int);
        process->ret = di_ioctl(process, ioctl_fd, command, ioctl_args);
        break;
    }
  }
}

struct pcb* next(void)
{
  if (!ready_queue) {
    // if no process available, return the idle process
    return &pcb_table[IDLE_PROC_PID];
  }

  struct pcb *process = ready_queue;
  ready_queue = ready_queue->next;

  process->state = RUNNING;
  process->next = 0;
  check_signals(process);
  return process;
}

void ready(struct pcb *process)
{
  if (process->pid == IDLE_PROC_PID) {
    return;
  }

  process->state = READY;
  process->next = 0;

  if (!ready_queue) {
    ready_queue = process;
  } else {
    struct pcb *iterator;
    // loop till the end of the queue
    for (iterator = ready_queue; iterator->next; iterator = iterator->next);
    iterator->next = process;
  }
}

// free all processes that are waiting to send to the process
// upon its termination
void free_senders_to(struct pcb *process)
{
  struct pcb *sender_queue = process->sender_queue;
  struct pcb *sender;

  while (sender_queue) {
    sender = sender_queue;
    sender_queue = sender_queue->sender_queue_next;
    if (sender->state == BLOCKED) {
      sender->ret = -1;
      sender->sender_queue_next = 0;
      ready(sender);
    }
  }
}

// free all processes that are waiting to receive from the process
// upon its termination
void free_receivers_from(struct pcb *process)
{
  int i;
  for (i = 1; i < PCB_TABLE_SIZE; i++) {
    struct pcb *p  = &pcb_table[i];
    if (p->state == BLOCKED && process_in_sender_queue(process, p->sender_queue)) {
      p->ret = -1;
      remove_process_from_sender_queue(process, p);
      ready(p);
    }
  }
}

void add_sigtramp_stack(struct pcb *process, int sig_no, int old_sig_ignore_mask)
{
  unsigned int old_sp = process->sp;

  // context for signal handler
  struct sig_handler_context *shc = (struct sig_handler_context *) (process->sp - sizeof(struct sig_handler_context));
  shc->ret_addr = (unsigned int) &sysstop;  // won't be used anyway
  shc->handler = (unsigned int) process->sig_handler_table[sig_no];
  shc->cntx = old_sp;
  shc->osp = old_sp;
  // to recover later
  shc->old_sig_ignore_mask = old_sig_ignore_mask;

  // context for sigtramp
  struct process_context *process_cpu_state = (struct process_context *) old_sp;
  struct process_context *sigtramp_context = (struct process_context *) ((unsigned int) shc - sizeof(struct process_context));
  sigtramp_context->edi = process_cpu_state->edi;
  sigtramp_context->esi = process_cpu_state->esi;
  sigtramp_context->ebp = process_cpu_state->ebp;
  sigtramp_context->esp = process_cpu_state->esp;
  sigtramp_context->ebx = process_cpu_state->ebx;
  sigtramp_context->edx = process_cpu_state->edx;
  sigtramp_context->ecx = process_cpu_state->ecx;
  sigtramp_context->eax = process_cpu_state->eax;
  sigtramp_context->iret_eip = (unsigned int) &sigtramp;  // address of sigtramp code
  sigtramp_context->iret_cs = process_cpu_state->iret_cs;
  sigtramp_context->eflags = process_cpu_state->eflags;

  process->sp = (unsigned int) sigtramp_context;
}

void check_signals(struct pcb *process)
{
  if (!(process->sig_pending_mask & process->sig_ignore_mask)) {
    // either no signals pending, or the priority is below threshold
    return;
  }

  unsigned int sig_mask = process->sig_pending_mask;
  // index of the most significant bit, which indicates
  // the signal with the highest priority
  int sig_no = 0;
  while (sig_mask >>= 1) {
    sig_no++;
  }

  // clear this signal in the sig_pending_mask
  process->sig_pending_mask &= ~(1 << sig_no);
  // modify the ignore mask
  unsigned int old_sig_ignore_mask = process->sig_ignore_mask;
  process->sig_ignore_mask = 0xffffffff << (sig_no + 1);
  // do the actual work of manipulating stack
  add_sigtramp_stack(process, sig_no, old_sig_ignore_mask);
}
