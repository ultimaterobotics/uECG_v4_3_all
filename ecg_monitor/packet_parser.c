#include "packet_parser.h"

//decode 8-bit acceleration value: we need more precision around 0 and less at high g
float decode_acc(float acc)
{
	float vv = acc - 128.0;
	int vm = vv;
	if(vm < 0) vm = -vm;
	float res = 0;
	if(vm > 100) //for values less than -12 and more than 12 m/s^2 precision is 0.5
	{
		res = 12.0 + (vm - 100) / 2.0;
	}
	else if(vm > 50) //for values from -12...-2 and 2...12 m/s^2 precision is 0.2
	{
		res = 2.0 + (vm - 50) / 5.0;
	}
	else
		res = vm / 25.0; //for values from -2 to 2 m/s^2 precision is 0.04
	if(vv < 0) res = -res;
	return res;
}

uint32_t valid_ids[16384]; //list of valid IDs: each device sends out its name once in a while, 
						//if name and first MAC bytes match - we add it into valid list and process
						//most packets don't have name included - so need to keep track of them
int valid_list_pos = 0;
int valid_list_inited = 0;
int protocol_version = 0;

//base unit in BLE mode captures all BLE traffic on given channel, so we need to check if 
//received packet is from uECG device, and only then parse it
//we should be prepared that parse function is called before full packet is in memory,
//also we should be prepared that bytestream is corrupted and misses bytes 
int parse_ble_packet(uint8_t *pack, int maxlen, uECG_data *uecg_res)
{
	if(!valid_list_inited) //init here so caller don't need to init manually - done only once
	{
		valid_list_inited = 1;
		valid_list_pos = 0;
		for(int x = 0; x < 16384; x++) valid_ids[x] = 0;
	}
	if(maxlen < 37) return -1; //valid uECG BLE packet should have 37 bytes, if we have less - wait for more data
	for(int p = 0; p < 6; p++)
		uecg_res->mac[5-p] = pack[3+p];
	if(uecg_res->mac[0] != 0xBA || uecg_res->mac[1] != 0xBE) //uECG MAC starts with BABE
		return -1;
	uint32_t unit_id = 0;
	for(int p = 0; p < 4; p++)
	{
		unit_id <<= 8;
		unit_id += uecg_res->mac[2+p];
	}
	int is_good = 0;
	for(int x = 0; x < 16384; x++) //check if ID is in the list
		if(valid_ids[x] == unit_id)
		{
			is_good = 1;
			break;
		}
	
	uecg_res->ID = unit_id;
	if(!is_good) //let's see if it has name uECG - depending on version, in can be in 2 places
	{
		int ppos = 3+6;
		int has_name = 0;
		if(pack[ppos] == 5 && pack[ppos+1] == 8) //old version sends name first
		{
			if(pack[ppos+2] == 'u' && pack[ppos+3] == 'E' && pack[ppos+4] == 'C' && pack[ppos+5] == 'G')
				has_name = 1; //valid device of the old type
		}
		if(pack[ppos] == 2 && pack[ppos+1] == 1) //new version sends flags first
		{
			ppos += 3;
			if(pack[ppos] == 5 && pack[ppos+1] == 8) //new version has name after flags
			{
				if(pack[ppos+2] == 'u' && pack[ppos+3] == 'E' && pack[ppos+4] == 'C' && pack[ppos+5] == 'G')
					has_name = 2; //valid device of the new type
			}
		}
		if(has_name)
		{//it's an uECG - add to list
			valid_ids[valid_list_pos] = unit_id;
			valid_list_pos++; //it can overflow and then we'll forget possibly valid device
			//but it's ok - if device is active, it will soon send packet with name again
			if(valid_list_pos >= 16384) valid_list_pos = 0;
		}
		
		if(has_name == 2) return 0;
		if(has_name == 1) is_good = 1;
	}
	
	if(is_good)
	{
		int ppos = 3+6; //3 bytes for BLE header (not 2 as in all BLE documents: nRF52 chip adds one poorly documented byte)
		//+6 bytes for sender MAC address, so payload starts at 10th byte of the packet
		int packet_version = 0;
		if(pack[ppos+1] == 8) //old protocol sends device name in every packet
			packet_version = 1;
		if(pack[ppos+1] == 255) //new protocol sends device name only once per 20 packets, others have only manufacturer's data
			packet_version = 2;
		
		if(!packet_version) return 0; //not from known uECG version
		
		uint8_t *data_pack;
		if(packet_version == 1) data_pack = pack + ppos + 7; //both protocol versions have the same header but different 
		if(packet_version == 2) data_pack = pack + ppos + 2; //number of ECG data points, packet data begins at different positions 

		uint8_t packet_id = data_pack[0]; //packed ID - increased by 1 with each new packet, after 127 is set back to 0
		//we use the same header structure to send different parameters that don't require update in each packet
		//like battery level, step counter, bpm etc. data_pack[1] has parameter ID in lower 4 bits and parameter
		//modifier in upper 4 bits
		uint8_t param_id = data_pack[1]&0xF; //parameter ID
		uint8_t param_mod = data_pack[1]>>4; //parameter modifier
		uint8_t param_hb = data_pack[2]; //higher byte (notation from old version where 2 bytes of header payload were used)
		uint8_t param_lb = data_pack[3]; //lower byte
		uint8_t param_tb = data_pack[4]; //third byte - wasn't present in old version
		
//		printf("%d %d %d %d %d\n", data_pack[0], data_pack[1], data_pack[2], data_pack[3], data_pack[4]);
		
		uecg_res->last_pack_id = packet_id;
 
		if(param_id == param_batt_bpm) //battery level and BPM data
		{
			uecg_res->battery_mv = 2000 + param_hb * 10;
			protocol_version = param_lb;
			if(protocol_version == 4)
				uecg_res->bpm = param_tb / 1.275;
			else
				uecg_res->bpm = param_tb;
		}
		if(param_id == param_lastRR) //2 RR intervals: current and previous
		{ //If connections is poor - we might miss packet with RR data, but when 2 RRs are in a single packet
		//we can be sure that they represent 2 consequitive beats even if some data were lost in between
			int v1_h = param_hb>>4; //we use 12 bits for each RR, so max value is 4096 milliseconds. For any normal beats it's enough - but for serious anomalies it's not reliable
			int v1_l = param_lb; //_hb stores 4 higher bits for each value, lb and tb - lower 8 bits for each value
			int v2_h = param_hb&0x0F;
			int v2_l = param_tb;
			
			int rr1_val = (v1_h<<8) + v1_l; //current RR interval
			if(rr1_val > 32767) rr1_val = -65536 + rr1_val; //explicitly get rid of the sign:
			// when translating to other languages it's more reliable than relying on type properties
			int rr2_val = (v2_h<<8) + v2_l; //previous RR interval
			if(rr2_val > 32767) rr2_val = -65536 + rr2_val;

			if(protocol_version == 4)
			{
				rr1_val /= 1.275;
				rr2_val /= 1.275;
			}

			uecg_res->rr_id = param_mod; //param_mod contains RR identifier, from 0 to 15 - increased by 1 with each detected
			uecg_res->rr_current = rr1_val; //beat on the device, required to identify duplicates: protocol sends the same data many
			uecg_res->rr_prev = rr2_val; //times, so we need to keep track of them
		}
		if(param_id == param_skin_res) //skin resistance, translation into kOhms will be provided later - not obvious even for us :)
		{
			uecg_res->skin_res = (param_hb<<8) | param_lb;
		}
		if(param_id == param_imu_steps) //step counter, overflows at 65536
		{
			uecg_res->steps = (param_hb<<8) | param_lb;
		}
		if(param_id == param_imu_acc) //current acceleration by x,y,z packed in a way to increase precision around 0, so angle can be extracted more or less reliably
		{
			uecg_res->acc_x = decode_acc(param_hb);
			uecg_res->acc_y = decode_acc(param_lb);
			uecg_res->acc_z = decode_acc(param_tb);
		}
		int pack_data_count = 17; //old version has 17 ECG data bytes
		if(packet_version == 2)
			pack_data_count = 21; //new version has 21 ECG data bytes

		//ECG data are stored as first absolute value, and a series of changes relative to the 1st
		//changes are scaled, so if given data region has large differences - they still will fit 8 bits
		//at the cost of lower precision. Normally it's not the case though.
		int pp = 5;
		int16_t v0 = (data_pack[pp]<<8) | data_pack[pp+1]; //first value
		pp += 2;
		uecg_res->RR_data[0] = v0;
		int scale_code = data_pack[pp++]; //scale (encoded: in case of huge noise we might need more than 255)
		int cur_scale = 1;
		if(scale_code < 100) cur_scale = scale_code;
		else cur_scale = 100 + (scale_code-100)*4;
		int prev_v = v0;
		for(int p = 1; p < pack_data_count; p++)
		{
			int dv = data_pack[pp++];
			if(dv > 127) dv = -256+dv; //need to convert into signed. This way is more reliable for translation to other languages
			dv *= cur_scale;
			uecg_res->RR_data[p] = prev_v + dv;
			prev_v += dv;
		}

		return pack_data_count; //return number of ECG data points parsed
	}
	return -1; //something went wrong
}