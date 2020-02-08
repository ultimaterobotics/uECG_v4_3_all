#include "device_functions.h"
#include "serial_functions.h"
#include "ui_functions.h"

#include <poll.h>
#include <fcntl.h>
#include <sys/time.h>

//#include <linux/kernel.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
//#include <asm/ioctls.h>

#include "drawing.h"

struct timeval curTime, prevTime, zeroTime;
int device = 0;
double real_time = 0;
speed_t baudrate;

void serial_functions_init()
{
	baudrate = B921600;
//	baudrate = B230400;
}

void open_device()
{
	uint8_t *dev = gtk_entry_get_text(GTK_ENTRY(serial_entry_device));
	struct termios newtio;
	device = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	bzero(&newtio, sizeof(newtio));

	newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = 0;//IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;//ICANON;

	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 1;

	tcflush(device, TCIOFLUSH);
	tcsetattr(device, TCSANOW, &newtio);
	char txt[128];
	sprintf(txt, "device port open result: %d\n", device);
	printf("%s", txt);
	add_text_to_main_serial_log(txt);	
}

void close_device()
{
	close(device);
	device = 0;
	add_text_to_main_serial_log("device closed\n");	
}

uint8_t hamming_7_4_copmr(uint8_t data)
{
	uint8_t compressed = 0;
	uint8_t par[3];
	for(int pbit = 0; pbit < 3; pbit++)
		par[pbit] = 0;

	compressed = (data&0b1000)<<2;
	compressed |= (data&0b0111)<<1;

	uint8_t tot_par = 0;
	for(int bit = 0; bit < 8; bit++)
		tot_par ^= (compressed&(1<<bit))>0;

	for(int bit = 0; bit < 8; bit++)
	{
		int bp = bit;
		tot_par ^= (compressed&(1<<bit))>0;
		for(int pbit = 0; pbit < 3; pbit++)
		{
			if((bp>>pbit & 0b01)) par[pbit] ^= (compressed&(1<<bit))>0;
		}
	}
	compressed |= par[0]<<7;
	compressed |= par[1]<<6;
	compressed |= par[2]<<4;
	compressed |= tot_par;
	return compressed;
}


int hamming_57_63_decopmr(uint8_t *compressed, uint8_t *data)
{
	for(int x = 0; x < 7; x++) data[x] = 0;

	uint8_t par[6];
	for(int pbit = 0; pbit < 6; pbit++)
		par[pbit] = 0;

	uint8_t cpar[6];
	cpar[0] = compressed[0]>>7;
	cpar[1] = (compressed[0]>>6)&0b01;
	cpar[2] = (compressed[0]>>4)&0b01;
	cpar[3] = compressed[0]&0b01;
	cpar[4] = compressed[1]&0b01;
	cpar[5] = compressed[3]&0b01;

	uint8_t tot_par_dat = compressed[7]&0b01;
		
	compressed[0] &= 0b00101110;
	compressed[1] &= 0b11111110;
	compressed[3] &= 0b11111110;
	compressed[7] &= 0b11111110;

	uint8_t tot_par = 0;
	for(int cb = 0; cb < 8; cb++)
		for(int bit = 0; bit < 8; bit++)
			tot_par ^= (compressed[cb]&(1<<bit))>0;

	for(int cb = 0; cb < 8; cb++)
	{
		for(int bit = 0; bit < 8; bit++)
		{
			int bp = cb*8+bit;
			for(int pbit = 0; pbit < 6; pbit++)
			{
				if((bp>>pbit & 0b01)) par[pbit] ^= (compressed[cb]&(1<<bit))>0;
			}
		}
	}
	
	uint8_t err_pos = 0;
	uint8_t err_cnt = 0;
	for(int p = 0; p < 6; p++)
	{
		if(cpar[p] != par[p])
		{
			err_cnt++;
			err_pos |= 1<<p;
//			printf("parity %d = %d, in data %d\n", p, par[p], cpar[p]);
		}
	}
	if(err_cnt != 0 && tot_par == tot_par_dat)
		return 0;
//		printf("uncorrectable error at %d\n", err_pos);
	
	int byte = err_pos / 8;
	int bit = err_pos%8;
	if(compressed[byte] & (1<<bit))
		compressed[byte] &= ~(1<<bit);
	else
		compressed[byte] |= (1<<bit);
	
	data[0] = (compressed[0]&0b00100000)<<2;
	data[0] |= (compressed[0]&0b00001110)<<4;
	data[0] |= compressed[1]>>4;
	data[1] = (compressed[1]&0b1110)<<4;
	data[1] |= compressed[2]>>3;
	data[2] = (compressed[2]&0b111)<<5;
	data[2] |= compressed[3]>>3;
	data[3] = (compressed[3]&0b110)<<5;
	data[3] |= compressed[4]>>2;
	data[4] = (compressed[4]&0b11)<<6;
	data[4] |= compressed[5]>>2;
	data[5] = (compressed[5]&0b11)<<6;
	data[5] |= compressed[6]>>2;
	data[6] = (compressed[6]&0b11)<<6;
	data[6] |= compressed[7]>>2;
	
	return 1;
}


