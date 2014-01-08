/* user.c : User processes */

#include <xeroskernel.h>
#include <xeroslib.h>

/* Your code goes here */
static unsigned int root_pid;

// producer process
void producer(void)
{
  int i;
  for (i = 0; i < 4; i++) {
    kprintf("Happy\n");
    sysyield();
  }

  sysputs("producer finished\n");
  sysstop();
}

// consumer process
void consumer(void)
{
  int i;
  for (i = 0; i < 4; i++) {
    kprintf("New Year\n");
    sysyield();
  }

  sysputs("consumer finished\n");
  sysstop();
}

// processes for a2
void messenger(void)
{
  char *str = 0;
  int pid = sysgetpid();
  sprintf(str, "process %d is alive\n", pid);
  sysputs(str);

  syssleep(5 * 1000);

  int message;
  sysrecv(&root_pid, &message, sizeof(int));
  sprintf(str, "process %d has received from root\n", pid);
  sysputs(str);
  sprintf(str, "it is about to sleep for %d milliseconds\n", message);
  sysputs(str);

  syssleep(message);

  sprintf(str, "sleeping has been stopped for process %d and it is about to exit\n", pid);
  sysputs(str);
}

// root process for a2
void a2_root(void)
{
  char *str = 0;
  root_pid = sysgetpid();

  sysputs("root process is alive\n");

  unsigned int first = syscreate(&messenger, 1024 * 8);
  sprintf(str, "first process has been created with pid: %d\n", first);
  sysputs(str);

  unsigned int second = syscreate(&messenger, 1024 * 8);
  sprintf(str, "second process has been created with pid: %d\n", second);
  sysputs(str);

  unsigned int third = syscreate(&messenger, 1024 * 8);
  sprintf(str, "third process has been created with pid: %d\n", third);
  sysputs(str);

  unsigned int fourth = syscreate(&messenger, 1024 * 8);
  sprintf(str, "fourth process has been created with pid: %d\n", fourth);
  sysputs(str);

  syssleep(4 * 1000);

  sysputs("root is awoken from sleep\n");

  int message = 10 * 1000;
  syssend(third, &message, sizeof(int));

  message = 7 * 1000;
  syssend(second, &message, sizeof(int));

  message = 20 * 1000;
  syssend(first, &message, sizeof(int));

  message = 27 * 1000;
  syssend(fourth, &message, sizeof(int));

  int dummy_message = 0;
  sprintf(str, "result of receiving from the fourth process: %d\n", sysrecv(&fourth, &dummy_message, sizeof(int)));
  sysputs(str);

  sprintf(str, "result of sending to the third process: %d\n", syssend(third, &dummy_message, sizeof(int)));
  sysputs(str);
}

// signal handler for a3
void signal_handler_1(void *cntx)
{
  sysputs("signal handler has been called!\n");
}

void signal_handler_2(void *cntx)
{
  sysputs("second signal handler has been called!\n");
}

void sig_process_1(void)
{
  syssleep(1 * 1000);
  char *str = 0;

  sprintf(str, "return value of sending signal 20 to root: %d\n", syskill(root_pid, 20));
  sysputs(str);

  sprintf(str, "return value of sending signal 18 to root: %d\n", syskill(root_pid, 18));
  sysputs(str);
}

void sig_process_2(void)
{
  syssleep(5 * 1000);
  char *str = 0;

  sprintf(str, "return value of sending signal 18 to root: %d\n", syskill(root_pid, 18));
  sysputs(str);
}

void sig_process_3(void)
{
  syssleep(5 * 1000);
  char *str = 0;

  sprintf(str, "return value of sending signal 20 to root: %d\n", syskill(root_pid, 20));
  sysputs(str);
}

