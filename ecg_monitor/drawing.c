#include "drawing.h"
#include "device_functions.h"

uint32_t cnt = 0;
uint32_t pp = 0;

int draw_inited = 0;
GdkGC *gc;
int WX, WY;

//CSimpleChart gyro_charts[3];
//CSimpleChart mag_charts[3];

uint8_t *draw_buf;

void draw_init()
{
	draw_inited = 1;
	gc = gdk_gc_new(draw_area->window);
	WX = 800;
	WY = 600;
//	WY = 600;
	int cw = 750;
	emg_chart = &emg_c;
	spg_init(emg_chart, 600, 128);
	spg_set_viewport(emg_chart, 10, 10, cw, 400);
	spg_set_parameter_float(emg_chart, "color scale", 30);

	spg_init(emg_charts+0, 150, 32);
	spg_set_viewport(emg_charts+0, 10, 10, cw, 150);
	spg_set_parameter_float(emg_charts+0, "color scale", 30);

	spg_init(emg_charts+1, 150, 32);
	spg_set_viewport(emg_charts+1, 10, 170, cw, 150);
	spg_set_parameter_float(emg_charts+1, "color scale", 30);

	spg_init(emg_charts+2, 150, 32);
	spg_set_viewport(emg_charts+2, 10, 330, cw, 150);
	spg_set_parameter_float(emg_charts+2, "color scale", 30);

//	spg_set_parameter_str(emg_chart, "scaling", "adaptive");
	for(int N = 0; N < 8; N++)
	{
		sc_create_simple_chart(stress_hist+N, 200, 0);
		sc_create_simple_chart(ecg_charts+N, 500, 0); 
		sc_setViewport(ecg_charts+N, 10, 10 + 100*N, cw, 200);
		sc_setParameter_cc(ecg_charts+N, "scaling", "auto");
		sc_setParameter_cv(ecg_charts+N, "scale", 1000.0);
		int r, g, b;
		r = ((N+1)%2)*100;
		g = (N%4)*50;
		b = 250 - (N/4)*100;
		if(N == 0) r = 0, g = 255, b = 0;
		if(N == 1) r = 0, g = 150, b = 255;
		if(N == 2) r = 255, g = 0, b = 255;
		if(N == 3) r = 255, g = 255, b = 0;
		if(N == 4) r = 255, g = 0, b = 0;
		sc_setParameter_cl(ecg_charts+N, "color", r, g, b);
//		sc_setParameter_cl(ecg_charts+N, "axis color", r, g, b);
		sc_setParameter_cl(ecg_charts+N, "axis color", 255, 255, 255);
	}

	for(int N = 0; N < 8; N++)
	{
		sc_create_simple_chart(stress_charts+N, 500, 0); 
		sc_setViewport(stress_charts+N, 10, 10 + 100*N, cw, 200);
		sc_setParameter_cc(stress_charts+N, "scaling", "manual");
		sc_setParameter_cv(stress_charts+N, "scale", 0.5);
		sc_setParameter_cv(stress_charts+N, "zero value", 0.5);
		int r, g, b;
		r = ((N+1)%2)*100;
		g = (N%4)*50;
		b = 250 - (N/4)*100;
		if(N == 0) r = 0, g = 255, b = 0;
		if(N == 1) r = 0, g = 150, b = 255;
		if(N == 2) r = 255, g = 0, b = 255;
		if(N == 3) r = 255, g = 255, b = 0;
		if(N == 4) r = 255, g = 0, b = 0;
		sc_setParameter_cl(stress_charts+N, "color", r, g, b);
//		sc_setParameter_cl(stress_charts+N, "axis color", r, g, b);
		sc_setParameter_cl(stress_charts+N, "axis color", 255, 255, 255);
	}
	for(int N = 0; N < 1; N++)
	{
		sc_create_simple_chart(acc_charts+N, 2000, 0); 
		sc_setViewport(acc_charts+N, 10, 10 + 200*N, cw, 500);
//		sc_setParameter_cc(acc_charts+N, "scaling", "manual");
//		sc_setParameter_cv(acc_charts+N, "zero value", 0.0);
		sc_setParameter_cc(acc_charts+N, "scaling", "auto");
		sc_setParameter_cv(acc_charts+N, "scale", 1000.0);
		if(N == 0)
			sc_setParameter_cl(acc_charts+N, "color", 200, 100, 255);
		if(N == 1)
			sc_setParameter_cl(acc_charts+N, "color", 0, 50, 0);
			
		sc_setParameter_cl(acc_charts+N, "axis color", 0, 0, 0);		
	}
	sc_create_simple_chart(acc_charts+1, 2000, 0);
	sc_setViewport(acc_charts+1, 10, 10, cw, 500);
//	sc_setParameter_cc(acc_charts+1, "scaling", "manual");
	sc_setParameter_cc(acc_charts+1, "scaling", "auto");
	sc_setParameter_cv(acc_charts+1, "zero value", 0.0);
	sc_setParameter_cv(acc_charts+1, "scale", 1000.0);
	sc_setParameter_cl(acc_charts+1, "color", 128, 128, 128);
	sc_setParameter_cl(acc_charts+1, "axis color", 0, 0, 0);

	draw_buf = (uint8_t*)malloc(WX*WY*3);
}

