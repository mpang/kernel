#include <xeroskernel.h>

struct pcb* find_process(int pid);  // implemented in msg.c

void sigtramp(void (*handler)(void*), void *cntx, void *osp)
{
  handler(cntx);
  syssigreturn(osp);
  sysputs("Returned from syssigreturn in sigtramp. Shouldn't happen!!!\n");
}

int signal(int pid, int sig_no)
{
  if (sig_no < 0 || sig_no >= SIG_TABLE_SIZE) {
    return -2;
  }

  struct pcb *target_process = find_process(pid);
  if (!target_process) {
    return -1;
  }

  unsigned int sig_mask = 1 << sig_no;

  if (!(target_process->sig_handler_mask & sig_mask)) {
    // no handler installed for this signal. don't record this signal
    return -2;
  }

  target_process->sig_pending_mask |= sig_mask;

  if (target_process->state == BLOCKED || target_process->state == SIGBLOCKED) {
    target_process->ret = (target_process->state == BLOCKED) ? -129 : sig_no;
    ready(target_process);
  }

  // actual stack manipulation will be done in the dispatcher
  return 0;
}
