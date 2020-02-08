#include "device_functions.h"
#include "serial_functions.h"
#include "drawing.h"

#include "packet_parser.h"

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

float bmi_ax;
float bmi_ay;
float bmi_az;
int bmi_steps;

float device_get_ax(){return bmi_ax;}
float device_get_ay(){return bmi_ay;}
float device_get_az(){return bmi_az;}
int device_get_steps(){return bmi_steps;}

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
int save_file_RR = -1;
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
		close(save_file_RR);
		close(save_file_skin);
		save_file_skin = -1;
		save_file = -1;
		save_file_RR = -1;
	}
}

long last_bpm_out_sec = 0;
int last_skin_v = 0;

typedef struct sdevice_data
{
	uECG_data data;
	int *RR_hist;
	int rr_hist_len;
	int rr_hist_pos;
	float stress;
	float score;
	int on_screen_id;
	int last_saved_rr_id;
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
	sprintf(stxt, "BPM %d steps %d bat %.2f skin %d", dev->data.bpm, dev->data.steps, dev->data.battery_mv, dev->data.skin_res);
	if(dev->on_screen_id == 0) gtk_label_set_text(lb_stress_1, stxt);	
	if(dev->on_screen_id == 1) gtk_label_set_text(lb_stress_2, stxt);	
	if(dev->on_screen_id == 2) gtk_label_set_text(lb_stress_3, stxt);	
	if(dev->on_screen_id == 3) gtk_label_set_text(lb_stress_4, stxt);	
	if(dev->on_screen_id == 4) gtk_label_set_text(lb_stress_5, stxt);	
}

sdevice_data ble_devices[100];
int known_devices = 0;

int known_dev_inited = 0;
float skin_d_avg = 0;

