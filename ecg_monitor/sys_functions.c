#include "sys_functions.h"

int delete_event( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
    g_print ("delete event occurred\n");
	gtk_main_quit ();
    return TRUE;
}

void destroy( GtkWidget *widget, gpointer   data )
{
    gtk_main_quit ();
}
