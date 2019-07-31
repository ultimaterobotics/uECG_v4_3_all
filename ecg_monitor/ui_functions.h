#include "widget_list.h"

int serial_send_nl;
int serial_send_cr;
int serial_autoscroll;
int serial_show_hex;

int telnet_autoscroll;

void ui_functions_init();
void serial_toggle_nl();
void serial_toggle_cr();
void serial_toggle_autoscroll();
void serial_toggle_hex();
void add_buf_to_main_serial_log(char *buf, int lng);
void add_text_to_main_serial_log(char *text);
void clear_serial_log();
void interface_init();
void serial_line_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

void clear_telnet_log();
void add_text_to_main_telnet_log(char *text);
void add_buf_to_main_telnet_log(char *buf, int lng);
void telnet_toggle_autoscroll();