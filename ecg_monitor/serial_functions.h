#include <sys/stat.h>
#include <termios.h>

#include <errno.h>

#include <stdint.h>
#include "widget_list.h"

int write_to_device(uint8_t *buf, int len);

void serial_functions_init();
void open_device();
void close_device();

void serial_main_init();
int serial_main_loop();
void send_line();

void device_changed();

void toggle_auto_scale();
void toggle_integrate();