int hamming_7_4_decopmr(uint8_t compressed, uint8_t *data)
{
	*data = 0;

	uint8_t par[3];
	for(int pbit = 0; pbit < 3; pbit++)
		par[pbit] = 0;

	uint8_t cpar[3];
	cpar[0] = compressed>>7;
	cpar[1] = (compressed>>6)&0b01;
	cpar[2] = (compressed>>4)&0b01;

	uint8_t tot_par_dat = compressed&0b01;
		
	compressed &= 0b00101110;

	uint8_t tot_par = 0;
	for(int bit = 0; bit < 8; bit++)
		tot_par ^= (compressed&(1<<bit))>0;

	for(int bit = 0; bit < 8; bit++)
	{
		int bp = bit;
		for(int pbit = 0; pbit < 3; pbit++)
		{
			if((bp>>pbit & 0b01)) par[pbit] ^= (compressed&(1<<bit))>0;
		}
	}
	
	uint8_t err_pos = 0;
	uint8_t err_cnt = 0;
	for(int p = 0; p < 3; p++)
	{
		if(cpar[p] != par[p])
		{
			err_cnt++;
			err_pos |= 1<<p;
//			printf("parity %d = %d, in data %d\n", p, par[p], cpar[p]);
		}
	}
	if(err_cnt != 0 && tot_par == tot_par_dat)
		return 0;
//		printf("uncorrectable error at %d\n", err_pos);
	
	if(err_cnt != 0)
	{
		int bit = err_pos;
		if(compressed & (1<<bit))
			compressed &= ~(1<<bit);
		else
			compressed |= (1<<bit);
	}
	
	*data = (compressed&0b00100000)>>2;
	*data |= (compressed&0b00001110)>>1;
	
	return 1;
}

float ax_h[32];
float ay_h[32];
float az_h[32];
float gx_h[32];
float gy_h[32];
float gz_h[32];


float get_median(float *vals, int h)
{
	float tmp[128];
	for(int x = 0; x < h; x++) tmp[x] = vals[x];
	for(int x1 = 0; x1 < h; x1++)
		for(int x2 = x1+1; x2 < h; x2++)
		{
			if(tmp[x1] > tmp[x2])
			{
				float tt = tmp[x1];
				tmp[x1] = tmp[x2];
				tmp[x2] = tt;
			}
		}
	return tmp[h/2];
}

float med_filter(float v, float *hist, int hlen)
{
	float dv2, dd;
	for(int x = 0; x < hlen-1; x++)
	{
		dd = hist[x+1] - hist[x];
		dv2 += dd*dd;
	}
	dv2 /= (float)hlen;
	dd = v - hist[0];
	dd *= dd;
	float rv = v;
	if(dd > dv2*5.0) rv = get_median(hist, hlen);
	else
	{
		dd = v - hist[1];
		dd *= dd;
		if(dd > dv2*10.0) rv = get_median(hist, hlen);
	}
	
	for(int x = hlen-1; x > 0; x--)
		hist[x] = hist[x-1];
	hist[0] = v;
	return rv;
}

uint8_t main_inited = 0;

