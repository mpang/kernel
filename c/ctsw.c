/* ctsw.c : context switcher */

#include <xeroskernel.h>
#include <i386.h>

/* Your code goes here - You will need to write some assembly code. You must
   use the gnu conventions for specifying the instructions. (i.e this is the
   format used in class and on the slides.) You are not allowed to change the
   compiler/assembler options or issue directives to permit usage of Intel's
   assembly language conventions.
*/

void set_evec(unsigned int xnum, unsigned long handler);

// switch from process to kernel
void _KernelEntryPoint(void);
// hardware timer interrupt
void _TimerEntryPoint(void);
// keyboard interrupt
void _KeyboardEntryPoint(void);

// have to use static variables to store corresponding values
// first before assgning them to process
// otherwise it would not work
static unsigned int saved_sp;
static int rc;
static int interrupt;
static unsigned int args;
static void *kernel_stack;

void contextinit(void)
{
  set_evec(KERNEL_INTERRUPT, (int) _KernelEntryPoint);
  set_evec(TIMER_INT, (int) _TimerEntryPoint);
  set_evec(KEYBOARD_INT, (int) _KeyboardEntryPoint);
  initPIT(100);

  kprintf("context switcher initialized\n");
}

int contextswitch(struct pcb *process)
{
  saved_sp = process->sp;
  rc = process->ret;

  // from kernel to process
  __asm __volatile("pushf");  // push flags on kernel stack
  __asm __volatile("pusha");  // push registers on kernel stack
  __asm __volatile("movl %%esp, %0" : "=g" (kernel_stack));  // save kernel stack pointer
  __asm __volatile("movl %0, %%esp" : : "g" (saved_sp));  // switch to process stack
  __asm __volatile("movl %0, 28(%%esp)" : : "g" (rc));  // store return value to eax on process stack
  __asm __volatile("popa");
  __asm __volatile("iret");

  // keyboard interrupt
  __asm __volatile("_KeyboardEntryPoint:");
  __asm __volatile("cli");
  __asm __volatile("pusha");
  __asm __volatile("movl $2, %%ecx" : : : "%eax", "%ecx", "%edx");
  __asm __volatile("jmp _CommonEntryPoint");

  // hardware timer interrupt
  __asm __volatile("_TimerEntryPoint:");
  __asm __volatile("cli");
  __asm __volatile("pusha");
  __asm __volatile("movl $1, %%ecx" : : : "%eax", "%ecx", "%edx");
  __asm __volatile("jmp _CommonEntryPoint");

  // from process to kernel
  __asm __volatile("_KernelEntryPoint:");
  __asm __volatile("cli");
  __asm __volatile("pusha");
  __asm __volatile("movl $0, %%ecx" : : : "%eax", "%ecx", "%edx");

  // common context switching
  __asm __volatile("_CommonEntryPoint:");
  __asm __volatile("movl %%eax, %0" : "=g" (rc) : : "%eax", "%ecx", "%edx");  // save request type
  __asm __volatile("movl %%ecx, %0" : "=g" (interrupt) : : "%eax", "%ecx", "%edx"); // save interrupt flag
  __asm __volatile("movl %%edx, %0" : "=g" (args) : : "%eax", "%ecx", "%edx");  // save process arguments
  __asm __volatile("movl %%esp, %0" : "=g" (saved_sp) : : "%eax", "%ecx", "%edx"); // save process stack pointer
  __asm __volatile("movl %0, %%esp" : : "g" (kernel_stack) : "%eax", "%ecx", "%edx"); // switch to kernel stack
  __asm __volatile("popa");
  __asm __volatile("popf");

  process->sp = saved_sp;
  if (interrupt) {
    process->ret = rc;
    rc = (interrupt == 1) ? TIMER : KEYBOARD;
  } else {
    process->args = args;
  }
  return rc;
}
