
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "widget_list.h"
#include "ui_base.h"
#include "ui_functions.h"
#include "serial_functions.h"

enum
{
	mode_serial = 0,
	mode_telnet
};

void main_loop()
{
	serial_main_loop();
}

int main( int   argc, char *argv[] )
{
    gtk_init (&argc, &argv);
    
	ui_functions_init();
	serial_functions_init();
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	create_widgets();
	interface_init();
	pack_containers();
	connect_signals();
	show_widgets();
    
	gtk_widget_set_usize(window, 1100, 650);

	serial_main_init();
	int main_id = g_timeout_add (30, main_loop, NULL);
    gtk_main ();
	device_close_log_file();
    return 0;
}