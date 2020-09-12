/**
 */

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
//#include "nrf_delay.h"
//#include "nrf_gpiote.h"
//#include "boards.h"
#include "mcp3911.h"
#include "bmi160.h"
#include "leds.h"
#include "ble_advertising.h"
#include "fast_radio.h"
#include "timer_core.h"

//openocd -f interface/stlink-v2.cfg -f target/nrf52.cfg
//flash write_image erase _build/nrf52832_xxaa.hex

uint8_t pin_button = 26;

float battery_mv = 3500; //by default non zero to prevent power off before actual measurement
int battery_low_threshold = 3200;

uint8_t battery_level = 0;
uint8_t packet_id = 0;
uint8_t adc_working = 0;
uint8_t data_packet[128];

uint8_t enable_led_indication = 1;

int dbg_val = 0;

uint8_t unsent_cnt = 0;
int mcp_buffer[128];
uint8_t mcp_buf_pos = 0;
uint8_t mcp_buf_len = 32;
uint8_t mcp_cnt = 0;

uint32_t spectr_scale = 0;
uint8_t spectr_buf[64];

int mcp_buffer_ble[128];
uint8_t mcp_buf_ble_pos = 0;
uint8_t mcp_buf_ble_len = 32;

int rr_hist_len = 2500;
int rr_hist_pos = 0;
uint16_t RR_hist[2500];

int16_t spectr_val = 0;
uint8_t sp_center_avg = 0;

int out_data_mode = 0;
int emg_mode = 0;

enum
{
	led_out_off = 0,
	led_out_peaks,
	led_out_peaks_inv,
	led_out_analog,
	led_out_analog_inv,
	led_out_maxvalue
};

void rr_init()
{
	for(int x = 0; x < rr_hist_len; x++)
	{
		RR_hist[x] = 0;
	}
}

int BPM = 0;

void update_BPM()
{
	float beats = 20;
	float DT = 0;
	int hpos = rr_hist_pos - 1;
	for(int x = 0; x < beats; x++)
	{
		if(hpos < 0) hpos += rr_hist_len;
		DT += RR_hist[hpos];
		hpos--;
	}
	DT /= 1000.0;
	BPM = 60.0 * beats / DT;
	return;
}

float std_avg = 0.001;
float short_avg = 0.01;
float long_avg = 0.001;
float rr_avg = 0.001;

float avg_RR = 0;

float SDNN = 0;
float RMSSD = 0;
int pNN_bins = 15;
float pNN_short[15];
float pNN_short_norm[15];

void update_HRV()
{
	int hpos = rr_hist_pos - 1;
	if(hpos < 0) hpos += rr_hist_len;
	float rr = RR_hist[hpos];
	hpos--;
	if(hpos < 0) hpos += rr_hist_len;
	float rr2 = RR_hist[hpos];
	
	if(rr < 200 || rr > 2000 || rr2 < 200 || rr2 > 2000) return; //bad RR, no update
	if(avg_RR < 0.0001) avg_RR = rr; //init
	
	avg_RR += rr_avg*rr;
	avg_RR *= 1.0 - rr_avg;

	float aRR = rr - avg_RR;
	SDNN += std_avg*aRR*aRR;
	SDNN *= 1.0 - std_avg;
	
	float dRR = rr - rr2;
	if(dRR < 0) dRR = -dRR;
	float cur_avg_RR = 60.0 / (float)BPM;
	int dRR_bin = dRR / (10.0 * cur_avg_RR);
	if(dRR_bin >= pNN_bins) dRR_bin = pNN_bins-1;

	for(int x = 0; x < pNN_bins; x++)
	{
		pNN_short[x] *= 1.0 - short_avg;
	}
	pNN_short[dRR_bin] += 1.0;

	float sum_short = 0.000001;
	for(int x = 0; x < pNN_bins; x++)
	{
		sum_short += pNN_short[x];
	}
	for(int x = 0; x < pNN_bins; x++)
	{
		pNN_short_norm[x] = pNN_short[x] / sum_short;
	}

	hpos--;
	if(hpos < 0) hpos += rr_hist_len;
	float rr3 = RR_hist[hpos];

	if(rr3 > 200 && rr3 < 2000)
	{
		float ddRR = rr2 - rr3;
		if(ddRR < 0) ddRR = -ddRR;
		float dif_rr = dRR - ddRR;
		RMSSD *= 1.0 - std_avg;
		RMSSD += std_avg*dif_rr*dif_rr;
	}	
}

uint8_t send_rr_id = 0;

void push_RR(uint32_t RR)
{
	RR_hist[rr_hist_pos] = RR;
	rr_hist_pos++;
	if(rr_hist_pos >= rr_hist_len)
		rr_hist_pos = 0;
	
	send_rr_id++;
	if(send_rr_id > 0x0F) send_rr_id = 0;
	
	update_BPM();
	update_HRV();
}


uint8_t led_out_mode = led_out_peaks;
uint8_t emg_data_id = 0;


uint8_t data_id = 0;
uint8_t data_id_ble = 0;
uint8_t param_send_id = 0;

enum param_sends
{
	param_batt_bpm = 0,
	param_sdnn,
	param_skin_res,
	param_lastRR,
	param_imu_acc,
	param_imu_steps,
	param_pnn_bins,
	param_end,
	param_emg_spectrum
};

int send_rr_cnt = 0;
float skin_ble_avg = 0;
int bin_send_id = 0;
float g_activity = 0;