float vv = 1.0;

void draw_loop()
{
	if(!draw_inited) draw_init();
	GdkColor clr;
//	clr.pixel = 0xFFFFFFFF;
	clr.pixel = 0xFF222222;
	gdk_gc_set_foreground(gc, &clr);
	gdk_draw_rectangle (draw_area->window, gc, 1, 0, 0, WX, WY);
	
	vv *= ((int)vv%3) + 1.05;
	if(vv > 50) vv *= 0.005;
	if(device_get_mode() == 1)
	{
		if(get_ble_draw_mode() == 0)
		{
			for(int N = 0; N < 5; N++)
				sc_draw_ln(ecg_charts+N, draw_area->window, gc, WX, WY);
		}
		else
		{
			if(get_ble_draw_hist() == 1)
			{
				clr.pixel = 0xFF22FF88;
				gdk_gc_set_foreground(gc, &clr);
				int hist_W = 150;
				for(int N = 0; N < 5; N++)
				{
					for(int x = 0; x < 19; x++)
					{
						float vv = sc_getV(stress_hist+N, 20-x);
						int hgt = vv*300.0;
						gdk_draw_rectangle(draw_area->window, gc, 1, 100+x*20, hist_W*N+100-hgt, 15, hgt);
					}
				}
			}
			else
			{
				for(int N = 0; N < 5; N++)
					sc_draw_ln(stress_charts+N, draw_area->window, gc, WX, WY);
			}
		}
	}
	else
	{
		if(draw_spectr)
		{
			int draw_X = WX;
			int draw_Y = WY;
			for(int x = 0; x < draw_X; x++)
			{
				for(int y = 0; y < draw_Y; y++)
				{
					int idx = (y*draw_X + x)*3;
					draw_buf[idx + 0] = 128;
					draw_buf[idx + 1] = 128;
					draw_buf[idx + 2] = 128;
				}
			}
			spg_draw(emg_chart, draw_buf, draw_X, draw_Y);
			gdk_draw_rgb_image (draw_area->window, draw_area->style->fg_gc[GTK_STATE_NORMAL],
				  0, 0, draw_X, draw_Y,
				  GDK_RGB_DITHER_MAX, draw_buf, draw_X * 3);			
			
		}
		else if(draw_emg)
		{
			int draw_X = WX;
			int draw_Y = WY;
			for(int x = 0; x < draw_X; x++)
			{
				for(int y = 0; y < draw_Y; y++)
				{
					int idx = (y*draw_X + x)*3;
					draw_buf[idx + 0] = 128;
					draw_buf[idx + 1] = 128;
					draw_buf[idx + 2] = 128;
				}
			}
			spg_draw(emg_charts+0, draw_buf, draw_X, draw_Y);
			spg_draw(emg_charts+1, draw_buf, draw_X, draw_Y);
			spg_draw(emg_charts+2, draw_buf, draw_X, draw_Y);
			gdk_draw_rgb_image (draw_area->window, draw_area->style->fg_gc[GTK_STATE_NORMAL],
				  0, 0, draw_X, draw_Y,
				  GDK_RGB_DITHER_MAX, draw_buf, draw_X * 3);			
			
		}
		else
		{
			for(int N = 0; N < 2; N++)
			{
				sc_draw_ln(acc_charts+N, draw_area->window, gc, WX, WY);
		//		sc_draw(gyro_charts+N, draw_area->window, gc, WX, WY);
			}
		}
	}
//	clr.pixel = 0xFF00FF00;
//	gdk_gc_set_foreground(gc, &clr);
//	gdk_draw_line(draw_area->window, gc, pp++, 10, 300, 300);
}
