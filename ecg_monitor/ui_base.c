#include "widget_list.h"
#include "ui_base.h"
#include "ui_functions.h"
#include "sys_functions.h"
#include "serial_functions.h"
#include "device_functions.h"

void create_widgets()
{
	main_page = gtk_hbox_new (0, 10);
    serial_bt_box = gtk_vbox_new (0, 10);
	serial_io_box = gtk_vbox_new (0, 10);
	label_box = gtk_hbox_new (0, 10);
	stress_label_box = gtk_vbox_new (0, 10);

    bt_serial_open = gtk_button_new_with_label ("Open");
    bt_serial_close = gtk_button_new_with_label ("Close");
    bt_serial_clear = gtk_button_new_with_label ("Clear");
	
	bt_draw_time = gtk_button_new_with_label ("Time series");
	bt_draw_phase = gtk_button_new_with_label ("Phasegram");
	bt_draw_centered = gtk_button_new_with_label ("Pulse centered");
	
	lb_rssi = gtk_label_new("");
	lb_batt = gtk_label_new("");
	lb_heart_rate = gtk_label_new("");

	lb_stress_0 = gtk_label_new("0");
	lb_stress_1 = gtk_label_new("1");
	lb_stress_2 = gtk_label_new("2");
	lb_stress_3 = gtk_label_new("3");
	lb_stress_4 = gtk_label_new("4");
	lb_stress_5 = gtk_label_new("5");
	
	chk_auto_scale = gtk_check_button_new_with_label("auto scale");
	gtk_button_clicked(chk_auto_scale);
	chk_save_file = gtk_check_button_new_with_label("save to file");
	chk_ble_mode = gtk_check_button_new_with_label("BLE mode");
	chk_draw_stress = gtk_check_button_new_with_label("draw stress");
	chk_stress_hist = gtk_check_button_new_with_label("hist mode");
		    
	serial_log_main = gtk_text_view_new();
	
	draw_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(draw_area, 800, 600);
	
	serial_entry_device = gtk_entry_new();
    gtk_entry_set_text (GTK_ENTRY (serial_entry_device), "/dev/ttyUSB0");
}

void connect_signals()
{
    g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), NULL);
    g_signal_connect (window, "destroy", G_CALLBACK (destroy), NULL);
	
    g_signal_connect(bt_serial_open, "clicked", G_CALLBACK (open_device), NULL);
    g_signal_connect(bt_serial_close, "clicked", G_CALLBACK (close_device), NULL);
	
	g_signal_connect (chk_save_file, "clicked", G_CALLBACK (device_toggle_file_save), NULL);
	g_signal_connect (chk_auto_scale, "clicked", G_CALLBACK (toggle_auto_scale), NULL);
	g_signal_connect (chk_ble_mode, "clicked", G_CALLBACK (device_toggle_ble_mode), NULL);	
	g_signal_connect (chk_draw_stress, "clicked", G_CALLBACK (device_toggle_stress_mode), NULL);
	g_signal_connect (chk_stress_hist, "clicked", G_CALLBACK (device_toggle_stress_hist), NULL);

    g_signal_connect (bt_serial_clear, "clicked", G_CALLBACK (clear_serial_log), window);
	
    g_signal_connect (bt_draw_time, "clicked", G_CALLBACK (device_draw_time), window);
    g_signal_connect (bt_draw_phase, "clicked", G_CALLBACK (device_draw_phase), window);
    g_signal_connect (bt_draw_centered, "clicked", G_CALLBACK (device_draw_centered), window);
}

void pack_containers()
{
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
	
    gtk_box_pack_start (GTK_BOX (serial_bt_box), serial_entry_device, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), bt_serial_open, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), bt_serial_close, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), bt_serial_clear, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), bt_draw_time, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), bt_draw_phase, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), bt_draw_centered, 0, 0, 0);

	int vpad = 40;
    gtk_box_pack_start (GTK_BOX (stress_label_box), lb_stress_0, 0, 0, vpad/4);
    gtk_box_pack_start (GTK_BOX (stress_label_box), lb_stress_1, 0, 0, vpad);
    gtk_box_pack_start (GTK_BOX (stress_label_box), lb_stress_2, 0, 0, vpad);
    gtk_box_pack_start (GTK_BOX (stress_label_box), lb_stress_3, 0, 0, vpad);
    gtk_box_pack_start (GTK_BOX (stress_label_box), lb_stress_4, 0, 0, vpad);
    gtk_box_pack_start (GTK_BOX (stress_label_box), lb_stress_5, 0, 0, vpad);

    gtk_box_pack_start (GTK_BOX (label_box), lb_rssi, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (label_box), lb_batt, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), label_box, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), lb_heart_rate, 0, 0, 0);
	
    gtk_box_pack_start (GTK_BOX (serial_bt_box), chk_auto_scale, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), chk_save_file, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), chk_ble_mode, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), chk_draw_stress, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (serial_bt_box), chk_stress_hist, 0, 0, 0);

	serial_log_main_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(serial_log_main_scroll), serial_log_main); 

    gtk_box_pack_start(GTK_BOX(serial_io_box), draw_area, 1, 0, 0);
	gtk_box_pack_start(GTK_BOX(serial_io_box), serial_log_main_scroll, 1, 1, 0);
	
    gtk_box_pack_start (GTK_BOX (main_page), serial_io_box, 1, 1, 0);
    gtk_box_pack_start(GTK_BOX(main_page), stress_label_box, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (main_page), serial_bt_box, 0, 0, 0);

    gtk_container_add (GTK_CONTAINER (window), main_page);
	
}
void show_widgets()
{
    gtk_widget_show (bt_serial_open);
    gtk_widget_show (bt_serial_close);
    gtk_widget_show (bt_serial_clear);
    gtk_widget_show (bt_draw_time);
    gtk_widget_show (bt_draw_phase);
    gtk_widget_show (bt_draw_centered);
//    gtk_widget_show (bt_serial_discover);
 //   gtk_widget_show (cb_devices);
//	gtk_widget_show (bt_serial_set);
//	gtk_widget_show (bt_serial_noleds);
//	gtk_widget_show (bt_set_zero);

	gtk_widget_show (lb_rssi);
	gtk_widget_show (lb_batt);
	gtk_widget_show (lb_heart_rate);

	gtk_widget_show (lb_stress_0);
	gtk_widget_show (lb_stress_1);
	gtk_widget_show (lb_stress_2);
	gtk_widget_show (lb_stress_3);
	gtk_widget_show (lb_stress_4);
	gtk_widget_show (lb_stress_5);

	gtk_widget_show (chk_auto_scale);
	gtk_widget_show (chk_save_file);
	gtk_widget_show (chk_ble_mode);
	gtk_widget_show (chk_draw_stress);
	gtk_widget_show (chk_stress_hist);

    gtk_widget_show (serial_log_main_scroll);
    gtk_widget_show (serial_log_main);
    gtk_widget_show (serial_entry_device);
	
    gtk_widget_show (draw_area);
    gtk_widget_show (serial_io_box);
	gtk_widget_show (serial_bt_box);
	gtk_widget_show (label_box);
	gtk_widget_show (stress_label_box);

	gtk_widget_show (main_page);

    /* and the window */
    gtk_widget_show (window);
	
}