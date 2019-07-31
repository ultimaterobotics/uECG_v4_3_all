#include "device_functions.h"
#include "serial_functions.h"
#include "drawing.h"

#include <sys/time.h>
#include <fcntl.h>

#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */

#include "fft.h"

SFFTools ft;

struct timeval bpm_time;

uint8_t *response_buf;
int response_pos = 0;
int response_buf_len = 4096;
int response_inited = 0;

int device_count = 0;

#define IMU_CHANNELS 9

uint8_t uart_prefix[2] = {79, 213};
uint8_t uart_suffix[2] = {76, 203};

float batt_voltage = 0;

float avg_rssi = 0;

float device_get_rssi()
{
	return avg_rssi;
}
float device_get_battery()
{
	return batt_voltage;
}

int records[300];
int record_ids[300];
int max_records = 25;
int cur_record = 0;
int record_size = 9;
int last_read_id = 0;
int last_read_record = 0;
int show_integrated = 0;

#define BPM_TIME_HIST 30
int bpm_times_s[BPM_TIME_HIST];
int bpm_times_us[BPM_TIME_HIST];

int bpm_chart_length = 300;
int bpm_data[300];

uint8_t last_pack_id = 0;
int err_conseq = 0;

int save_file = -1;
int save_file_skin = -1;
int save_turned_on = 0;

int ble_mode = 0;

int sockfd = -1;

void connection_init()
{
	sockfd = 1;
	return;
	/* first what are we going to send and where are we going to send it? */
//	int portno =        1366;
//	char *host =        "localhost";
	int portno =        443;
	char *host =        "ultimaterobotics.com.ua";
	//    sprintf(buff,"POST %s HTTP1.1\r\nAccept: */*\r\nReferer: <REFERER HERE>\r\nAccept-Language: en-us\r\nContent-Type: application/x-www-form-urlencoded\r\nAccept-Encoding: gzip,deflate\r\nUser-Agent: Mozilla/4.0\r\nContent-Length: %d\r\nPragma: no-cache\r\nConnection: keep-alive\r\n\r\n%s",argv[2],strlen(postdata),postdata);

	struct hostent *server;
	struct sockaddr_in serv_addr;
	//    printf("Request:\n%s\n",message);

	/* create the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		printf("ERROR opening socket\n");
		return;
	}

	/* lookup the ip address */
	server = gethostbyname(host);
	if (server == NULL)
	{
		printf("ERROR, no such host\n");
		return;
	}

	/* fill in the structure */
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

	/* connect the socket */
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
		printf("ERROR connecting\n");


//	close(sockfd);
}

uint32_t segment_id = 1;
int unsent_data_cnt = 0;
int send_queue[3000];
int need_send_bpm = 0;

void post_data()
{
	if(sockfd < 0) return;
    char message[16384],response[4096];
	char out_json[16384];
    /* fill in the parameters */
	char *message_head = "POST /ecg_data_send HTTP/1.1\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s";
	int jpos = 0;

/*
{
"segment_id": 1245,
"battery_level": 3640,
"data_length": 50,
"data": [
1023,
888,
929,
432,
-121
]
}*/	
	jpos += sprintf(out_json+jpos, "{");
	jpos += sprintf(out_json+jpos, "\"segment_id\":%d,", segment_id++);
	jpos += sprintf(out_json+jpos, "\"battery_level\":%d,", (int)(batt_voltage*1000.0));
	jpos += sprintf(out_json+jpos, "\"data_length\":%d,", unsent_data_cnt);
	jpos += sprintf(out_json+jpos, "\"data\": [");
	for(int x = 0; x < unsent_data_cnt-1; x++)
		jpos += sprintf(out_json+jpos, "%d,", send_queue[x]);
	jpos += sprintf(out_json+jpos, "%d]", send_queue[unsent_data_cnt-1]);
	if(need_send_bpm)
	{
		jpos += sprintf(out_json+jpos, ",\"bpm_data\": [");
		for(int x = 0; x < bpm_chart_length-1; x++)
			jpos += sprintf(out_json+jpos, "%d,", bpm_data[x]);
		jpos += sprintf(out_json+jpos, "%d]", bpm_data[bpm_chart_length-1]);
		need_send_bpm = 0;
	}
	jpos += sprintf(out_json+jpos, "}\r\n");
	
//	sprintf(message, "curl -d '%s' -H \"Content-Type: application/json\" -X POST https://ultimaterobotics.com.ua/ecg_data_send &", out_json);
//	system(message);
	unsent_data_cnt = 0;
	return;
//    sprintf(message, message_head, jpos, out_json);

	printf("sending %s\n\n", message);

    /* send the request */
    int total = strlen(message);
    int sent = 0;
    do {
        int bytes = write(sockfd, message+sent, total-sent);
        if (bytes < 0)
		{
            printf("ERROR writing message to socket");
			close(sockfd);
			sockfd = -1;
			break;
		}
        if (bytes == 0)
            break;
        sent += bytes;
    } while (sent < total);

	unsent_data_cnt = 0;
}