void serial_main_init()
{
	gettimeofday(&prevTime, NULL);
	gettimeofday(&zeroTime, NULL);
}
int hex_mode_pos = 0;
int added_lines_count = 0;
int serial_main_loop()
{
	draw_loop();
	if(!main_inited)
	{
		serial_main_init();
		main_inited = 1;
	}
	gettimeofday(&curTime, NULL);
	int dT = (curTime.tv_sec - prevTime.tv_sec) * 1000000 + (curTime.tv_usec - prevTime.tv_usec);
	real_time += (double)dT * 0.000001;

	prevTime = curTime;
	
	char rss[32];
	sprintf(rss, "RSSI %.1f", device_get_rssi());
//	sprintf(rss, "RSSI %.1f", 78.0);
	gtk_label_set_text(lb_rssi, rss);
 
	sprintf(rss, "%.2fv %g %g %g %d", device_get_battery(), device_get_ax(), device_get_ay(), device_get_az(), device_get_steps());
//	sprintf(rss, "batt %.2fv", 3.84);
	gtk_label_set_text(lb_batt, rss);
//	PangoAttrList *attrs = pango_attr_list_new();
//	PangoAttribute atrb;
//	atrb.klass = ;
//	pango_attr_list_insert(attrs, atrb);
//	pango_attr_list_unref(attrs);
	
	sprintf(rss, "BPM %d SGR %d", device_get_bpm(), device_get_skin_res());
//	sprintf(rss, "BPM %d", 68);
	gtk_label_set_text(lb_heart_rate, rss);
	
	if(device > 0)
	{
		struct pollfd pfdS;
		pfdS.fd = device;
		pfdS.events = POLLIN | POLLPRI;
//		char tmbuf[64];
//		int len = sprintf(tmbuf, "%g", real_time);
//		gtk_entry_set_text(GTK_ENTRY(entry_line), tmbuf);
		if(poll( &pfdS, 1, 1 ))
		{
			int lng = 0;
			uint8_t bbf[4096];
			uint8_t hex_bbf[16384];
			lng = read(device, bbf, 4096);
			if(lng > 0)
			{
				device_parse_response(bbf, lng);
				char *htext = (char*)hex_bbf;
				int htext_pos = 0;
				for(int p = 0; p < lng; p++)
				{
					htext_pos += sprintf(htext+htext_pos, "%02X ", bbf[p]);
					hex_mode_pos++;
					if(hex_mode_pos == 8 || hex_mode_pos == 24)
						htext_pos += sprintf(htext+htext_pos, " ");
					if(hex_mode_pos == 16)
						htext_pos += sprintf(htext+htext_pos, "   ");
					if(hex_mode_pos == 32)
					{
						htext_pos += sprintf(htext+htext_pos, "\n");
						hex_mode_pos = 0;
					}
				}
/*				add_buf_to_main_serial_log(htext, htext_pos);
				added_lines_count++;
				if(added_lines_count > 10000)
				{
					clear_serial_log();
					added_lines_count = 0;
				}*/
//				add_buf_to_main_serial_log(bbf, lng);
			}
		}
	}
	
	return 1;
}

uint8_t auto_scale = 1;

void toggle_auto_scale()
{
	auto_scale = !auto_scale;
	if(auto_scale)
	{
		for(int N = 0; N < 3; N++)
		{
			sc_setParameter_cc(acc_charts+N, "scaling", "auto");
//			sc_setParameter_cc(gyro_charts+N, "scaling", "auto");
		}
	}
	else
	{
		for(int N = 0; N < 3; N++)
		{
			sc_setParameter_cc(acc_charts+N, "scaling", "manual");
			sc_setParameter_cv(acc_charts+N, "zero value", 0.0);
			sc_setParameter_cv(acc_charts+N, "scale", 8000.0);
//			sc_setParameter_cc(gyro_charts+N, "scaling", "manual");
//			sc_setParameter_cv(gyro_charts+N, "zero value", 0.0);
//			sc_setParameter_cv(gyro_charts+N, "scale", 250.0);
		}
	}
}

int write_to_device(uint8_t *buf, int len)
{
	if(device > 0)
		return write(device, buf, len);
	return -1;
}

void send_data(uint8_t *data, int len)
{
	if(device > 0)
	{
		uint8_t buf[1234];

		int pp = 0;
		buf[pp++] = 29;
		buf[pp++] = 115;
		buf[pp++] = len; //payload length
		for(int x = 0; x < len; x++)
			buf[x+pp] = data[x];
		buf[len+pp] = 0;
		write(device, buf, pp+len+1);
	}
}