float encode_acc(float acc)
{
	float shift = 0, dA = 0, coeff = 1;
	float am = acc;
	if(am < 0) am = -am;
	if(am < 2)
	{
		shift = 0;
		dA = 0;
		coeff = 25.0;
	}
	else if (am < 12)
	{
		shift = 50;
		dA = 2.0;
		coeff = 5.0;
	}
	else 
	{
		shift = 100;
		dA = 12.0;
		coeff = 2.0;
	}
	float res = shift + (am - dA) * coeff;
	if(acc < 0) res = -res;
	return res;
}

void prepare_data_packet()
{
	uint8_t idx = 0;
	data_packet[idx++] = 0; //fill in the end
	packet_id++;
	if(packet_id > 128) packet_id = 0;
	data_packet[idx++] = packet_id;
	uint32_t unit_id = NRF_FICR->DEVICEID[1];
	data_packet[idx++] = (unit_id>>24)&0xFF;
	data_packet[idx++] = (unit_id>>16)&0xFF;
	data_packet[idx++] = (unit_id>>8)&0xFF;
	data_packet[idx++] = (unit_id)&0xFF;
	uint8_t send_cnt = unsent_cnt;
//	if(send_cnt > 4) send_cnt = 4;
//	if(send_cnt < 1) send_cnt = 1;
	if(emg_mode)
	{
		data_packet[idx++] = 32;
		data_packet[idx++] = battery_level;
		data_packet[idx++] = emg_data_id;
		data_packet[idx++] = sp_center_avg;
		data_packet[idx++] = sp_center_avg;
		if(spectr_scale > 0xFFFF) spectr_scale = 0xFFFF;
		data_packet[idx++] = (spectr_scale>>8)&0xFF;
		data_packet[idx++] = spectr_scale&0xFF;
		for(int n = 0; n < 32; n++)
			data_packet[idx++] = spectr_buf[n];
		data_packet[0] = idx+1;
		uint8_t check = 0;
		for(int x = 0; x < idx; x++)
			check += data_packet[x];
		data_packet[idx] = check;
		return;	
	}
	data_packet[idx++] = send_cnt;
	int cur_send_id;
	param_send_id++;
	if(param_send_id == param_end) param_send_id = 0;
	cur_send_id = param_send_id;

	data_packet[idx++] = cur_send_id;
	if(cur_send_id == param_batt_bpm)
	{
		data_packet[idx++] = battery_level;
		data_packet[idx++] = 5; //version_id
		data_packet[idx++] = BPM;
	}
	if(cur_send_id == param_sdnn)
	{
		int v1 = sqrt(SDNN);
		int v2 = sqrt(RMSSD);
		if(v1 > 0xFFF) v1 = 0xFFF;
		if(v2 > 0xFFF) v2 = 0xFFF;
		uint8_t v1_h = (v1>>8)&0x0F;
		uint8_t v1_l = v1&0xFF;
		uint8_t v2_h = (v2>>8)&0x0F;
		uint8_t v2_l = v2&0xFF;
		data_packet[idx++] = (v1_h<<4) | v2_h;
		data_packet[idx++] = v1_l;
		data_packet[idx++] = v2_l;
	}
	if(cur_send_id == param_skin_res)
	{
		int v = mcp_get_filtered_skin();//skin_ble_avg;
		data_packet[idx++] = v>>8;
		data_packet[idx++] = v&0xFF;
		float temp = bmi.T;
		int ti = temp;
		uint8_t t8 = 0;
		if(ti < -20) ti = -20;
		if(ti > 77) ti = 77;
		if(ti < 30) t8 = 20 + ti;
		if(ti > 47) t8 = 220 + (ti-47);
		if(ti >= 30 && ti <= 47)
		{
			t8 = 50 + (temp-30.0)*10.0;
		}
		data_packet[idx++] = t8;
	}
	if(cur_send_id == param_lastRR)
	{
		int hpos = rr_hist_pos - 1;
		int v1 = RR_hist[hpos];
		uint8_t v1_h = (v1>>8)&0xFF;
		uint8_t v1_l = v1&0xFF;
		data_packet[idx++] = send_rr_id;
		data_packet[idx++] = v1_h;
		data_packet[idx++] = v1_l;
	}
	if(cur_send_id == param_imu_acc)
	{
		int ax = 128 + encode_acc(bmi.aX);
		if(ax < 0) ax = 0;
		if(ax > 255) ax = 255;
		int ay = 128 + encode_acc(bmi.aY);
		if(ay < 0) ay = 0;
		if(ay > 255) ay = 255;
		int az = 128 + encode_acc(bmi.aZ);
		if(az < 0) az = 0;
		if(az > 255) az = 255;
		data_packet[idx++] = ax;
		data_packet[idx++] = ay;
		data_packet[idx++] = az;
	}
	if(cur_send_id == param_imu_steps)
	{
		int steps = bmi.step_cnt;
		int g_byte = g_activity * 10.0;
		if(g_byte > 255) g_byte = 255;
		data_packet[idx++] = (steps>>8)&0xFF;
		data_packet[idx++] = steps&0xFF;
		data_packet[idx++] = g_byte;
	}
	if(cur_send_id == param_pnn_bins)
	{
		int bin_v1 = pNN_short_norm[bin_send_id] * 255.0;
		int bin_v2 = pNN_short_norm[bin_send_id+1] * 255.0;
		data_packet[idx++] = bin_send_id;
		data_packet[idx++] = bin_v1;
		data_packet[idx++] = bin_v2;
		bin_send_id += 2;
		if(bin_send_id >= 15) bin_send_id = 0;
	}
		
	int n = 0;
	int pp = mcp_buf_pos - send_cnt - 1;
	if(pp < 0) pp += mcp_buf_len;
	while(n++ < send_cnt)
	{
		data_packet[idx++] = (mcp_buffer[pp] >> 8)&0xFF;
		data_packet[idx++] = (mcp_buffer[pp]) & 0xFF;
		if(++pp >= mcp_buf_len) pp = 0;
	}

	int16_t v = mcp_get_filtered_skin();//skin_ble_avg;
	data_packet[idx++] = v>>8;
	data_packet[idx++] = v&0xFF;

	data_packet[0] = idx+2;
	uint8_t check = 0;
	for(int x = 0; x < idx; x++)
		check += data_packet[x];
	data_packet[idx++] = check;
	uint8_t check_e = 0;
	for(int x = 0; x < idx; x+=2)
		check_e += data_packet[x];
	data_packet[idx] = check_e;
}