void device_toggle_file_save()
{
	save_turned_on = !save_turned_on;
}

int device_get_mode()
{
	return ble_mode;
}
int stress_mode = 0;
int stress_hist_mode = 0;
void device_toggle_stress_mode()
{
	stress_mode = !stress_mode;
}
void device_toggle_stress_hist()
{
	stress_hist_mode = !stress_hist_mode;
}
int get_ble_draw_mode()
{
	return stress_mode;
}
int get_ble_draw_hist()
{
	return stress_hist_mode;
} 
void device_toggle_ble_mode()
{
	if(ble_mode == 0)
	{
		uint8_t cmd[8];
		cmd[0] = 177;
		cmd[1] = 103;
		cmd[2] = 39;		
		send_data(cmd, 3);
		ble_mode = 1;
	}
	else
	{
		uint8_t cmd[64];
		cmd[0] = 103;
		cmd[1] = 177;
		cmd[2] = 39;
		send_data(cmd, 3);
		ble_mode = 0;
	}
}

void device_close_log_file()
{
	if(save_file > 0)
	{
		close(save_file);
		close(save_file_skin);
		save_file_skin = -1;
		save_file = -1;
	}
}

long last_bpm_out_sec = 0;
int last_skin_v = 0;


typedef struct sdevice_data
{
	uint8_t mac[6];
	uint32_t id;
	uint8_t ver;
	uint8_t battery_level;
	float battery_voltage;
	int *RR_hist;
	int rr_hist_len;
	int rr_hist_pos;
	int last_pack_id;
	int last_RR;
	int last_RR_id;
	float stress;
	float score;
	int on_screen_id;
}sdevice_data;

