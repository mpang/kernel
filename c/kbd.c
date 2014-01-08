#include <kbd.h>
#include <xeroskernel.h>
#include <stdarg.h>

unsigned int kbtoa(unsigned char code);
void enable_irq(unsigned int, int);

static int echo;
static unsigned char eof;
static int eof_flag;
static int enter_key_flag;
static unsigned char kbd_buffer[KBD_BUFFER_SIZE];
static int kbd_buffer_count;  // # of characters stored in kernel buffer
static unsigned char *app_buffer;
static int app_buffer_len;
static int app_buffer_count;  // # of characters stored in application buffer

// does the initialization for the keyboard
void kbd_init(void)
{
  eof = CRTL_D;
  eof_flag = 0;
  enter_key_flag = 0;
  app_buffer = 0;
  app_buffer_len = 0;
  app_buffer_count = 0;

  int i;
  for (i = 0; i < KBD_BUFFER_SIZE; i++) {
    kbd_buffer[i] = 0;
  }

  kbd_buffer_count = 0;
}

// @device_no 0 is non-echo version, 1 is echo version
int kbd_open(int device_no)
{
  echo = device_no;
  kbd_init();
  enable_irq(1, 0);
  return 0;
}

int kbd_close(void)
{
  enable_irq(1, 1);
  return 0;
}

int kbd_ioctl(unsigned long command, unsigned int args)
{
  va_list arg = (va_list) args;

  switch (command) {
    case 53:
      // only supports command to change eof character
      eof = (unsigned char) va_arg(arg, unsigned int);
      return 0;
    default:
      return -1;
  }
}

int kbd_write(void *buffer, int buffer_len)
{
  return -1;
}

// flush kernel buffer
// @index index of leftmost unread character in kernel buffer
//        4 means all data has been read
void flush_buffer(int index)
{
  // number of remaining characters in kernel buffer
  int remaining_chars = kbd_buffer_count - index;
  int i;
  // move remaining characters to the beginning of kernel buffer
  for (i = 0; i < remaining_chars; i++) {
    kbd_buffer[i] = kbd_buffer[index + i];
  }

  // set the rest of kernel buffer to 0
  for (; i < KBD_BUFFER_SIZE; i++) {
    kbd_buffer[i] = 0;
  }

  kbd_buffer_count = remaining_chars;
}

// copy kernel buffer to application buffer
void transfer_buffer(void)
{
  int transfer_done_early = 0;
  int i;
  for (i = 0; i < kbd_buffer_count && !transfer_done_early; i++) {
    unsigned char value = kbd_buffer[i];
    if (value == ENTER_KEY) {
      value = '\n';
      enter_key_flag = 1;
    }
    *app_buffer++ = value;
    app_buffer_count++;

    // either enter key is encountered, or app buffer is already full
    // which means sysread is succeeded
    if (enter_key_flag || app_buffer_count == app_buffer_len) {
      transfer_done_early = 1;
    }
  }

  flush_buffer(i);
}

// always copy from kernel buffer to application buffer
// @return -1 if error
//          0 if eof
//         -2 if process should be blocked
//          positive number if read succeeds (indicates bytes been read)
int kbd_read(void *buffer, int buffer_len)
{
  if (buffer_len < 0 || !buffer) {
    return -1;
  }

  if (eof_flag) {
    return 0;
  }

  app_buffer = buffer;
  app_buffer_len = buffer_len;
  // attempt to transfer from kernel buffer to application buffer
  // and let dispatcher know the result
  if (kbd_buffer_count > 0) {
    transfer_buffer();
  } else {
    return -2;
  }

  // bytes been read less than buffer_len, as well as no enter key entered
  // block the process since read is not completed yet
  if (app_buffer_count < app_buffer_len && !enter_key_flag) {
    return -2;
  }
  // read completes
  enter_key_flag = 0;
  int ret = app_buffer_count;
  app_buffer_count = 0;
  return ret;
}

void kbd_int_handler(void)
{
  // either eof has been encountered before, or kernel buffer is full
  if (eof_flag || kbd_buffer_count == KBD_BUFFER_SIZE) {
    return;
  }

  if (!(inb(0x64) & 0x1)) {
    // low order bit is 0 in port 0x64, which
    // indicates there's no data available
    return;
  }

  // read scan code from port 0x60 and convert to ascii value
  unsigned char value = (unsigned char) kbtoa(inb(0x60));
  if (!value) {
    // discard up key values
    return;
  }

  if (value == eof) {
    eof_flag = 1;
  } else {
    if (echo) {
      kprintf("%c\n", value);
    }

    kbd_buffer[kbd_buffer_count++] = value;
  }

  // either eof is encountered, carriage return is encountrered, kernel buffer is full, 
  // or application buffer will be filled up by current # of characters in kernel buffer
  if (app_buffer && (value == eof
                      || value == ENTER_KEY
                      || kbd_buffer_count == KBD_BUFFER_SIZE
                      || kbd_buffer_count + app_buffer_count == app_buffer_len)) {

    transfer_buffer();
    // unblock process since read is finished
    if (read_blocked_process && (eof_flag || enter_key_flag || app_buffer_count == app_buffer_len)) {
      read_blocked_process->ret = (eof_flag) ? 0 : app_buffer_count;
      ready(read_blocked_process);
      // reset counters, flags, buffers, etc
      enter_key_flag = 0;
      app_buffer_count = 0;
      app_buffer = 0;
      app_buffer_len = 0;
    }
  }
}