void prepare_data_packet_ble()
{
	data_packet[0] = data_id_ble;
	if(emg_mode)
	{
		data_packet[1] = param_emg_spectrum;
		data_packet[2] = battery_level;
		data_packet[3] = emg_data_id;
		data_packet[4] = sp_center_avg;
		if(spectr_scale > 0xFFFF) spectr_scale = 0xFFFF;
		data_packet[5] = (spectr_scale>>8)&0xFF;
		data_packet[6] = spectr_scale&0xFF;
		//14 bytes left, so pack them carefully
		int idx = 7;
		for(int n = 1; n < 5; n++) //first 4 directly
			data_packet[idx++] = spectr_buf[n];
		//10 bytes left, packing sp[5]...sp[14]
		for(int n = 0; n < 5; n++)
			data_packet[idx++] = (spectr_buf[5+n*2] + spectr_buf[5+n*2+1])/2;
		//5 bytes left, packing sp[15]...sp[29]
		for(int n = 0; n < 5; n++)
			data_packet[idx++] = (spectr_buf[15+n*3] + spectr_buf[15+n*3+1] + spectr_buf[15+n*3+2])/3;
		return;
	}
	int skin_res = mcp_get_filtered_skin();
	skin_ble_avg *= 0.98;
	skin_ble_avg += 0.02*skin_res;
	int cur_send_id;
	param_send_id++;
	if(param_send_id == param_end) param_send_id = 0;
	cur_send_id = param_send_id;

	data_packet[1] = cur_send_id;
	if(cur_send_id == param_batt_bpm)
	{
		data_packet[2] = battery_level;
		data_packet[3] = 5; //version_id
		data_packet[4] = BPM;
	}
	if(cur_send_id == param_sdnn)
	{
		int v1 = sqrt(SDNN);
		int v2 = sqrt(RMSSD);
		if(v1 > 0xFFF) v1 = 0xFFF;
		if(v2 > 0xFFF) v2 = 0xFFF;
		uint8_t v1_h = (v1>>8)&0x0F;
		uint8_t v1_l = v1&0xFF;
		uint8_t v2_h = (v2>>8)&0x0F;
		uint8_t v2_l = v2&0xFF;
		data_packet[2] = (v1_h<<4) | v2_h;
		data_packet[3] = v1_l;
		data_packet[4] = v2_l;
	}
	if(cur_send_id == param_skin_res)
	{
		int v = skin_ble_avg;
		data_packet[2] = v>>8;
		data_packet[3] = v&0xFF;
		float temp = bmi.T;
		int ti = temp;
		uint8_t t8 = 0;
		if(ti < -20) ti = -20;
		if(ti > 77) ti = 77;
		if(ti < 30) t8 = 20 + ti;
		if(ti > 47) t8 = 220 + (ti-47);
		if(ti >= 30 && ti <= 47)
		{
			t8 = 50 + (temp-30.0)*10.0;
		}
		data_packet[4] = t8;
	}
	if(cur_send_id == param_lastRR)
	{
		int hpos = rr_hist_pos - 1;
		if(hpos < 0) hpos += rr_hist_len;
		int hpos2 = hpos - 1;
		if(hpos2 < 0) hpos2 += rr_hist_len;
		int v1 = RR_hist[hpos];
		int v2 = RR_hist[hpos2];
		if(v1 > 0xFFF) v1 = 0xFFF;
		if(v2 > 0xFFF) v2 = 0xFFF;
		uint8_t v1_h = (v1>>8)&0x0F;
		uint8_t v1_l = v1&0xFF;
		uint8_t v2_h = (v2>>8)&0x0F;
		uint8_t v2_l = v2&0xFF;
		data_packet[1] |= send_rr_id<<4;
		data_packet[2] = (v1_h<<4) | v2_h;
		data_packet[3] = v1_l;
		data_packet[4] = v2_l;
	}
	if(cur_send_id == param_imu_acc)
	{
		int ax = 128 + encode_acc(bmi.aX);
		if(ax < 0) ax = 0;
		if(ax > 255) ax = 255;
		int ay = 128 + encode_acc(bmi.aY);
		if(ay < 0) ay = 0;
		if(ay > 255) ay = 255;
		int az = 128 + encode_acc(bmi.aZ);
		if(az < 0) az = 0;
		if(az > 255) az = 255;
		data_packet[2] = ax;
		data_packet[3] = ay;
		data_packet[4] = az;
	}
	if(cur_send_id == param_imu_steps)
	{
		int steps = bmi.step_cnt;
		int g_byte = g_activity * 10.0;
		if(g_byte > 255) g_byte = 255;
		data_packet[2] = (steps>>8)&0xFF;
		data_packet[3] = steps&0xFF;
		data_packet[4] = g_byte;
	}

	if(cur_send_id == param_pnn_bins)
	{
		int bin_v1 = pNN_short_norm[bin_send_id] * 255.0;
		int bin_v2 = pNN_short_norm[bin_send_id+1] * 255.0;
		int bin_v3 = pNN_short_norm[bin_send_id+2] * 255.0;
		data_packet[1] |= (bin_send_id<<4);
		bin_send_id += 3;
		if(bin_send_id >= 13) bin_send_id = 0;
		data_packet[2] = bin_v1;
		data_packet[3] = bin_v2;
		data_packet[4] = bin_v3;
	}
	int send_seq_cnt = 30; //definitely enough for any BLE4 case
	int pp = mcp_buf_ble_pos - send_seq_cnt - 1;
	if(pp < 0) pp += mcp_buf_ble_len;
	uint8_t idx = 5;
	int zero_val = mcp_buffer_ble[pp];
	data_packet[idx++] = (zero_val >> 8)&0xFF;
	data_packet[idx++] = (zero_val) & 0xFF;
	int start_pp = pp;
	int prev_vv = zero_val;
	int cur_scale = 1;
	int max_dv = 0;
	for(int x = 0; x < send_seq_cnt; x++)
	{
		if(++pp >= mcp_buf_ble_len) pp = 0;
		int vv = mcp_buffer_ble[pp];
		int dv = vv - prev_vv;
		if(dv < 0) dv = -dv;
		if(dv > max_dv) max_dv = dv;
		prev_vv = vv;
	}
	cur_scale = 1 + (max_dv / 127);
	int scale_code = cur_scale;
	if(scale_code > 100) scale_code = 100 + (scale_code-100) / 4;
	data_packet[idx++] = scale_code;
	pp = start_pp;
	prev_vv = zero_val;
	for(int x = 0; x < send_seq_cnt; x++)
	{
		if(++pp >= mcp_buf_ble_len) pp = 0;
		int vv = mcp_buffer_ble[pp];
		int dv = (vv - prev_vv) / cur_scale;
		data_packet[idx++] = dv&0xFF;
		prev_vv = vv;
	}
}