void push_device_RR(sdevice_data *dev, int RR)
{
	printf("push rr %d\n", RR);
	int time_limit = 120000;
	int time_acc = 0;
	dev->RR_hist[dev->rr_hist_pos] = RR;
	int hp = dev->rr_hist_pos;
	dev->rr_hist_pos++;
	if(dev->rr_hist_pos >= dev->rr_hist_len) dev->rr_hist_pos = 0;
	
	int is_valid = 0;
	
	int rr_stack1[3000];
	int rr_stack2[3000];
	int rr_scnt = 0;
	
	int prev_rr = -1;
	while(time_acc < time_limit && hp != dev->rr_hist_pos && rr_scnt < 1000)
	{
		int rr = dev->RR_hist[hp];
		hp--;
		if(hp < 0) hp += dev->rr_hist_len;
		int prr = prev_rr;
		if(rr < 200 && rr > 2000) is_valid = 0;
		else
		{
			is_valid++;
			prev_rr = rr;
		}
		if(is_valid >= 2)
		{
			time_acc += rr;
			rr_stack1[rr_scnt] = rr;
			rr_stack2[rr_scnt] = prr;
			rr_scnt++;
		}
	}
	
	if(time_acc < time_limit)
	{
		dev->stress = 0.5;
		return;
	}
	
	int fbins[200];
	int fsbins[20];
	for(int x = 0; x < 200; x++)
	{
		fbins[x] = 0;
		fsbins[x/10] = 0;
	}
	
	int total_v = 10;
	for(int x = 0; x < rr_scnt; x++)
	{
		float dr = rr_stack1[x] - rr_stack2[x];
		float rel_dr = 2.0*dr / (float)(rr_stack1[x] + rr_stack2[x]);
		int bin_id = rel_dr * 1000;
		if(bin_id > 199) bin_id = 199;
		fbins[bin_id]++;
		total_v++;
	}
	float s_b15 = 0.001, s_b2050 = 0;
	for(int x = 0; x < 15; x++)
		s_b15 += fbins[x];
	for(int x = 20; x < 50; x++)
		s_b2050  += fbins[x];
	
	for(int x = 0; x < 200; x++)
	{
		fsbins[x/10] += fbins[x];
	}
	for(int x = 0; x < 20; x++)
		sc_addV(stress_hist+dev->on_screen_id, (float)fsbins[x] / (float)total_v);
	
	dev->stress = s_b15 / (s_b15 + s_b2050);
	
	printf("%d\n", rr_scnt);
	for(int x = 0; x < 10; x++)
		printf("%d %d\n", rr_stack1[x], rr_stack2[x]);
	printf("\n\n");
	
	sc_addV(stress_charts + dev->on_screen_id, dev->stress);
	
	char stxt[128];
	sprintf(stxt, "stress level: %d", (int)(dev->stress*100.0));
	if(dev->on_screen_id == 0) gtk_label_set_text(lb_stress_1, stxt);	
	if(dev->on_screen_id == 1) gtk_label_set_text(lb_stress_2, stxt);	
	if(dev->on_screen_id == 2) gtk_label_set_text(lb_stress_3, stxt);	
	if(dev->on_screen_id == 3) gtk_label_set_text(lb_stress_4, stxt);	
	if(dev->on_screen_id == 4) gtk_label_set_text(lb_stress_5, stxt);	
}

sdevice_data ble_devices[100];
int known_devices = 0;

enum param_sends
{
	param_batt = 0,
	param_bpm,
	param_sdnn,
	param_rmssd,
	param_lastRR,
	param_pnn_bins,
	param_max_wo_bins
};

int known_dev_inited = 0;

