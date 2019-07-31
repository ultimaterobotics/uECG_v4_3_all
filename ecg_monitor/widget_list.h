#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

GtkWidget *window;

GtkWidget *draw_area;

GtkWidget *bt_serial_open;
GtkWidget *bt_serial_clear;
GtkWidget *bt_serial_close;

GtkWidget *bt_draw_time;
GtkWidget *bt_draw_phase;
GtkWidget *bt_draw_centered;

GtkWidget *lb_rssi;
GtkWidget *lb_batt;

GtkWidget *lb_heart_rate;

GtkWidget *lb_stress_0;
GtkWidget *lb_stress_1;
GtkWidget *lb_stress_2;
GtkWidget *lb_stress_3;
GtkWidget *lb_stress_4;
GtkWidget *lb_stress_5;

GtkWidget *chk_auto_scale;
GtkWidget *chk_save_file;
GtkWidget *chk_ble_mode;
GtkWidget *chk_draw_stress;
GtkWidget *chk_stress_hist;
	
GtkWidget *serial_log_main_scroll;

GtkWidget *serial_bt_box;
GtkWidget *serial_io_box;
GtkWidget *label_box;
GtkWidget *stress_label_box;

GtkWidget *serial_entry_device;
GtkWidget *serial_log_main;

GtkWidget *main_page;