void root(void)
{
  root_pid = sysgetpid();
  void (*dummy_handler)(void*) = 0;
  void (**oldhandler)(void*) = &dummy_handler;
  int fd;
  char *str = 0;

  sysputs("-----------------------------------\n");
  sysputs("Step 1:\n");
  sysputs("root process is alive\n");
  sysputs("-----------------------------------\n");

  sysputs("Step 2:\n");
  fd = sysopen(KEYBOARD_ECHO);
  sprintf(str, "return value of opening echo keyboard: %d\n", fd);
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 3:\n");
  unsigned char first_buffer;
  int i;
  for (i = 0; i < 10; i++) {
    sysread(fd, &first_buffer, sizeof(unsigned char));
    sprintf(str, "character read: %c\n", first_buffer);
    sysputs(str);
  }
  sysputs("-----------------------------------\n");

  sysputs("Step 4:\n");
  sprintf(str, "return value of opening non-echo keyboard: %d\n", sysopen(KEYBOARD_NO_ECHO));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 5:\n");
  sprintf(str, "return value of opening echo keyboard: %d\n", sysopen(KEYBOARD_ECHO));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 6:\n");
  sprintf(str, "return value of closing echo keyboard: %d\n", sysclose(fd));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 7:\n");
  fd = sysopen(KEYBOARD_NO_ECHO);
  sprintf(str, "return value of opening non-echo keyboard: %d\n", fd);
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 8:\n");
  unsigned char second_buffer[31] = { '\0' };
  int result1;
  int result2;

  result1 = sysread(fd, second_buffer, 10 * sizeof(unsigned char));
  sprintf(str, "return value of first read: %d\n", result1);
  sysputs(str);

  result2 = sysread(fd, second_buffer + result1, 10 * sizeof(unsigned char));
  sprintf(str, "return value of second read: %d\n", result2);
  sysputs(str);

  sprintf(str, "return value of third read: %d\n", sysread(fd, second_buffer + result1 + result2, 10 * sizeof(unsigned char)));
  sysputs(str);

  sprintf(str, "%s\n", second_buffer);
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 9:\n");
  while (sysread(fd, &first_buffer, sizeof(unsigned char)) > 0) {
    sprintf(str, "character read: %c\n", first_buffer);
    sysputs(str);
  }
  sysputs("-----------------------------------\n");

  sysputs("Step 10:\n");
  sprintf(str, "return value of closing non-echo keyboard: %d\n", sysclose(fd));
  sysputs(str);

  fd = sysopen(KEYBOARD_ECHO);
  sprintf(str, "return value of opening echo keyboard: %d\n", fd);
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 11:\n");
  sprintf(str, "return value of installing handler for signal 18: %d\n", syssighandler(18, &signal_handler_1, oldhandler));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 12:\n");
  sprintf(str, "return value of creating 1st child process: %d\n", syscreate(&sig_process_1, 1024 * 8));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 13 & 14:\n");
  sprintf(str, "return value of reading from keyboard: %d\n", sysread(fd, &first_buffer, sizeof(unsigned char)));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 15:\n");
  sprintf(str, "return value of creating 2nd child process: %d\n", syscreate(&sig_process_2, 1024 * 8));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 16:\n");
  sprintf(str, "return value of installing handler for signal 18: %d\n", syssighandler(18, &signal_handler_2, oldhandler));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 17 & 18:\n");
  sprintf(str, "return value of reading from keyboard: %d\n", sysread(fd, &first_buffer, sizeof(unsigned char)));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 19:\n");
  sprintf(str, "return value of installing handler for signal 20: %d\n", syssighandler(20, *oldhandler, oldhandler));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 20:\n");
  sprintf(str, "return value of creating 3rd child process: %d\n", syscreate(&sig_process_3, 1024 * 8));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 21 & 22:\n");
  sprintf(str, "return value of reading from keyboard: %d\n", sysread(fd, &first_buffer, sizeof(unsigned char)));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 23:\n");
  while (sysread(fd, &first_buffer, sizeof(unsigned char)) > 0) {
    sprintf(str, "character read: %c\n", first_buffer);
    sysputs(str);
  }
  sysputs("-----------------------------------\n");

  sysputs("Step 24:\n");
  sprintf(str, "return value of reading from keyboard: %d\n", sysread(fd, &first_buffer, sizeof(unsigned char)));
  sysputs(str);
  sysputs("-----------------------------------\n");

  sysputs("Step 25:\n");
  sysputs("root about to be terminated\n");
  sysputs("-----------------------------------\n");
}

// idle process
void idleproc(void)
{
  while (1) {
    __asm __volatile("hlt");
  }
}