void device_parse_response(uint8_t *buf, int len)
{
	if(!known_dev_inited)
	{
		for(int x = 0; x < 100; x++)
		{
			ble_devices[x].RR_hist = (int*)malloc(1000*sizeof(int));
			ble_devices[x].rr_hist_len = 1000;
			ble_devices[x].rr_hist_pos = 0;
		}
		known_dev_inited = 1;
		
		draw_spectr = 0;
		draw_emg = 0;
		sfft_1D_init(&ft, 512);
	}
//	if(sockfd < 0) connection_init();
	if(save_turned_on && save_file < 0)
	{
		time_t rawtime;
		time (&rawtime);
		struct tm * curTm = localtime(&rawtime);
		char repfname[256];
		sprintf(repfname, "ecg_log_y%d_m%d_d%d_h%d_m%d_s%d.txt", (2000+curTm->tm_year-100), curTm->tm_mon, curTm->tm_mday, curTm->tm_hour, curTm->tm_min, curTm->tm_sec);
		save_file = open(repfname, O_WRONLY | O_CREAT, 0b110110110);
		sprintf(repfname, "ecg_skin_y%d_m%d_d%d_h%d_m%d_s%d.txt", (2000+curTm->tm_year-100), curTm->tm_mon, curTm->tm_mday, curTm->tm_hour, curTm->tm_min, curTm->tm_sec);
		save_file_skin = open(repfname, O_WRONLY | O_CREAT, 0b110110110);
	}
	if(!save_turned_on && save_file > 0)
	{
		close(save_file);
		close(save_file_skin);
		save_file = -1;
		save_file_skin = -1;
	}
	if(!response_inited)
	{
		response_inited = 1;
		response_buf = (uint8_t*)malloc(response_buf_len);
		response_pos = 0;
	}
	if(len > response_buf_len/2) len = response_buf_len/2;
	if(response_pos > response_buf_len/2)
	{
		int dl = response_buf_len/4;
		memcpy(response_buf, response_buf+response_pos-dl, dl);
		response_pos = dl;
	}
	memcpy(response_buf+response_pos, buf, len);
	response_pos += len;
	int processed_pos = 0;
	for(int x = 0; x < response_pos-25; x++)
	{
		if(response_buf[x] == uart_prefix[0] && response_buf[x+1] == uart_prefix[1])
		{
			uint8_t rssi_level = response_buf[x+2];
			uint8_t *pack = response_buf + x + 3;
			if(ble_mode)
			{
				if(x + 37 >= response_pos) continue;
				uint8_t mac[6];
				for(int p = 0; p < 6; p++)
				{
					mac[5-p] = pack[3+p];
				}
				if(mac[0] != 0xBA || mac[1] != 0xBE)
				{
					processed_pos = x + 37;
					continue;					
				}
				uint8_t name[16];
				for(int p = 0; p < 16; p++)
					name[p] = pack[3+6+2+p];
				if(name[0] == 'C' && name[1] == 'O' && name[2] == '2')
				{
					name[8] = 0;
					for(int p = 0; p < 6; p++)
						printf("%02X ", mac[p]);
					printf(": %s\n", name);
					processed_pos = x + 37;
					continue;
				}
				int dev_by_mac = -1;
				if(name[0] == 'u' && name[1] == 'E' && name[2] == 'C' && name[3] == 'G')
				{
					for(int d = 0; d < known_devices; d++)
					{
						int match = 1;
						for(int m = 0; m < 6; m++)
							if(ble_devices[d].mac[m] != mac[m])
								match = 0;
						if(match)
						{
							dev_by_mac = d;
							break;
						}
					}
					if(dev_by_mac < 0)
					{
						if(known_devices < 50)
						{
							for(int m = 0; m < 6; m++)
								ble_devices[known_devices].mac[m] = mac[m];
							ble_devices[known_devices].score = 1;
							ble_devices[known_devices].ver = 2;
							ble_devices[known_devices].last_pack_id = 0;
							ble_devices[known_devices].last_RR = 0;
							ble_devices[known_devices].last_RR_id = -1;
							dev_by_mac = known_devices;
							known_devices++;
						}
					}
				}
				if(dev_by_mac < 0) //overflow
				{
					processed_pos = x + 37;
					continue;						
				}
				
				for(int d = 0; d < known_devices; d++)
					ble_devices[d].score *= 0.999;
				
				ble_devices[dev_by_mac].score += 1.0;
				
				int active_dev_id = -1;
				for(int d = 0; d <= dev_by_mac; d++)
					if(ble_devices[d].score > 20) active_dev_id++;
					
				if(active_dev_id < 0 || active_dev_id > 7)
				{
					processed_pos = x + 37;
					continue;
				}
				ble_devices[dev_by_mac].on_screen_id = active_dev_id;
								
				uint8_t *data_pack = pack + 3 + 6 + 2 + 4 + 1;
				uint8_t packet_id = data_pack[0];
				uint8_t param_id = data_pack[1];
				uint8_t param_hb = data_pack[2];
				uint8_t param_lb = data_pack[3];
				
				if(param_id == param_batt)
				{
					ble_devices[dev_by_mac].battery_level = param_hb;
					if(param_lb == 3)
						ble_devices[dev_by_mac].ver = 3;
					batt_voltage = ble_devices[dev_by_mac].battery_level;
					int bat_v = 0;
					if(ble_devices[dev_by_mac].battery_level > 147)
						bat_v = 4200;
					else bat_v = 4200 - (147 - ble_devices[dev_by_mac].battery_level)*35;
					batt_voltage = bat_v;
					batt_voltage /= 1000.0;
					ble_devices[dev_by_mac].battery_voltage = batt_voltage;					
				}
				if(param_id == param_lastRR)
				{
					int rr_val = ((param_hb&0b1111)<<8) + param_lb;
					if(rr_val > 32767) rr_val = -65536 + rr_val;
					int rr_id = param_hb>>4;
					
					if((ble_devices[dev_by_mac].last_RR_id + 1)%16 == rr_id)
					{
						push_device_RR(ble_devices+dev_by_mac, rr_val);
					}
					ble_devices[dev_by_mac].last_RR_id = rr_id;
					ble_devices[dev_by_mac].last_RR = rr_val;
				}
				
				int sp = 4;
				int vv = (data_pack[sp]<<8) | data_pack[sp+1];
				if(vv > 32768) vv = -65536 + vv;
				int start_value = vv;
				sp += 2;
				int scale = data_pack[sp];
				sp++;
//				printf("stv %d scale %d \n", start_value, scale);
				int pack_values[20];
				int pack_data_count = 15;
				if(ble_devices[dev_by_mac].ver == 2)
				{
					pack_data_count = 9;
					sp = 4;
					for(int p = 0; p < pack_data_count; p++)
					{
						pack_values[p] = (data_pack[sp]<<8) | data_pack[sp+1];
						if(pack_values[p] > 32767)
							pack_values[p] = -65536 + pack_values[p];
						sp += 2;
					}
				}
				else
				{
					pack_values[0] = start_value;
					sp = 7;
					for(int p = 1; p < pack_data_count; p++)
					{
						int dv = data_pack[sp];
						sp++;
						if(dv > 127) dv = -256+dv;
						dv *= scale;
						pack_values[p] = pack_values[p-1] + dv;
					}
				}
//				for(int p = 0; p < pack_data_count; p++)
//					printf("%d ", pack_values[p]);
//				printf("\n");
//				for(int p = 0; p < pack_data_count; p++)
//					printf("%g ", sc_getV(ecg_charts + active_dev_id, pack_data_count-p));
//				printf("\n");
//				printf("====================\n");

				int d_id = packet_id - ble_devices[dev_by_mac].last_pack_id;
				ble_devices[dev_by_mac].last_pack_id = packet_id;
				if(d_id < 0) d_id += 256;
				
//				printf("pack_id %d d_id %d, dev_score %g\n", packet_id, d_id, known_scores[dev_by_mac]);
				 
				if(d_id > pack_data_count)
				{
					printf("pack_id %d d_id %d, dev_score %g\n", packet_id, d_id, ble_devices[dev_by_mac].score);
					float vv = sc_getV(ecg_charts + active_dev_id, 1);
					for(int p = 0; p < d_id - pack_data_count; p++)
						sc_addV(ecg_charts + active_dev_id, vv);
					d_id = pack_data_count;
				}
				for(int p = 0; p < d_id; p++)
				{
					float vv = pack_values[pack_data_count - d_id + p];
					sc_addV(ecg_charts + active_dev_id, vv);
				}
//				for(int p = 0; p < pack_data_count; p++)
//				{
//					float vv = pack_values[p];
//					sc_addV(ecg_charts + active_dev_id, vv);
//				}
				
				
				processed_pos = x + 37;
				continue;
			}
			uint8_t message_length = pack[0];
			uint8_t packet_id = pack[1];
			uint8_t ppos = 2;
			uint32_t unit_id = (pack[ppos++]<<24);
			unit_id |= (pack[ppos++]<<16);
			unit_id |= (pack[ppos++]<<8);
			unit_id |= pack[ppos++];
			printf("unit id %lu\n", unit_id);
//			if(last_pack_id + 1 != packet_id && last_pack_id != packet_id)
//				printf("pack err %d %d\n", last_pack_id, packet_id);
			uint8_t data_points = pack[ppos++];
			uint8_t battery_level = pack[ppos++];

//			printf("pack %d %d %d %d %d\n", pack[0], pack[1], pack[2], pack[3], pack[4]);
						
			avg_rssi *= 0.9;
			avg_rssi += 0.1 * (float)rssi_level;
			
			if(data_points == 32) //EMG mode
			{
				if(x + message_length >= response_pos) continue;
				if(x + 40 >= response_pos) continue;
				uint8_t check = 0;
				for(int x = 0; x < message_length-1; x++)
				{
					check += pack[x];
				}
	//			check--;
				if(check != pack[message_length-1])
				{
					printf("check %d pack check %d\n", check, pack[message_length-1]);
					processed_pos = x + 40;
					continue;					
				}
				
				int dev_id = -1;
				for(int d = 0; d < known_devices; d++)
				{
					if(ble_devices[d].id == unit_id)
						dev_id = d;
				}
				for(int d = 0; d < known_devices; d++)
					ble_devices[d].score *= 0.99;
				if(dev_id < 0)
				{
					int ins_id = -1;
					for(int d = 0; d < known_devices; d++)
						if(ble_devices[d].score < 1)
						{
							ins_id = d;
							break;
						}
					if(ins_id < 0) ins_id = known_devices;
					ble_devices[ins_id].id = unit_id;
					ble_devices[ins_id].score = 30;
					if(ins_id == known_devices && known_devices < 100) known_devices++;
				}
				else
				{
					ble_devices[dev_id].score += 1.0;
				}
				

				int data_id = pack[ppos++];
				static int prev_emg_data_id = 0;
				if(data_id == prev_emg_data_id)
				{
					processed_pos = x + 40;
					continue;
				}
				prev_emg_data_id = data_id;
				ppos++;
				ppos++;
				int spectr_scale = (pack[ppos]<<8) | pack[ppos+1]; ppos += 2;
				float sp_sc = 1.0+spectr_scale;
				float spvals[32];
				printf("%g ", sp_sc);
				for(int n = 0; n < 32; n++)
				{
					spvals[n] = pack[ppos+n];
					spvals[n] *= sp_sc / 255.0;
					printf("%g ", spvals[n]);
				}
				printf("\n");
				if(dev_id >= 0 && dev_id < 3)
				spg_add_spectr(emg_charts+dev_id, spvals);
				processed_pos = x + 40;
				draw_emg = 1;
				continue;
			}
//			printf("got %d points\n", data_points);
			if(0)if(data_points != 4)
			{
				printf("got %d points, corrected to 4\n", data_points);
				data_points = 4;
			}
			if(data_points > 16)
			{
				printf("got %d points, too many\n", data_points);
				continue;
			}

			if(x + message_length > response_pos-1) continue;			
			uint8_t check = 0;
			for(int x = 0; x < message_length-1; x++)
			{
				check += pack[x];
			}
//			check--;
			if(check != pack[message_length-1])
				printf("check %d pack check %d\n", check, pack[message_length-1]);
//			check += 1;
//			check = pack[5 + data_points*2];
			if(check == pack[message_length-1])
			{
				uint8_t exp_pack_id = last_pack_id + 1;
				if(exp_pack_id > 128) exp_pack_id = 0;
					err_conseq++;
					
				if(packet_id != exp_pack_id && err_conseq < 2)
				{
					processed_pos = x + data_points*2;
					continue;
				}
				else err_conseq = 0;
			}
//			printf("check %d pcheck %d\n", check, pack[5 + data_points*2]);
			if(check == pack[message_length-1])
			{
				batt_voltage = battery_level;
				int bat_v = 0;
				if(battery_level > 147)
					bat_v = 4200;
				else bat_v = 4200 - (147 - battery_level)*35;
				batt_voltage = bat_v;
				batt_voltage /= 1000.0;
//				batt_voltage = batt_voltage * 3.0 /255.0 * 1.2 / 0.45 * 1.35;
//				printf("batv %g\n", batt_voltage);
				int16_t ch0[64];
				int16_t ch1[64];
				int pos = ppos+4;
				if(last_pack_id != packet_id)
					for(int x = 0; x < data_points; x++)
				{
					float pv = sc_getV(acc_charts, 1);
					ch0[x] = (pack[pos]<<8) | (pack[pos+1]); pos += 2;
					ch1[x] = ch0[x];//(pack[pos]<<8) | (pack[pos+1]); pos += 2;
//					printf("%d %g\n", x, ch0[x]);
					static float c0 = 0;
					static float c1 = 0;
					c0 *= 0.9;
					c0 += 0.1*ch0[x];
					c1 *= 0.9;
					c1 += 0.1*ch1[x];
					sc_addV(acc_charts, ch0[x]);

					static int fft_dv = 0;
					fft_dv++;
					if(fft_dv > 10)
					{
						fft_dv = 0;
						float dat[512];
						float sp[256];
						for(int h = 0; h < 512; h++)
						{
							dat[h] = sc_getV(acc_charts, 512-h);
							float wfx = 256 - h;
							wfx /= 256;
							wfx *= 3.1415/2;
							dat[h] *= cos(wfx);
						}
						sfft_1D(&ft, dat, NULL, 0);
						float *sr = sfft_get_spect1D_r(&ft);
						float *si = sfft_get_spect1D_i(&ft);
						for(int h = 0; h < 256; h++)
							sp[h] = sqrt(sr[h]*sr[h] + si[h]*si[h]) * (float)(h) / 256.0;
						spg_add_spectr(emg_chart, sp);
					}
					int has_skin_data = 0;
					if(x == data_points-1 && pos < message_length-2)
					{
						has_skin_data = 1;
						int16_t skin = (pack[pos]<<8) | (pack[pos+1]); pos += 2;
						last_skin_v = skin;
						sc_addV(acc_charts+1, skin);
					}

					send_queue[unsent_data_cnt++] = ch0[x];
					if(unsent_data_cnt > 1000) unsent_data_cnt = 1000;
					
					if(save_file > 0 && save_turned_on)
					{
						char out_line[256];
						int len = 0;
						if(has_skin_data)
						{
							static int skin_dec_cnt = 0;
							skin_dec_cnt++;
							if(skin_dec_cnt >= 10)
							{
								len = sprintf(out_line, "%d\n", last_skin_v);
								write(save_file_skin, out_line, len);
								skin_dec_cnt = 0;
							}
						}
						struct timespec spec;
						clock_gettime(CLOCK_REALTIME, &spec);
						len = sprintf(out_line, "%d.%d %d\n", spec.tv_sec, (int)(spec.tv_nsec / 1.0e6), (int)ch0[x]);
						write(save_file, out_line, len);
					}
//					sc_addV(acc_charts+1, ch1[x]);
//					sc_addV(acc_charts, c0);
//					sc_addV(acc_charts+1, c1);

					static float avg_base = 0;
					static float avg_local = 0;
					static float avg_max = 0;
					static float avg_min = 0;
					static float loc_max = 0;
					static float loc_min = 0;

					static int peak_in = 0;

					int cv = ch0[x];
					float cfv = ch0[x];

					avg_base *= 0.998; 
					avg_base += 0.002 * cfv;
					avg_local *= 0.98;
					avg_local += 0.02 * cfv;
					float dv = cfv - avg_base;
					float sdv = cfv - avg_local;

					loc_max *= 0.98;
					loc_min *= 0.98;
					avg_max *= 0.9995;
					avg_min *= 0.9995;
					if(dv > loc_max) loc_max = dv;
					if(dv < loc_min) loc_min = dv;
					if(dv > avg_max) avg_max = avg_max*0.99 + 0.01*dv;
					if(dv < avg_min) avg_min = avg_min*0.99 + 0.01*dv;	
	
					if((loc_max - loc_min) > 1.5*(avg_max - avg_min))
						peak_in++;
					else
						peak_in = 0;
					static int peak_detected = 0;
					if(peak_in > 5)
						peak_detected = 100;
						
					if(peak_detected > 0) peak_detected--;

					
					static float avgg = 0;
					static float avrr = 0;
					static float avg_l = 0;
					static float avg_s = 0;
					static float ls_peak = 0;
					
					dv = ch0[x];
					avgg *= 0.97; 
					avgg += 0.03 * dv;
					avrr *= 0.9; 
					avrr += 0.1 * dv;
//					dv -= pv;
					dv -= avgg;
					if(dv < 0) dv = -dv;
					avg_l *= 0.9995; 
					avg_l += 0.0005 * dv;
					avg_s *= 0.95;
					avg_s += 0.05 * dv;
//					printf("%g  %g  %g\n", pv, avg_s, avg_l);
//					printf("%g\n", pv); 
					float cc = fabs(avrr-avgg);// / (fabs(avgg) + 1.0);
					if(cc - 70.0 > 0)
					{
						peak_detected = 1;
					}
					else peak_detected = 0;
//					sc_addV(acc_charts+1, (peak_detected>0) * 800);// cc*1.0);//(cc - ls_peak) * 100.0);
					static int ls_peak_cnt = 0;
					static int ls_peak_in = 0;
					if(cc > ls_peak)
						ls_peak_in++;
					else 
						ls_peak_in = 0;
					if(ls_peak_in > 15)
					{
						if(ls_peak_cnt == 0)
						{
							for(int n = BPM_TIME_HIST-1; n > 0; n--)
							{
								bpm_times_s[n] = bpm_times_s[n-1];
								bpm_times_us[n] = bpm_times_us[n-1];
							}
							gettimeofday(&bpm_time, NULL);
							bpm_times_s[0] = bpm_time.tv_sec;
							bpm_times_us[0] = bpm_time.tv_usec;
							
							if(bpm_time.tv_sec > last_bpm_out_sec + 10)
							{
								need_send_bpm = 1;
								for(int x = 0; x < bpm_chart_length; x++)
									bpm_data[x] = bpm_data[x+1];
								int bb = device_get_bpm();
								if(bb < 30 || bb > 300) bb = 0;
								bpm_data[bpm_chart_length-1] = bb;							
								if(last_bpm_out_sec + 15 < bpm_time.tv_sec)
									last_bpm_out_sec += 10;
								else
									last_bpm_out_sec = bpm_time.tv_sec;
							}
						}

						ls_peak = cc;
						ls_peak_cnt = 10;
						
					}
					ls_peak *= 0.9995;
					if(ls_peak_cnt > 0)
					{
						ls_peak_cnt--;
//						sc_addV(acc_charts+1, 800);
					}
					else
						;
//						sc_addV(acc_charts+1, 0);
	//				sc_addV(acc_charts, pv1);
	//				sc_addV(acc_charts+1, pv2);
				}
				last_pack_id = packet_id;
			}
/*			else
			{
				for(int x = 0; x < 4; x++)
				{
					int ch0 = sc_getV(acc_charts, 1);
					int ch1 = sc_getV(acc_charts+1, 1);
					sc_addV(acc_charts, ch0);
					sc_addV(acc_charts+1, ch1);
					sc_addV(acc_charts+2, sc_getV(acc_charts+2, 1));
				}
			}*/
			processed_pos = x + data_points*2;
		}
	}
	if(unsent_data_cnt > 20) post_data();
	memcpy(response_buf, response_buf+processed_pos, response_pos-processed_pos);
	response_pos -= processed_pos;
//	printf("processed to %d, new buf end %d\n", processed_pos, response_pos);
}

int device_get_bpm()
{
//	return 0;
	float tot_dt = 0;
//	for(int n = 0; n < BPM_TIME_HIST-1; n++)
//	{ 
//		tot_dt += (bpm_times_s[n+1] - bpm_times_s[n]) * 1000 + (bpm_times_us[n+1] - bpm_times_us[n])/1000;
//	}

	tot_dt = (bpm_times_s[0] - bpm_times_s[BPM_TIME_HIST-1]) + (bpm_times_us[0] - bpm_times_us[BPM_TIME_HIST-1]) / 1000000.0;
//	printf("%d %d - %d %d = %g\n", bpm_times_s[0], bpm_times_us[0], bpm_times_s[1], bpm_times_us[1], tot_dt);
//	tot_dt *= 0.001;
	if(tot_dt < 0.2) tot_dt = 0.2;
	int bpm = (float)(BPM_TIME_HIST-1) * 60.0 / tot_dt;
	return bpm;
}

void device_draw_time()
{
	draw_spectr = 0;
}
void device_draw_phase()
{
	draw_spectr = 1;
}
void device_draw_centered()
{
	
}