float avg_v = 0;
float avg_dv = 0;
float avg_dv_f = 0;

int prev_v = 0;
float prev_peak = 0;
int prev_peak_cnt = 0;

float avg_base = 0;
float avg_local = 0;
float avg_max = 0;
float avg_min = 0;
float loc_max = 0;
float loc_min = 0;

int peak_in = 0;

uint32_t peak_start_ms = 0;
uint32_t prev_peak_start = 0;

int val_ble = 0;
int val_ble_z = 0;

void h2color(int h, uint8_t *r, uint8_t *g, uint8_t *b) //HSV color model with S=V=100
{
	uint8_t hi = (h/60)%6;
	uint8_t a = (h%60)*51/12; //a: 0...255
	uint8_t Vinc = a;
	uint8_t Vdec = 255 - a;

	switch(hi)
	{
		case 0: 
			*r = 255;
			*g = Vinc;
			*b = 0;
			break;
		case 1: 
			*r = Vdec;
			*g = 255;
			*b = 0;
			break;
		case 2: 
			*r = 0;
			*g = 255;
			*b = Vinc;
			break;
		case 3: 
			*r = 0;
			*g = Vdec;
			*b = 255;
			break;
		case 4: 
			*r = Vinc;
			*g = 0;
			*b = 255;
			break;
		case 5: 
			*r = 255;
			*g = 0;
			*b = Vdec;
			break;
		default:
			*r = 255;
			*g = 255;
			*b = 255;
	}
	return;
}

int emu_data = 0;
int emu_cnt = 0;

