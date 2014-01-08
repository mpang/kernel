/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */

/* Symbolic constants used throughout Xinu */

typedef	char		Bool;		/* Boolean type			*/
#define	FALSE		0		/* Boolean constants		*/
#define	TRUE		1
#define	EMPTY		(-1)		/* an illegal gpq		*/
#define	NULL		0		/* Null pointer for linked lists*/
#define	NULLCH		'\0'		/* The null character		*/


/* Universal return constants */

#define	OK		 1		/* system call ok		*/
#define	SYSERR		-1		/* system call failed		*/
#define	EOF		-2		/* End-of-file (usu. from read)	*/
#define	TIMEOUT		-3		/* time out  (usu. recvtim)	*/
#define	INTRMSG		-4		/* keyboard "intr" key pressed	*/
					/*  (usu. defined as ^B)	*/
#define	BLOCKERR	-5		/* non-blocking op would block	*/

/* Functions defined by startup code */


void bzero(void *base, int cnt);
void bcopy(const void *src, void *dest, unsigned int n);
int kprintf(char * fmt, ...);
void lidt(void);
void init8259(void);
void disable(void);
void outb(unsigned int, unsigned char);
unsigned char inb(unsigned int);


/* Functions and structures in memeory manager */
struct mem_header {
  unsigned long size;
  struct mem_header *previous;
  struct mem_header *next;
  unsigned char *data_start[0];
};

extern void kmeminit(void);
extern void *kmalloc(int size);
extern void kfree(void *pointer);


/* Variables, constants, functions and structures in dispatcher */
// process states
#define BLOCKED 0
#define READY 1
#define RUNNING 2
#define SLEPT 3
#define STOPPED 4
#define SIGBLOCKED 5

// other constants
#define PCB_TABLE_SIZE 32
#define SIG_TABLE_SIZE 32
#define KEYBOARD_NO_ECHO 0
#define KEYBOARD_ECHO 1
#define FD_TABLE_SIZE 4
#define NUM_OF_DEVICE 2
#define IDLE_PROC_PID 0

// structures
struct pcb {
  int pid;
  int state;
  int ret;  // process return value, if any
  unsigned int sp;  // address to the process stack
  unsigned int args;  // address to process arguments, if any
  unsigned int sleep_duration;  // for sleep device, in terms of ticks
  struct pcb *sender_queue; // sender queue for messaging system
  // specific to sender queue, because sleep queue and ready queue
  // are mutually exclusive, but sender queue and ready queue are not
  struct pcb *sender_queue_next;
  struct pcb *next;
  // array of signal handlers
  void (*sig_handler_table[SIG_TABLE_SIZE])(void*);
  // bit mask records all the signals that are currently targeted at this process
  unsigned int sig_pending_mask;
  // bit mask indicates whether a handler is available for a signal
  unsigned int sig_handler_mask;
  // bit mask indicates what signals should be ignored
  unsigned int sig_ignore_mask;
  // file descriptor table, which holds the index to the device table
  int fd_table[FD_TABLE_SIZE];
};

struct process_context {
  unsigned int edi;
  unsigned int esi;
  unsigned int ebp;
  unsigned int esp;
  unsigned int ebx;
  unsigned int edx;
  unsigned int ecx;
  unsigned int eax;
  unsigned int iret_eip;
  unsigned int iret_cs;
  unsigned int eflags;
};

// stack context for signal handler when doing the
// context switch
struct sig_handler_context {
  unsigned int ret_addr;
  unsigned int handler;
  unsigned int cntx;
  unsigned int osp;
  // save it here so that it could be recovered later
  // in sigreturn
  unsigned int old_sig_ignore_mask;
};

// device structure
struct devsw {
  int major_no;
  int (*open)(int);
  int (*close)(void);
  int (*write)(void*, int);
  int (*read)(void*, int);
  int (*ioctl)(unsigned long, unsigned int);
};

// variables
struct pcb pcb_table[PCB_TABLE_SIZE];
// device table internal to the kernel
struct devsw device_table[NUM_OF_DEVICE];
struct pcb *ready_queue;  // use pcb struct directly to implement queue
struct pcb *read_blocked_process; // only one process can open a device at a time

// functions
extern void dispinit(void);
extern void dispatch(void);
extern struct pcb* next(void);
extern void ready(struct pcb *process);


/* Constants and functions in context switcher */
// interrupt handler number
#define KERNEL_INTERRUPT 49
#define TIMER_INT 32
#define KEYBOARD_INT 33

// request type
#define CREATE 0
#define YIELD 1
#define STOP 2
#define GET_PID 3
#define PUTS 4
#define SEND 5
#define RECEIVE 6
#define TIMER 7
#define SLEEP 8
#define SIGHANDLER 9
#define SIGRETURN 10
#define SIGKILL 11
#define SIGWAIT 12
#define KEYBOARD 13
#define OPEN 14
#define CLOSE 15
#define WRITE 16
#define READ 17
#define IOCTL 18

// functions
extern void contextinit(void);
extern int contextswitch(struct pcb *process);


/* Functions in process creator */
extern int create(void (*func)(void), int stack);


/* Functions in syscall.c */
extern int syscall(int call, ...);
extern int syscreate(void (*func)(void), int stack);
extern void sysyield(void);
extern void sysstop(void);
extern unsigned int sysgetpid(void);
extern void sysputs(char *str);
extern int syssend(unsigned int receiver_pid, void *buffer, int buffer_len);
extern int sysrecv(unsigned int *sender_pid, void *buffer, int buffer_len);
extern unsigned int syssleep(unsigned int ms);
extern int syssighandler(int signal, void (*newhandler)(void*), void (**oldhandler)(void*));
extern void syssigreturn(void *old_sp);
extern int syskill(int pid, int signal_number);
extern int syssigwait(void);
extern int sysopen(int device_no);
extern int sysclose(int fd);
extern int syswrite(int fd, void *buffer, int buffer_len);
extern int sysread(int fd, void *buffer, int buffer_len);
extern int sysioctl(int fd, unsigned long command, ...);


/* Functions in user.c */
extern void root(void);
extern void idleproc(void);


/* Functions in meg.c */
extern int send(unsigned int receiver_pid, void *buffer, int buffer_len, struct pcb *sender);
extern int recv(unsigned int *sender_pid, void *buffer, int buffer_len, struct pcb *receiver);


/* Functions in sleep.c */
extern void sleep(struct pcb *process, unsigned int ms);
extern void tick(void);

/* Functions in signal.c */
extern void sigtramp(void (*handler)(void*), void *cntx, void *osp);
extern int signal(int pid, int sig_no);

/* Functions in di_calls.c */
extern void di_init(void);
extern int di_open(struct pcb *process, int device_no);
extern int di_close(struct pcb *process, int fd);
extern int di_write(struct pcb *process, int fd, void *buffer, int buffer_len);
extern int di_read(struct pcb *process, int fd, void *buffer, int buffer_len);
extern int di_ioctl(struct pcb *process, int fd, unsigned long command, unsigned int args);