void device_parse_response(uint8_t *buf, int len)
//we should be prepared that bytestream has missed bytes - it happens too often to ignore,
//so each data transfer from base starts with 2 fixed prefix bytes. But it could happen that
//actual data contents occasionally matches these prefix bytes, so we can't just treat them
//as guaranteed packet start - so we assume it _could_ be a packet start and try to parse
//the result, and see if it makes sense
{
	if(!known_dev_inited) //we keep track of most active devices to display them - once in a while
 	{ //due to errors the same uECG might appear with different MACs, so we need to forget devices
	//that don't send many packets
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
//	if(sockfd < 0) connection_init(); //network interface init - not operational for some time already, need full rework
	if(save_turned_on && save_file < 0) //if saving into file was just requested - init it
	{
		time_t rawtime;
		time (&rawtime);
		struct tm * curTm = localtime(&rawtime);
		char repfname[256];
		sprintf(repfname, "ecg_log_y%d_m%d_d%d_h%d_m%d_s%d.txt", (2000+curTm->tm_year-100), curTm->tm_mon, curTm->tm_mday, curTm->tm_hour, curTm->tm_min, curTm->tm_sec);
		save_file = open(repfname, O_WRONLY | O_CREAT, 0b110110110);
		sprintf(repfname, "ecg_RR_y%d_m%d_d%d_h%d_m%d_s%d.txt", (2000+curTm->tm_year-100), curTm->tm_mon, curTm->tm_mday, curTm->tm_hour, curTm->tm_min, curTm->tm_sec);
		save_file_RR = open(repfname, O_WRONLY | O_CREAT, 0b110110110);
		sprintf(repfname, "ecg_skin_y%d_m%d_d%d_h%d_m%d_s%d.txt", (2000+curTm->tm_year-100), curTm->tm_mon, curTm->tm_mday, curTm->tm_hour, curTm->tm_min, curTm->tm_sec);
		save_file_skin = open(repfname, O_WRONLY | O_CREAT, 0b110110110);
	}
	if(!save_turned_on && save_file > 0) //if saving was just disabled - close files
	{
		close(save_file);
		close(save_file_skin);
		close(save_file_RR);
		save_file = -1;
		save_file_RR = -1;
		save_file_skin = -1;
	}
	if(!response_inited) //init buffer for storing bytestream data ("response")
	{
		response_inited = 1;
		response_buf = (uint8_t*)malloc(response_buf_len);
		response_pos = 0;
	}
	//===========
	//buffer for storing bytestream, if buffer is overfilled - older half of the buffer is dropped
	if(len > response_buf_len/2) len = response_buf_len/2;
	if(response_pos > response_buf_len/2)
	{
		int dl = response_buf_len/4;
		memcpy(response_buf, response_buf+response_pos-dl, dl);
		response_pos = dl;
	}
	memcpy(response_buf+response_pos, buf, len);
	response_pos += len;
	//======= at this point, response_buf contains most recent unprocessed data, starting from 0
	int processed_pos = 0;
	for(int x = 0; x < response_pos-25; x++) //25 - always less than minimum valid packet length
	{
		if(response_buf[x] == uart_prefix[0] && response_buf[x+1] == uart_prefix[1])
		{//we detected possible start of the packet, trying to make sense of it
			uint8_t rssi_level = response_buf[x+2];
			uint8_t *pack = response_buf + x + 3;
			if(ble_mode) //in each mode packet structure is completely different
			{
				uECG_data res_data;
				int detected_data_count = parse_ble_packet(pack, response_pos - x - 3, &res_data);
				if(detected_data_count < 0) //not from uECG or corrupted/unrecognized packet
					continue;
				if(detected_data_count == 0) //it's from uECG but has no ECG data
				{
					processed_pos = x+5;
					continue;
				}
				
				printf("dev id %08X\n", res_data.ID);
				
				int dev_by_mac = -1;
				for(int d = 0; d < known_devices; d++)
				{
					if(ble_devices[d].data.ID == res_data.ID)
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
							ble_devices[known_devices].data.ID = res_data.ID;
						ble_devices[known_devices].data.last_pack_id = res_data.last_pack_id;
						ble_devices[known_devices].data.rr_id = res_data.rr_id;
						ble_devices[known_devices].data.rr_current = res_data.rr_current;
						ble_devices[known_devices].score = 1;
						dev_by_mac = known_devices;
						known_devices++;
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
				if((ble_devices[dev_by_mac].data.rr_id + 1)%16 == res_data.rr_id)
				{
					push_device_RR(ble_devices+dev_by_mac, res_data.rr_current);
				}
				ble_devices[dev_by_mac].data.rr_id = res_data.rr_id;
				ble_devices[dev_by_mac].data.rr_current = res_data.rr_current;

				int d_id = res_data.last_pack_id - ble_devices[dev_by_mac].data.last_pack_id;
				ble_devices[dev_by_mac].data.last_pack_id = res_data.last_pack_id;
				if(d_id < 0) d_id += 256;
				
				if(d_id > detected_data_count)
				{
					printf("pack_id %d d_id %d, dev_score %g\n", res_data.last_pack_id, d_id, ble_devices[dev_by_mac].score);
					float vv = sc_getV(ecg_charts + active_dev_id, 1);
					for(int p = 0; p < d_id - detected_data_count; p++)
						sc_addV(ecg_charts + active_dev_id, vv);
					d_id = detected_data_count;
				}
				for(int p = 0; p < d_id; p++)
				{
					float vv = res_data.RR_data[detected_data_count - d_id + p];
					sc_addV(ecg_charts + active_dev_id, vv);
				}
				if(save_file > 0 && save_turned_on && active_dev_id == 0)
				{
					char out_line[256];
					int len = 0;
					struct timespec spec;
					clock_gettime(CLOCK_REALTIME, &spec);
					for(int p = 0; p < d_id; p++)
					{
						int val = res_data.RR_data[detected_data_count - d_id + p];
						len = sprintf(out_line, "%d.%d %d\n", spec.tv_sec, (int)(spec.tv_nsec / 1.0e6), (int)val);
						write(save_file, out_line, len);
					}
					if(ble_devices[dev_by_mac].data.rr_id != ble_devices[dev_by_mac].last_saved_rr_id)
					{
						len = sprintf(out_line, "%d.%d %d %d\n", spec.tv_sec, (int)(spec.tv_nsec / 1.0e6), res_data.rr_current, res_data.rr_prev);
						write(save_file_RR, out_line, len);
						ble_devices[dev_by_mac].last_saved_rr_id = ble_devices[dev_by_mac].data.rr_id;
					}
//					ble_devices[dev_by_mac].last_RR = rr_val;
					
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
			int param_id = pack[ppos++];
			
			uint8_t param_hb = pack[ppos++];
			uint8_t param_lb = pack[ppos++];
			uint8_t param_tb = pack[ppos++];
			
			uint8_t battery_level = 0;
			
			int data_pos = ppos;
								
			if(param_id == param_batt_bpm)
			{
				battery_level = param_hb;
				int bat_v = 0;
				bat_v = 2000 + battery_level * 10;
//				if(battery_level > 147)
//					bat_v = 4200;
//				else bat_v = 4200 - (147 - battery_level)*35;
				batt_voltage = bat_v;
				batt_voltage /= 1000.0;
				
//				ble_devices[dev_by_mac].bpm = param_tb;
			}
/*				if(param_id == param_lastRR)
				{
					int rr_val = (param_lb<<8) + param_tb;
					if(rr_val > 32767) rr_val = -65536 + rr_val;
					
					int rr_id = param_hb;

//					if(ble_devices[dev_by_mac].last_RR_id != rr_id)
					if((ble_devices[dev_by_mac].last_RR_id + 1)%16 == rr_id)
					{
						push_device_RR(ble_devices+dev_by_mac, rr1_val);
					}
//					printf("")
					ble_devices[dev_by_mac].last_RR_id = rr_id;
					ble_devices[dev_by_mac].last_RR = rr1_val;
				}
				if(param_id == param_skin_res)
				{
					ble_devices[dev_by_mac].skin_res = (param_hb<<8) | param_lb;
				}*/
			if(param_id == param_imu_steps)
			{
				bmi_steps = (param_hb<<8) | param_lb;
			}
			if(param_id == param_imu_acc)
			{					
				bmi_ax = decode_acc(param_hb);
				bmi_ay = decode_acc(param_lb);
				bmi_az = decode_acc(param_tb);
			}
			
//			uint8_t battery_level = pack[ppos++];

//			printf("pack %d %d %d %d %d\n", pack[0], pack[1], pack[2], pack[3], pack[4]);
						
			avg_rssi *= 0.9;
			avg_rssi += 0.1 * (float)rssi_level;
		
			static int non_emg_cnt = 0;
			
			if(data_points != 32) non_emg_cnt++;
			if(non_emg_cnt > 1000)
				draw_emg = 0;
			if(data_points == 32) //EMG mode
			{
				non_emg_cnt = 0;
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
					if(ble_devices[d].data.ID == unit_id)
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
					ble_devices[ins_id].data.ID = unit_id;
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
			{
				printf("check %d pack check %d length %d\n", check, pack[message_length-1], message_length);
//				check = pack[message_length-1];
//				for(int x = 0; x < message_length; x++)
//				{
//					printf("%02X ", pack[x]);
//				}
//				printf("\n");
			}
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
				
//				printf("A: %d %d %d S %d\n", acc_x, acc_y, acc_z, steps);
//				batt_voltage = batt_voltage * 3.0 /255.0 * 1.2 / 0.45 * 1.35;
//				printf("batv %g\n", batt_voltage);
				int16_t ch0[64];
				int16_t ch1[64];
				
				int pos = data_pos;

				if(last_pack_id != packet_id)
					for(int n = 0; n < data_points; n++)
				{
					float pv = sc_getV(acc_charts, 1);
					int16_t val = (pack[pos]<<8) | (pack[pos+1]);
					printf(" %d", val);
					ch0[n] = val; pos += 2;
					ch1[n] = ch0[x];//(pack[pos]<<8) | (pack[pos+1]); pos += 2;
//					printf("%d %g\n", x, ch0[x]);
					static float c0 = 0;
					static float c1 = 0;
					c0 *= 0.9;
					c0 += 0.1*ch0[n];
					c1 *= 0.9;
					c1 += 0.1*ch1[n];
					sc_addV(acc_charts, ch0[n]);

					static int fft_dv = 0;
					fft_dv++;
					if(0)if(fft_dv > 10)
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
					int has_skin_data = (n == data_points-1);
					if(has_skin_data)
					{
						int16_t skin = (pack[pos]<<8) | (pack[pos+1]); pos += 2;
						last_skin_v = skin;
						static int skin_dec = 0;
						skin_d_avg *= 0.95;
						skin_d_avg += 0.05*(float)skin;
						skin_dec++;
						if(skin_dec > 10)
						{
							sc_addV(acc_charts+1, skin_d_avg);
							skin_dec = 0;
						}
					}

					send_queue[unsent_data_cnt++] = ch0[n];
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
						len = sprintf(out_line, "%d.%d %d\n", spec.tv_sec, (int)(spec.tv_nsec / 1.0e6), (int)ch0[n]);
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

					int cv = ch0[n];
					float cfv = ch0[n];

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

device_get_skin_res()
{
	return (int)skin_d_avg;
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