int push_mcp_data()
{
	if(emg_mode)
	{
		if(mcp_fft_process())
		{
			float *sp = mcp_fft_get_spectr();
			float norm = 0.001;
			float avg_center = 0;
			for(int x = 1; x < 32; x++)
			{
				if(sp[x] > norm) norm = sp[x];
				if(x > 5 && x <= 30) avg_center += sp[x];
			}
			avg_center *= 0.04;
			if(enable_led_indication)
			{
				if(avg_center > 8)
					leds_set(255, 60, 120);
				else
					leds_set(0, 0, 0);
			}
			avg_center *= 2;
			if(avg_center > 255) sp_center_avg = 255;
			else sp_center_avg = avg_center;
			if(norm > 65535) spectr_scale = 65535;
			else spectr_scale = norm;
//			spectr_scale = sqrt_fast(norm);
			norm = 255.0 / norm;
			for(int x = 0; x < 32; x++)
			{
				if(sp[x] * norm > 255) spectr_buf[x] = 255;
				else spectr_buf[x] = sp[x] * norm;
			}
			emg_data_id++;
			return 1;
		}
		return 0;
	}
	int filtered_value = mcp_get_filtered_value();
	mcp_buffer[mcp_buf_pos] = filtered_value;
	
	float gg = sqrt(bmi.aX*bmi.aX + bmi.aY*bmi.aY + bmi.aZ*bmi.aZ);
	g_activity *= 0.95;
	g_activity += 0.05*gg;
	
	val_ble += filtered_value;
	val_ble_z++;
	if(val_ble_z >= 8)
	{
		mcp_buffer_ble[mcp_buf_ble_pos] = val_ble>>3;
/*		if(emu_cnt == 0)
			emu_data += 50;
		if(emu_cnt == 5)
			emu_data -= 50;
		emu_data++;
		emu_cnt++;
		if(emu_cnt > 10) emu_cnt = 0;
		if(emu_data > 3000) emu_data = -3000;*/
		if(++mcp_buf_ble_pos >= mcp_buf_ble_len) mcp_buf_ble_pos = 0;
		val_ble = 0;
		val_ble_z = 0;
		data_id_ble++;
	}
	
	if(++mcp_buf_pos >= mcp_buf_len) mcp_buf_pos = 0;

	int cv = filtered_value;
	if(cv > 32767) cv = -0xFFFF + cv - 1;
	float cfv = cv;

	avg_base *= 0.997; 
	avg_base += 0.003 * cfv;
	avg_local *= 0.98;
	avg_local += 0.02 * cfv;
	float dv = cfv - avg_base;
	float sdv = cfv - avg_local;

	loc_max *= 0.9;
	loc_min *= 0.9;
	avg_max *= 0.9995;
	avg_min *= 0.9995;
	if(sdv > loc_max) loc_max = sdv;
	if(sdv < loc_min) loc_min = sdv;
	if(dv > avg_max) avg_max = dv;
	if(dv < avg_min) avg_min = dv;	
	
	uint32_t ms = millis();
	if((loc_max - loc_min) > 0.4*(avg_max - avg_min))
		peak_in++;
	else
		peak_in = 0;
	if(peak_in > 10)
	{
		if(prev_peak_cnt == 0 && ms - prev_peak_start > 200) //prevent obvious errors
		{
			peak_start_ms = ms;
			uint32_t RR = peak_start_ms - prev_peak_start;
			push_RR(RR);
			prev_peak_start = peak_start_ms;
		}
		prev_peak_cnt = 80;
	}
	if(prev_peak_cnt > 0)
	{
		prev_peak_cnt--;
		int skin = mcp_get_filtered_skin();
		if(skin < -3600) skin = -3600;
		if(skin > 3600) skin = 3600;
		int hh = (skin + 3600) / 20;
		if(hh < 0) hh = 0;
		if(hh > 359) hh = 359;
//		uint8_t r, g, b;
//		h2color(hh, &r, &g, &b);
//		leds_set(r, g, b);
		if(enable_led_indication)
			leds_set(255, 20, 120);
	}
	else
		if(enable_led_indication)
			leds_set(0, 0, 0);
		
	if(led_out_mode == led_out_analog)
	{
		int analog_baselev = 60;
		uint32_t pdt = ms - peak_start_ms;
		int dval = 0;
		if(pdt < 50)
			dval = pdt*3;
		else if(pdt < 100)
			dval = 150 - pdt*3;
		int vv = analog_baselev + dval;
		if(vv < 0) vv = 0;
		if(vv > 255) vv = 255;
		leds_set_driver(vv>100);
	}
	if(led_out_mode == led_out_analog_inv)
	{
		int analog_baselev = 150;
		uint32_t pdt = ms - peak_start_ms;
		int dval = 0;
		if(pdt < 50)
			dval = pdt*3;
		else if(pdt < 100)
			dval = 150 - pdt*3;
		int vv = analog_baselev - dval;
		if(vv < 0) vv = 0;
		if(vv > 255) vv = 255;
		leds_set_driver(vv > 100); 

//		float vv = (cfv - avg_min) / (avg_max - avg_min + 0.001);
//		leds_set_driver((1.0-vv)*255.0);
	}
	if(led_out_mode == led_out_peaks)
	{
		if(prev_peak_cnt > 0)
			leds_set_driver(255);
		else
			leds_set_driver(0);
	}
	if(led_out_mode == led_out_peaks_inv)
	{
		if(prev_peak_cnt > 0)
			leds_set_driver(0);
		else
			leds_set_driver(255);
	}
//	}
	return 1;
}


void fast_clock_start()
{
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {}
}
void slow_clock_start()
{
	NRF_CLOCK->LFCLKSRC = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {}
}
void fast_clock_stop()
{
	slow_clock_start();
//	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
//	NRF_CLOCK->TASKS_LFCLKSTART = 1;
//	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {}
	NRF_CLOCK->TASKS_HFCLKSTOP = 1;
}


