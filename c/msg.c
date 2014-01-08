/* msg.c : messaging system (assignment 2) */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>

/* Your code goes here */
struct pcb* find_process(int pid);
int process_in_sender_queue(struct pcb *process, struct pcb *sender_queue);
void add_process_to_sender_queue(struct pcb *sender, struct pcb *receiver);
void remove_process_from_sender_queue(struct pcb *sender, struct pcb *receiver);

int send(unsigned int receiver_pid, void *buffer, int buffer_len, struct pcb *sender)
{
  // invalid buffer_len or buffer address
  if (buffer_len < 0 || !buffer) {
    return -3;
  }

  if (sender->pid == receiver_pid) {
    return -2;
  }

  struct pcb *receiver = find_process(receiver_pid);
  if (!receiver) {
    return -1;
  }

  // if receiving process is not blocked waiting, or is not waiting to receive from this particular process, 
  // block the sender process to wait for the receiver
  if (receiver->state != BLOCKED || !process_in_sender_queue(sender, receiver->sender_queue)) {
    sender->state = BLOCKED;
    add_process_to_sender_queue(sender, receiver);
    return -3;
  }

  // else receiver has made a matching receive call
  // get receiving buffer and size from receiver's args
  va_list args = (va_list) receiver->args;
  va_arg(args, unsigned int);  // don't care about it
  void *receiver_buffer = va_arg(args, void*);
  int receiver_buffer_len = va_arg(args, int);
  // min(buffer_len, receiver_buffer_len)
  int buffer_limit = buffer_len > receiver_buffer_len ? receiver_buffer_len : buffer_len;
  if (buffer_limit > 0) {
    blkcopy(receiver_buffer, buffer, buffer_limit);
  }

  remove_process_from_sender_queue(sender, receiver);
  ready(receiver);

  return buffer_limit;
}

int recv(unsigned int *sender_pid, void *buffer, int buffer_len, struct pcb *receiver)
{
  if (buffer_len < 0 || !buffer) {
    return -3;
  }

  if (receiver->pid == *sender_pid) {
    return -2;
  }

  // if it's receive_all (*sender_pid == 0), then get the sender from the sender_queue
  // otherwise find it in pcb_table
  struct pcb *sender = (!*sender_pid) ? receiver->sender_queue : find_process(*sender_pid);
  if (!sender) {
    return -1;
  }

  // if sender is not blocked waiting, or is not waiting to send to this particular process,
  // block the receiver to wait for the sender
  if (sender->state != BLOCKED || !process_in_sender_queue(sender, receiver->sender_queue)) {
    receiver->state = BLOCKED;
    add_process_to_sender_queue(sender, receiver);
    return -3;
  }

  // if we get to here, then sender is available
  va_list args = (va_list) sender->args;
  va_arg(args, unsigned int);  // dont care about it
  void *sender_buffer = va_arg(args, void*);
  int sender_buffer_len = va_arg(args, int);
  // min(sender_buffer_len, buffer_len)
  int buffer_limit = buffer_len > sender_buffer_len ? sender_buffer_len : buffer_len;
  if (buffer_limit > 0) {
    blkcopy(buffer, sender_buffer, buffer_limit);
  }

  remove_process_from_sender_queue(sender, receiver);
  ready(sender);

  // update the sender pid for receive_all syscall
  if (!*sender_pid) {
    *sender_pid = sender->pid;
  }

  return buffer_limit;
}

// helper function to find a non-stopped process
struct pcb* find_process(int pid)
{
  // extract the right most 2 bytes, which is the index to the pcb table
  int pix = pid & 0xffff;
  // guard against malformed pid
  if (pix < 0 || pix >= PCB_TABLE_SIZE) {
    return 0;
  }

  struct pcb *process = &pcb_table[pix];
  return process->state != STOPPED ? process : 0;
}

// helper function to determine whether a process is in the sender queue
int process_in_sender_queue(struct pcb *process, struct pcb *sender_queue)
{
  struct pcb *iterator;
  for (iterator = sender_queue; iterator; iterator = iterator->sender_queue_next) {
    if (process->pid == iterator->pid) {
      return 1;
    }
  }

  return 0;
}

// helper function to add sending process to
// the sender_queue of receiving process
void add_process_to_sender_queue(struct pcb *sender, struct pcb *receiver)
{
  sender->sender_queue_next = 0;
  
  if (!receiver->sender_queue) {
    receiver->sender_queue = sender;
  } else {
    struct pcb *iterator;
    for (iterator = receiver->sender_queue; iterator->sender_queue_next; iterator = iterator->sender_queue_next);
    iterator->sender_queue_next = sender;
  }
}

// helper function to remove sender from receiver's sender queue
void remove_process_from_sender_queue(struct pcb *sender, struct pcb *receiver)
{
  struct pcb *sender_queue = receiver->sender_queue;
  if (!sender_queue) {
    return;
  }

  if (sender->pid == sender_queue->pid) {
    sender_queue = sender_queue->sender_queue_next;
  } else {
    struct pcb *iterator;
    for (iterator = sender_queue; iterator->sender_queue_next; iterator = iterator->sender_queue_next) {
      if (iterator->sender_queue_next->pid == sender->pid) {
        iterator->sender_queue_next = iterator->sender_queue_next->sender_queue_next;
        break;
      }
    }
  }

  receiver->sender_queue = sender_queue;
  sender->sender_queue_next = 0;
}
