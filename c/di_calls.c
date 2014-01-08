#include <xeroskernel.h>
#include <kbd.h>

int other_device_open(void);

void di_init(void)
{
  int i;
  for (i = 0; i < NUM_OF_DEVICE; i++) {
    device_table[i].major_no = i;
    device_table[i].open = &kbd_open;
    device_table[i].close = &kbd_close;
    device_table[i].write = &kbd_write;
    device_table[i].read = &kbd_read;
    device_table[i].ioctl = &kbd_ioctl;
  }

  kprintf("devices initialized\n");
}

int di_open(struct pcb *process, int device_no)
{
  if (device_no < 0 || device_no >= NUM_OF_DEVICE) {
    return -1;
  }

  // only a single device can be open at a time
  if (other_device_open()) {
    return -1;
  }

  int i;
  for (i = 0; i < FD_TABLE_SIZE; i++) {
    if (process->fd_table[i] == -1) {
      int result = device_table[device_no].open(device_no);
      if (result == -1) {
        // failure in opening device
        return -1;
      }

      process->fd_table[i] = device_no;
      return i;
    }
  }

  // couldn't find an empty slot in fd_table
  return -1;
}

// close the device associated with fd
// subsequent system calls (including sysclose) returns error
int di_close(struct pcb *process, int fd)
{
  if (fd < 0 || fd >= FD_TABLE_SIZE) {
    return -1;
  }

  // fd already closed
  if (process->fd_table[fd] == -1) {
    return -1;
  }

  int result = device_table[process->fd_table[fd]].close();
  process->fd_table[fd] = -1;
  return result;
}

int di_write(struct pcb *process, int fd, void *buffer, int buffer_len)
{
  if (fd < 0 || fd >= FD_TABLE_SIZE) {
    return -1;
  }

  if (process->fd_table[fd] == -1) {
    return -1;
  }

  return device_table[process->fd_table[fd]].write(buffer, buffer_len);
}

int di_read(struct pcb *process, int fd, void *buffer, int buffer_len)
{
  if (fd < 0 || fd >= FD_TABLE_SIZE) {
    return -1;
  }

  if (process->fd_table[fd] == -1) {
    return -1;
  }

  return device_table[process->fd_table[fd]].read(buffer, buffer_len);
}

int di_ioctl(struct pcb *process, int fd, unsigned long command, unsigned int args)
{
  if (fd < 0 || fd >= FD_TABLE_SIZE) {
    return -1;
  }

  if (process->fd_table[fd] == -1) {
    return -1;
  }

  return device_table[process->fd_table[fd]].ioctl(command, args);
}

// return 1 if other device is opened, 0 otherwise
int other_device_open(void)
{
  int i;
  for (i = 0; i < PCB_TABLE_SIZE; i++) {
    if (pcb_table[i].state != STOPPED) {
      int j;
      for (j = 0; j < FD_TABLE_SIZE; j++) {
        if (pcb_table[i].fd_table[j] > -1) {
          return 1;
        }
      }
    }
  }

  return 0;
}