void mode_lowbatt()
{
//	set_led(0);
//	nrf_delay_ms(1);
	mcp3911_turnoff();
	stop_time();
	fast_clock_stop();
	NRF_SPI0->ENABLE = 0;
	leds_set(0, 0, 0);
	leds_set_driver(0);
	NRF_GPIO->OUTCLR = 1<<11;
//	nrf_delay_ms(1);
	NRF_POWER->TASKS_LOWPWR = 1;
//	NRF_POWER->DCDCEN = 1;
	NRF_POWER->SYSTEMOFF = 1;
}
void stop_rtc()
{
	NRF_RTC1->TASKS_STOP = 1;
}
void start_rtc()
{
	stop_rtc();
	NRF_RTC1->TASKS_CLEAR = 1;
	NRF_RTC1->CC[0] = 220;
	NRF_RTC1->CC[1] = 0xFFFF;
	NRF_RTC1->CC[2] = 0xFFFF;
	NRF_RTC1->CC[3] = 0xFFFF;
	NRF_RTC1->PRESCALER = 0x3CFFFF;
	NRF_RTC1->INTENSET = (1<<16); //CC0 event
	NVIC_EnableIRQ(RTC1_IRQn);
	NRF_RTC1->TASKS_START = 1;
}
void RTC1_IRQHandler(void)
{
	/* Update compare counter */
	if (NRF_RTC1->EVENTS_COMPARE[0] != 0)
	{
		NRF_RTC1->EVENTS_COMPARE[0] = 0;
		NRF_RTC1->TASKS_CLEAR = 1;  // Clear Counter		    
//		if(U32_delay_ms) U32_delay_ms--; // used in V_hw_delay_ms()
		__SEV(); // to avoid race condition
		stop_rtc();
	}
}

