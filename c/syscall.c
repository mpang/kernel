/* syscall.c : syscalls */

#include <xeroskernel.h>
#include <stdarg.h>

/* Your code goes here */
int syscall(int call, ...)
{
  va_list arg_list;
  int ret;

  va_start(arg_list, call);

  // we're in process space
  __asm __volatile("movl %0, %%eax" : : "g" (call) : "%eax");  // save param call to register eax
  __asm __volatile("movl %0, %%edx" : : "g" (arg_list) : "%eax");   // save rest of arguments to register edx
  __asm __volatile("int %0" : : "i" (KERNEL_INTERRUPT) : "%eax");  // call interrupt
  __asm __volatile("movl %%eax, %0" : "=g" (ret) : : "%eax");  // save return value to variable ret

  va_end(arg_list);

  return ret;
}

// system call to create a new process
int syscreate(void (*func)(void), int stack)
{
  return syscall(CREATE, func, stack);
}

// system call to yield the processor to the next process
void sysyield(void)
{
  syscall(YIELD);
}

// system call to stop the current process
void sysstop(void)
{
  syscall(STOP);
}

unsigned int sysgetpid(void)
{
  return syscall(GET_PID);
}

void sysputs(char *str)
{
  syscall(PUTS, str);
}

int syssend(unsigned int receiver_pid, void *buffer, int buffer_len)
{
  return syscall(SEND, receiver_pid, buffer, buffer_len);
}

int sysrecv(unsigned int *sender_pid, void *buffer, int buffer_len)
{
  return syscall(RECEIVE, sender_pid, buffer, buffer_len);
}

unsigned int syssleep(unsigned int ms)
{
  return syscall(SLEEP, ms);
}

int syssighandler(int signal, void (*newhandler)(void*), void (**oldhandler)(void*))
{
  return syscall(SIGHANDLER, signal, newhandler, oldhandler);
}

void syssigreturn(void *old_sp)
{
  syscall(SIGRETURN, old_sp);
}

int syskill(int pid, int signal_number)
{
  return syscall(SIGKILL, pid, signal_number);
}

int syssigwait(void)
{
  return syscall(SIGWAIT);
}

int sysopen(int device_no)
{
  return syscall(OPEN, device_no);
}

int sysclose(int fd)
{
  return syscall(CLOSE, fd);
}

int syswrite(int fd, void *buffer, int buffer_len)
{
  return syscall(WRITE, fd, buffer, buffer_len);
}

int sysread(int fd, void *buffer, int buffer_len)
{
  return syscall(READ, fd, buffer, buffer_len);
}

int sysioctl(int fd, unsigned long command, ...)
{
  va_list args;
  va_start(args, command);

  int ret = syscall(IOCTL, fd, command, (unsigned int) args);
  va_end(args);
  return ret;
}
