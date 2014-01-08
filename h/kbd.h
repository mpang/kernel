#define CRTL_D 0x04  // control-d
#define ENTER_KEY 0x0a
#define KBD_BUFFER_SIZE 4

extern void kbd_int_handler(void);
extern int kbd_open(int device_no);
extern int kbd_close(void);
extern int kbd_write(void *buffer, int buffer_len);
extern int kbd_read(void *buffer, int buffer_len);
extern int kbd_ioctl(unsigned long command, unsigned int args);