void read_battery()
{
	
	uint32_t result = 0;
	
	// Configure SAADC singled-ended channel, Internal reference (0.6V) and 1/6 gain.
	NRF_SAADC->CH[0].CONFIG = (SAADC_CH_CONFIG_GAIN_Gain1_6    << SAADC_CH_CONFIG_GAIN_Pos) |
							(SAADC_CH_CONFIG_MODE_SE         << SAADC_CH_CONFIG_MODE_Pos) |
							(SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
							(SAADC_CH_CONFIG_RESN_Bypass     << SAADC_CH_CONFIG_RESN_Pos) |
							(SAADC_CH_CONFIG_RESP_Bypass     << SAADC_CH_CONFIG_RESP_Pos) |
							(SAADC_CH_CONFIG_TACQ_3us        << SAADC_CH_CONFIG_TACQ_Pos);

	// Configure the SAADC channel with VDD as positive input, no negative input(single ended).
	NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_AnalogInput0 << SAADC_CH_PSELP_PSELP_Pos;
	NRF_SAADC->CH[0].PSELN = SAADC_CH_PSELN_PSELN_NC << SAADC_CH_PSELN_PSELN_Pos;

	// Configure the SAADC resolution.
	NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_14bit << SAADC_RESOLUTION_VAL_Pos;

	// Configure result to be put in RAM at the location of "result" variable.
	NRF_SAADC->RESULT.MAXCNT = 1;
	NRF_SAADC->RESULT.PTR = (uint32_t)&result;

	// No automatic sampling, will trigger with TASKS_SAMPLE.
	NRF_SAADC->SAMPLERATE = SAADC_SAMPLERATE_MODE_Task << SAADC_SAMPLERATE_MODE_Pos;

	// Enable SAADC (would capture analog pins if they were used in CH[0].PSELP)
	NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos;


	static int cal_cnt = 0;
	cal_cnt++;
	if(0)if(cal_cnt > 1000)
	{
		cal_cnt = 0;
		// Calibrate the SAADC (only needs to be done once in a while)
		NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
		while (NRF_SAADC->EVENTS_CALIBRATEDONE == 0);
		NRF_SAADC->EVENTS_CALIBRATEDONE = 0;
		while (NRF_SAADC->STATUS == (SAADC_STATUS_STATUS_Busy <<SAADC_STATUS_STATUS_Pos));
	}

	// Start the SAADC and wait for the started event.
	NRF_SAADC->TASKS_START = 1;
	while (NRF_SAADC->EVENTS_STARTED == 0);
	NRF_SAADC->EVENTS_STARTED = 0;

	// Do a SAADC sample, will put the result in the configured RAM buffer.
	NRF_SAADC->TASKS_SAMPLE = 1;
	while (NRF_SAADC->EVENTS_END == 0);
	NRF_SAADC->EVENTS_END = 0;

// Convert the result to voltage
// Result = [V(p) - V(n)] * GAIN/REFERENCE * 2^(RESOLUTION)
// Result = (VDD - 0) * ((1/6) / 0.6) * 2^14
// VDD = Result / 4551.1
//  precise_result = (float)result / 4551.1f;
//  precise_result; // to get rid of set but not used warning

	// Stop the SAADC, since it's not used anymore.
	NRF_SAADC->TASKS_STOP = 1;
	while (NRF_SAADC->EVENTS_STOPPED == 0);
	NRF_SAADC->EVENTS_STOPPED = 0;
	
	float res = result;
	//16384 = 3600 mV on A0
	res = res * 3600.0 / 16384.0; //now res is in mv
	res *= 3.0; //1:3 divider on the battery measurement
	battery_mv = res;
	res -= 2000.0; //everything below 2V is too low for a battery
	if(res < 0)
	{
		battery_level = 0;
		return;
	}
	if(res > 2550) //shouldn't ever happen - 4.55v on the battery is way too much
	{
		battery_level = 255;
		return;
	}
	battery_level = res / 10;
	return;
}


void mode_idle()
{
//	leds_pause();
	pause_time();
	NRF_POWER->TASKS_LOWPWR = 1;
}
void mode_resume_idle()
{
	NRF_POWER->TASKS_CONSTLAT = 1;
	resume_time();
//	leds_resume();
}

void low_power_cycle()
{
	NRF_RADIO->POWER = 0;
//	mode_idle();
	__WFI();
//	mode_resume_idle();
	NRF_RADIO->POWER = 1;
}


enum
{
	radio_mode_fast = 0,
	radio_mode_ble,
	radio_mode_fast64
};

uint8_t radio_mode = radio_mode_ble;

void RADIO_IRQHandler()
{
	if(radio_mode == radio_mode_fast || radio_mode == radio_mode_fast64)
		fr_irq_handler();
	if(radio_mode == radio_mode_ble)
		ble_irq_handler();
}

void switch_to_ble()
{
	fr_disable();
//	fr_poweroff();
	config_ble_adv();
}

void switch_to_fr32()
{
	fr_disable();
//	fr_poweroff();
	fr_init(32);
	fr_listen();	
}

void switch_to_fr48()
{
	fr_disable();
//	fr_poweroff();
	fr_init(48);
	fr_listen();	
}
void switch_to_fr64()
{
	fr_disable();
//	fr_poweroff();
	fr_init(64);
	fr_listen();	
}

void process_btn_short()
{
	if(radio_mode == radio_mode_ble)
	{
		radio_mode = radio_mode_fast64;
		switch_to_fr64();
	}
	else if(radio_mode == radio_mode_fast64)
	{
		radio_mode = radio_mode_fast;
		switch_to_fr32();
	}
	else
	{
		radio_mode = radio_mode_ble;
		switch_to_ble();
	}
}

void process_btn_long()
{
	if(emg_mode == 0)
	{
		emg_mode = 1;
		mcp_fft_mode(1);
	}
	else
	{
		emg_mode = 0;
		mcp_fft_mode(0);
	}
}

int main(void)
{
	int init_ok = 1;
	
	NRF_UICR->NFCPINS = 0;

	NRF_GPIO->PIN_CNF[pin_button] = (0b11 << 2); // pull up << 2 | input buffer << 1 | dir << 0
//	NRF_GPIO->DIRCLR = 1<<pin_button;

	fast_clock_start();
	start_time();
	NRF_GPIO->DIRSET = 1<<11;
	NRF_GPIO->OUTSET = 1<<11;
	leds_init();

//	NRF_POWER->DCDCEN = 1;

	for(int x = 0; x < 3; x++)
	{
		leds_set((x==0)*255, (x==1)*255, (x==2)*255);
		delay_ms(300);
	}
//	leds_set_driver(55);
//	leds_set(255, 60, 120);
	leds_set(0, 0, 0);
	delay_ms(300);

	if(radio_mode == radio_mode_ble)
		config_ble_adv();
	if(radio_mode == radio_mode_fast)
	{
		fr_init(32);
		fr_listen();
	}
	if(radio_mode == radio_mode_fast64)
	{
		fr_init(64);
		fr_listen();
	}
	
	init_mcp3911(13, 12, 14, 15, 17);
	if(!mcp3911_is_ok())
	{
		init_ok = 0;
		leds_set(255, 0, 0);
		delay_ms(1000);
		leds_set(0, 0, 0);
		delay_ms(200);
		leds_set(255, 0, 255);
		delay_ms(1000);
		leds_set(0, 0, 0);
		delay_ms(200);
	}
	bmi160_init(13, 12, 14, 18, 20);
	if(!bmi160_is_ok())
	{
		init_ok = 0;
		leds_set(255, 0, 0);
		delay_ms(1000);
		leds_set(0, 0, 0);
		delay_ms(200);
		leds_set(255, 255, 0);
		delay_ms(1000);
		leds_set(0, 0, 0);
		delay_ms(200);
	}
	if(!(NRF_GPIO->IN & 1<<pin_button))
	{
		leds_set(255, 0, 0);
		delay_ms(1000);
		leds_set(0, 0, 0);
		delay_ms(200);
		leds_set(0, 0, 255);
		delay_ms(1000);
		leds_set(0, 0, 0);
		delay_ms(200);
	}
	

//	leds_set(0, 255, 0);

	delay_ms(100);
	
	rr_init();

	uint32_t last_sent_ms = 0;
	uint8_t ble_send_cnt = 0;
	uint8_t btn_pressed = 0;
	uint32_t btn_on = 0;
	uint32_t btn_off = 0;
	
	int measure_skin = 0;
	uint32_t last_skin_time = 0;
	measure_skin = 1;
	
	NRF_RNG->TASKS_START = 1;
	NRF_RNG->SHORTS = 0;
	NRF_RNG->CONFIG = 1;
	
	int ble_dt1 = 5;
	int ble_dt2 = 5;
	int send_iter = 0;
	int low_bat_cnt = 0;
	
	int bmi_skip_cnt = 0;
	
	if(init_ok)
	{
		for(int x = 0; x < 3; x++)
		{
			leds_set(0, 255, 0);
			delay_ms(200);
			leds_set(0, 0, 0);
			delay_ms(200);
		}
	}

	while(1)
	{
//		low_power_cycle();
		uint32_t ms = millis();
		if(0)if(measure_skin == 0 && ms - last_skin_time > 5000)
		{
			measure_skin = 1;
			last_skin_time = ms;
		}
		if(0)if(measure_skin == 1 && ms - last_skin_time > 5)
		{
			measure_skin = 0;
			last_skin_time = ms;
		}
		
		if(!(NRF_GPIO->IN & 1<<pin_button))
		{
			enable_led_indication = 0;
			if(!btn_pressed)
			{
				btn_pressed = 1;
				btn_on = ms;
			}
			if(ms - btn_on > 25 && ms - btn_on < 1000)
				leds_set(0, 0, 255);
			if(ms - btn_on > 1000 && ms - btn_on < 5000)
				leds_set(0, 255, 0);
			if(ms - btn_on > 5000)
				leds_set(255, 255, 255);
		}
		else
		{
			enable_led_indication = 1;
			if(btn_pressed)
			{
				btn_pressed = 0;
				btn_off = ms;
				uint32_t btn_time = btn_off - btn_on;
				if(btn_time > 25) //ignore too short presses - noise
				{
					if(btn_time < 1000)
						process_btn_short();
					else if(btn_time < 5000)
						process_btn_long();
				}
//				mcp_set_filter_mode(radio_mode == radio_mode_ble);
			}
		}
		if(mcp3911_read())
		{
			if(push_mcp_data())
			{
				mcp_cnt++;
				data_id++;
	//			if(mcp_cnt > 128)
	//				leds_set(255, 255, 1);
	//			else
	//				leds_set(255, 255, 0);
				
				if(unsent_cnt < 30) unsent_cnt++;
				read_battery();
				if(battery_mv < battery_low_threshold)
				{
					low_bat_cnt++;
					if(low_bat_cnt > 100)
						mode_lowbatt();
				}
				else
					low_bat_cnt = 0;
			}
			if(!emg_mode)
			{
				bmi160_read();
				bmi_skip_cnt++;
				if(bmi_skip_cnt == 10)
				{
					bmi160_read_temp();
				}
				if(bmi_skip_cnt > 20)
				{
					bmi160_read_steps();
					bmi_skip_cnt = 0;
				}
			}
/*			if(0)if(fft_pending())
			{
				fft_skip_cnt++; 
				if(fft_skip_cnt > 0)
				{
//					sfft_butterfly();
					int cv = 10 + get_avg_sp();
					//if(cv > spectr_val && cv < 400) 
						spectr_val = cv;
					fft_skip_cnt = 0;
				}
			}*/
			
		}		

		if(radio_mode == radio_mode_fast)
		{
			if(unsent_cnt > 5*(!emg_mode) && ms - last_sent_ms > ble_dt1)
			{
				prepare_data_packet();
				fr_send_and_listen(data_packet);
				unsent_cnt = 0;
				last_sent_ms = ms;
				send_iter = 1;
				if(NRF_RNG->EVENTS_VALRDY)
				{
					volatile int rnd = NRF_RNG->VALUE;
					NRF_RNG->EVENTS_VALRDY = 0;
					ble_dt1 = 1 + 2*emg_mode + (rnd >> 6);
					ble_dt2 = 1 + 2*emg_mode + (rnd%(3+3*emg_mode));
					NRF_RNG->TASKS_START;
				}
			}
			
			if(last_sent_ms > 0 && ms - last_sent_ms > ble_dt2 && send_iter == 1)
			{
				fr_send_and_listen(data_packet);
				send_iter = 2;
			}
			if(last_sent_ms > 0 && ms - last_sent_ms > 2*ble_dt2 && send_iter == 2)
			{
				fr_send_and_listen(data_packet);
				send_iter = 3;
			}
			if(last_sent_ms > 0 && ms - last_sent_ms > 3*ble_dt2 && send_iter == 3)
			{
				fr_send_and_listen(data_packet);
				send_iter = 4;
			}
		}

		if(radio_mode == radio_mode_fast64)
		{
			if(unsent_cnt > 15*(!emg_mode) && ms - last_sent_ms > ble_dt1)
			{
				prepare_data_packet();
				fr_send_and_listen(data_packet);
				unsent_cnt = 0;
				last_sent_ms = ms;
				send_iter = 1;
				if(NRF_RNG->EVENTS_VALRDY)
				{
					volatile int rnd = NRF_RNG->VALUE;
					NRF_RNG->EVENTS_VALRDY = 0;
					ble_dt1 = 1 + 2*emg_mode + (rnd >> 6);
					ble_dt2 = 1 + 2*emg_mode + (rnd%(3+3*emg_mode));
					NRF_RNG->TASKS_START;
				}
			}
			
			if(last_sent_ms > 0 && ms - last_sent_ms > ble_dt2 && send_iter < 5)
			{
				fr_send_and_listen(data_packet);
				last_sent_ms = ms;
				send_iter++;
			}
		}
		
		if(radio_mode == radio_mode_ble)
		{
			int dt = ble_dt1, dt2 = ble_dt2;
			if(ms - last_sent_ms > dt)
			{
				prepare_data_packet_ble();
				prepare_adv_packet(data_packet, 40);
				send_adv_packet();
				ble_send_cnt = 1;
				last_sent_ms = ms;

				if(NRF_RNG->EVENTS_VALRDY)
				{
					volatile int rnd = NRF_RNG->VALUE;
					NRF_RNG->EVENTS_VALRDY = 0;
					ble_dt1 = 11 + (rnd%4);
					ble_dt2 = 5 + ((rnd*17)%5);
					dbg_val = ble_dt1;
					NRF_RNG->TASKS_START;
				}
			}
			if(ms - last_sent_ms > dt2 && ble_send_cnt == 1)
			{
				send_adv_packet();
				ble_send_cnt = 2;
				last_sent_ms = ms;
			}
			if(ms - last_sent_ms > dt2 && ble_send_cnt == 2)
			{
				send_adv_packet();
				last_sent_ms = ms;
				ble_send_cnt = 3;
			}
		}
	}

}

