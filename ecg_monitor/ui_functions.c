#include "ui_functions.h"
#include "widget_list.h"
#include "sys_functions.h"
#include "serial_functions.h"

GtkTextBuffer *serial_log_main_buf;

void ui_functions_init()
{
	serial_autoscroll = 1;
}

void add_buf_to_main_serial_log(char *buf, int lng)
{
	GtkTextIter iter_end;
	gtk_text_buffer_get_end_iter (serial_log_main_buf, &iter_end);
	gtk_text_buffer_insert(serial_log_main_buf, &iter_end, buf, lng);

	gtk_text_buffer_get_end_iter (serial_log_main_buf, &iter_end);
	gtk_text_iter_set_line_offset (&iter_end, 0);

	if(serial_autoscroll)
	{
		GtkTextIter *mark = gtk_text_buffer_get_mark(serial_log_main_buf, "scroll");
		gtk_text_buffer_move_mark (serial_log_main_buf, mark, &iter_end);

		gtk_text_view_scroll_mark_onscreen(serial_log_main, mark);
	}
}

void add_text_to_main_serial_log(char *text)
{
	int lng = 0;
	while(text[lng++]);
	add_buf_to_main_serial_log(text, lng-1);
}

void clear_serial_log()
{
	gtk_text_buffer_set_text (serial_log_main_buf, "", 0);
}

void interface_init()
{
	serial_log_main_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(serial_log_main));
	GtkTextIter iter_end;
	gtk_text_buffer_get_end_iter(serial_log_main_buf, &iter_end);
	gtk_text_buffer_create_mark(serial_log_main_buf, "scroll", &iter_end, 1);
}
