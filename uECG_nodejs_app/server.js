var express = require('express');
var app = express();
var path = require('path');
//var binary = require('binary');
var bodyParser = require('body-parser');

const SerialPort = require('serialport');
const ByteLength = require('@serialport/parser-byte-length');
var port;
var parser;

var in_fw_upload = 0;
var fw_upload_USB = 0;
var fw_upload_RF = 0;

var update_rate = 4;

var cur_port_path = "";

prepare_parser = function()
{
	parser.on('data', function(data) {
		serial_process(data);
	});

	in_fw_upload = 0;

	setTimeout(update_history_values, 1000 / update_rate);
}

wait_new_port = function(port_name_list, name_part)
{
	SerialPort.list().then( ports =>
	{
		if(fw_upload_USB > 0) return;
		if(fw_upload_RF > 0) return;
		var completed = 0;
		for(var p = 0; p < ports.length; p++)
		{
			if(port_name_list.indexOf(ports[p].path) < 0 && ports[p].path.includes(name_part))
			{
				console.log('device found: ' + ports[p].path);
				console.log(ports[p]);
				cur_port_path = ports[p].path;
				port = new SerialPort(cur_port_path, { baudRate: 921600 });
				parser = port.pipe(new ByteLength({length: 16}));
				prepare_parser();
				completed = 1;
			}
		}
		if(completed == 0) wait_new_port(port_name_list, name_part);
	},
	err => { console.error('Error listing ports', err) } );

}
wait_for_port = function(name_part)
{
	console.log('waiting for new device...');
	SerialPort.list().then( ports =>
	{
		port_name_list = [];
		var need_to_wait = 1;
		for(var p = 0; p < ports.length; p++)
		{
			port_name_list[p] = ports[p].path;
			if(ports[p].path.indexOf(name_part) >= 0)
				if(ports[p].vendorId !== undefined && ports[p].productId !== undefined)
				if(ports[p].vendorId.indexOf('10C4') == 0 || ports[p].vendorId.indexOf('10c4') == 0)
					if(ports[p].productId.indexOf('EA60') == 0 || ports[p].productId.indexOf('ea60') == 0)
					{
						cur_port_path = ports[p].path;
						need_to_wait = 0;
					}

		}
		if(need_to_wait > 0)
			wait_new_port(port_name_list, name_part);
		else
		{
			console.log('device found: ' + cur_port_path);
			port = new SerialPort(cur_port_path, { baudRate: 921600 });
			parser = port.pipe(new ByteLength({length: 16}));
			prepare_parser();
		}
	},
	err => { console.error('Error listing ports', err) } );

}

init_serial = function()
{
	if(process.platform == 'linux')
	{
		wait_for_port('ttyUSB');
	}
	if(process.platform == 'darwin')
	{
		wait_for_port('tty');
	}
	if(process.platform == 'win32' || process.platform == 'win64')
	{
		wait_for_port('COM');
	}
}

init_serial();

//=================================================ECG===============================================

//var fs = require('fs');
//var stream = fs.createWriteStream("ecg_data.csv");

app.use(express.json());

var cur_ecg_data = [];
var last_frame_id = 0;

var cur_id = 0;
var prev_id = 0;

var ecg_data_queue = [];
var ecg_data_queue_init = 0;
var sample_length = 0;
var queue_length = 800;
var response_length = 1200;

var ecg_data = {};
ecg_data.bpm = 0;
ecg_data.HRV_params = [];
ecg_data.RR_id = -1;
ecg_data.RR_cur = 0;
ecg_data.RR_prev = 0;
ecg_data.temperature = 0;
ecg_data.skin_res = 0;
ecg_data.battery_mv = 0;
ecg_data.acc_x = 0;
ecg_data.acc_y = 0;
ecg_data.acc_z = 0;
ecg_data.steps = 0;
ecg_data.steps_rate = 0;
ecg_data.hrv_parameter = 1.0;

var history_pos = 0;
var max_history_len = 12*3600*update_rate;
var bpm_history = [];
var hrv_history = [];
var gsr_history = [];
var step_rate_history = [];
var temp_history = [];
var batt_history = [];
var acc_x_history = [];
var acc_y_history = [];
var acc_z_history = [];

var rr_history_pos = 0;
var rr_prev_id = -1;
var rr_cur_history = [];
var rr_prev_history = [];

var prev_steps = 0;

update_history_values = function()
{
	if(in_fw_upload > 0) return;
	history_pos++;
	if(history_pos >= max_history_len)
	{
		bpm_history.pop();
		hrv_history.pop();
		gsr_history.pop();
		step_rate_history.pop();
		temp_history.pop();
		batt_history.pop();
		acc_x_history.pop();
		acc_y_history.pop();
		acc_z_history.pop();
	}

	ecg_data.steps_rate *= 0.9;
	var dst = ecg_data.steps - prev_steps;
	if(dst > 4) dst = 4; 
	if(dst < 0) dst = 0;
	prev_steps = ecg_data.steps;
	ecg_data.steps_rate += 0.1 * dst;

	bpm_history.unshift(ecg_data.bpm);
	hrv_history.unshift(ecg_data.hrv_parameter);
	gsr_history.unshift(ecg_data.skin_res);
	step_rate_history.unshift(ecg_data.steps_rate);
	temp_history.unshift(ecg_data.temperature);
	batt_history.unshift(ecg_data.battery_mv);
	acc_x_history.unshift(ecg_data.acc_x);
	acc_y_history.unshift(ecg_data.acc_y);
	acc_z_history.unshift(ecg_data.acc_z);

	if(rr_prev_id != ecg_data.RR_id)
	{
		rr_prev_id = ecg_data.RR_id;
		rr_history_pos++;
		if(rr_history_pos >= max_history_len)
		{
			rr_cur_history.pop();
			rr_prev_history.pop();
		}
		rr_cur_history.unshift(ecg_data.RR_cur);
		rr_prev_history.unshift(ecg_data.RR_prev);

		console.log("RR: " + ecg_data.RR_prev + "  " + ecg_data.RR_cur);

		if(ecg_data.RR_prev > 0 && ecg_data.RR_cur > 0)
		{
			var rel = ecg_data.RR_cur / ecg_data.RR_prev;
			if(rel < 1) rel = 1 / rel;
		        var false_detect = 0;
		        if(rel > 1.7 && rel < 2.3) false_detect = 1;
		        if(rel > 2.7 && rel < 3.3) false_detect = 1;
		        if(rel > 3.8) false_detect = 1;
		        if(false_detect < 1)
			{
			    ecg_data.hrv_parameter *= 0.95;
			    ecg_data.hrv_parameter += 0.05*rel;
				console.log("hrv: " + ecg_data.hrv_parameter);
			}
		}
	}
	setTimeout(update_history_values, 1000 / update_rate);
}

var battery_level = 0;

var last_saved_id = -1;

var serial_stream = Buffer.alloc(1);
var uart_prefix = [79, 213];

var unit_list = [];

var data_id = 0;

//enum in C, hope won't need to change it, only add fields later
const param_batt_bpm = 0;
const param_sdnn = 1;
const param_skin_res = 2;
const param_lastRR = 3;
const param_imu_acc = 4;
const param_imu_steps = 5;
const param_pnn_bins = 6;
const param_end = 7;
const param_emg_spectrum = 8;

decode_acc = function(acc)
{
	var vv = acc - 128.0;
	var vm = vv;
	if(vm < 0) vm = -vm;
	var res = 0;
	if(vm > 100) //for values less than -12 and more than 12 m/s^2 precision is 0.5
	{
		res = 12.0 + (vm - 100) / 2.0;
	}
	else if(vm > 50) //for values from -12...-2 and 2...12 m/s^2 precision is 0.2
	{
		res = 2.0 + (vm - 50) / 5.0;
	}
	else
		res = vm / 25.0; 
//for values from -2 to 2 m/s^2 precision is 0.04
	if(vv < 0) res = -res;
	return res;
}

var prev_received_rr_id = -11;
var prev_received_rr = 0;

parse_direct_packet_uecg = function(pack)
{
	var message_length = pack[0];
//	if(message_length > maxlen) return -1; //not enough received data
	var packet_id = pack[1];
	var ppos = 2;
	var unit_id = (pack[ppos++]<<24);
	unit_id |= (pack[ppos++]<<16);
	unit_id |= (pack[ppos++]<<8);
	unit_id |= pack[ppos++];

	var data_points = pack[ppos++];
//	console.log('id: ' + unit_id + ' points ' + data_points);
	if(data_points < 6) return ; //at least 6 data points per valid uECG packet
	if(data_points > 31) return ; //too many
	
	var data_len = 7+5+data_points*2+1;
	if(message_length == data_len + 1) //1-byte checksum
	{
		var check = 0;
		for(var x = 0; x < message_length-1; x++)
		{
			check += pack[x];
		}
		check = check%256;
		if(check != pack[message_length-1])
		{
			console.log("check " + pack[message_length-1] + " chksum " + check);
			return; //checksum failed
		}
	}
	else //2-byte checksum
	{
		var check = 0;
		for(var x = 0; x < message_length-2; x++)
		{
			check += pack[x];
		}
		check = check%256;
		var check_e = 0;
		for(var x = 0; x < message_length-1; x+=2)
		{
			check_e += pack[x];
		}
		check_e = check_e%256;

		if(check != pack[message_length-2] || check_e != pack[message_length-1])
		{
			console.log("check " + pack[message_length-1] + " chksum " + check + " chksum_e " + check_e);
			return; //checksum failed
		}
	}	
	var uecg_res = [];
	uecg_res.ID = unit_id;

	var param_id = pack[ppos]&0xF; //parameter ID
	var param_mod = pack[ppos]>>4; //parameter modifier

	if(param_id > param_emg_spectrum) return; //error sneaked past checksum

	ppos++;
	var param_hb = pack[ppos++]; //higher byte (notation from old version where 2 bytes of header payload were used)
	var param_lb = pack[ppos++]; //lower byte
	var param_tb = pack[ppos++]; //third byte - wasn't present in old version
		
	uecg_res.param_id = param_id;
	uecg_res.last_pack_id = packet_id;

	var is_good = 0;
	var unit_index = 0;
	for(var x = 0; x < unit_list.length; x++) //check if ID is in the list
		if(unit_list[x].ID == unit_id)
		{
			is_good = 1;
			unit_index = x;
			break;
		}
	
	if(!is_good)//add to list
	{
		uecg_res.protocol_version = 0;
		unit_list.push(uecg_res);
		unit_index = unit_list.length-1;
	}

	if(unit_index != 0) return;

	if(unit_list[unit_index].last_pack_id == packet_id)
	{
//		console.log("duplicate pack");
//		consq_d_cnt++;
//		printf("duplicate, pack id %d (repeated %d)\n", packet_id, consq_d_cnt);
//		printf("%d %d %d %d\n", pack[0], pack[1], pack[2], pack[3]);
//		printf("duplicate %d\n", packet_id);
		return ;			
	}

//check for data integrity
	var prev_val = 0;
	for(var x = 0; x < data_points; x++)
	{
		var val = pack[ppos]*256 + pack[ppos+1];
		if(val > 32767) val = -65535 + val;
		if(x > 0)
		{
			var dv = val - prev_val;
			if(dv < 0) dv = -dv;
			if(dv > 11111500) return;
		}
		prev_val = val;
	}


	var pack_dist = packet_id - unit_list[unit_index].last_pack_id;
	if(unit_list[unit_index].last_pack_id > 80 && packet_id < 50)
		pack_dist = packet_id + (129-unit_list[unit_index].last_pack_id);
	if(pack_dist > 50) pack_dist = 50;
	unit_list[unit_index].last_pack_id = packet_id;
//	consq_d_cnt = 0;
//	console.log("non duplicate " + packet_id + " unit " + unit_index);
//	printf("pack id %d version %d dev idx %d param %d\n", packet_id, packet_version, unit_index, param_id);

//	float lost_packs = packet_id - last_packs_ids[unit_index] - 1;
//	if(lost_packs < 0) lost_packs += 256;
//	lost_packs_avg[unit_index] *= 0.99;
//	if(lost_packs < 15) lost_packs_avg[unit_index] += 0.01*lost_packs;
//	else 
//	{
//		lost_packs_avg[unit_index] *= 0.8;
//		lost_packs_avg[unit_index] += 0.2 * lost_packs;
//	}
//	if(lost_packs_avg[unit_index] < 30)
//		uecg_res->radio_quality = 100 - lost_packs_avg[unit_index]*3;
//	else
//		uecg_res->radio_quality = 0;

//	last_packs_ids[unit_index] = packet_id;
 
	if(param_id == param_batt_bpm) //battery level and BPM data
	{
		uecg_res.battery_mv = 2000 + param_hb * 10;
		uecg_res.protocol_version = param_lb;
		unit_list[unit_index].protocol_version = uecg_res.protocol_version;
		if(uecg_res.protocol_version == 4)
			uecg_res.bpm = param_tb / 1.275;
		else
			uecg_res.bpm = param_tb;
	}
	if(param_id == param_lastRR) //2 RR intervals: current and previous
	{ //If connections is poor - we might miss packet with RR data, but when 2 RRs are in a single packet
	//we can be sure that they represent 2 consequitive beats even if some data were lost in between
		var rr_id = param_hb;
		var rr_val = param_lb*256 + param_tb;

		if(rr_id == prev_received_rr_id + 1 || (prev_received_rr_id == 15 && rr_id == 0))
		{
			uecg_res.rr_id = rr_id;
			uecg_res.rr_cur = rr_val;
			uecg_res.rr_prev = prev_received_rr;
		}
		prev_received_rr = rr_val;
		prev_received_rr_id = rr_id;

/* BLE VERSION	var v1_h = param_hb>>4; //we use 12 bits for each RR, so max value is 4096 milliseconds. For any normal beats it's enough - but for serious anomalies it's not reliable
		var v1_l = param_lb; //_hb stores 4 higher bits for each value, lb and tb - lower 8 bits for each value
		var v2_h = param_hb&0x0F;
		var v2_l = param_tb;
		
		var rr1_val = (v1_h<<8) + v1_l; //current RR interval
		if(rr1_val > 32767) rr1_val = -65536 + rr1_val; //explicitly get rid of the sign:
		// when translating to other languages it's more reliable than relying on type properties
		var rr2_val = (v2_h<<8) + v2_l; //previous RR interval
		if(rr2_val > 32767) rr2_val = -65536 + rr2_val;

		if(unit_list[unit_index].protocol_version == 4)
		{
			rr1_val /= 1.275;
			rr2_val /= 1.275;
		}

		console.log("RR: " + param_mod + " " + rr1_val + " " + rr2_val);
		uecg_res.rr_id = param_mod; //param_mod contains RR identifier, from 0 to 15 - increased by 1 with each detected
		uecg_res.RR_cur = rr1_val; //beat on the device, required to identify duplicates: protocol sends the same data many
		uecg_res.rr_prev = rr2_val; //times, so we need to keep track of them*/
	}
	if(param_id == param_skin_res) //skin resistance, translation into kOhms will be provided later - not obvious even for us :)
	{
		uecg_res.skin_res = (param_hb<<8) | param_lb;
		if(param_tb < 50)
			uecg_res.temperature = -20 + param_tb;
		else if(param_tb > 220)
			uecg_res.temperature = 47 + (param_tb-220);
		else 
			uecg_res.temperature = 30 + 0.1 * (param_tb - 50);
	}
	if(param_id == param_imu_steps) //step counter, overflows at 65536
	{
		uecg_res.steps = (param_hb<<8) | param_lb;
	}
	if(param_id == param_imu_acc) //current acceleration by x,y,z packed in a way to increase precision around 0, so angle can be extracted more or less reliably
	{
		uecg_res.acc_x = decode_acc(param_hb);
		uecg_res.acc_y = decode_acc(param_lb);
		uecg_res.acc_z = decode_acc(param_tb);
	}

	uecg_res.RR_data = Array();
	if(pack_dist > 1)
	{
		var val = pack[ppos]*256 + pack[ppos+1];
		if(val > 32767) val = -65535 + val;
		for(var x = 0; x < data_points*pack_dist; x++)
			uecg_res.RR_data.push(val);
	}
	for(var x = 0; x < data_points; x++)
	{
//		var val = (pack[ppos]<<8) | pack[ppos+1];
		var val = pack[ppos]*256 + pack[ppos+1];
		if(val > 32767) val = -65535 + val;
		ppos += 2;
		if(val > 50000) console.log(val);
		uecg_res.RR_data.push(val);
		if(x > 0)
		{
			var dv = uecg_res.RR_data[x] - uecg_res.RR_data[x-1];
			if(dv < 0) dv = -dv;
//			if(dv > 500) return -1;
		}
//		printf("ecg %d %d\n", x, val);
	}
	return uecg_res;
}


serial_process = function(data)
{
	serial_stream = Buffer.concat([serial_stream, data]);
	var stream_length = serial_stream.length;
	var processed_pos = 0;
	for(var x = 0; x < stream_length-25; x++) //25 - always less than minimum valid packet length
	{
		if(serial_stream[x] == uart_prefix[0] && serial_stream[x+1] == uart_prefix[1])
		{//we detected possible start of the packet, trying to make sense of it
			var rssi_level = serial_stream[x+2];
			var ppos = x + 3;
			var message_length = serial_stream[ppos];
			if(ppos + message_length >= stream_length)
				break;
			processed_pos = ppos + message_length - 1;
			res = parse_direct_packet_uecg(serial_stream.slice(ppos, ppos+message_length));
			if(res === undefined)
				;
			else
			{
				if(res.battery_mv !== undefined) ecg_data.battery_mv = res.battery_mv;
				if(res.acc_x !== undefined)
				{
					ecg_data.acc_x = res.acc_x;
					ecg_data.acc_y = res.acc_y;
					ecg_data.acc_z = res.acc_z;
				}
				if(res.steps !== undefined) ecg_data.steps = res.steps;
				if(res.skin_res !== undefined) ecg_data.skin_res = res.skin_res;
				if(res.bpm !== undefined) ecg_data.bpm = res.bpm;
				if(res.temperature !== undefined) ecg_data.temperature = res.temperature;
				if(res.rr_id !== undefined)
				{
					if(res.rr_id != ecg_data.RR_id)
					{
						ecg_data.RR_id = res.rr_id;
						ecg_data.RR_cur = res.rr_cur;
						ecg_data.RR_prev = res.rr_prev;
					}
				}

				for(var x = 0; x < res.RR_data.length; x++)
				{
					if(ecg_data_queue.length > 400)
					      ecg_data_queue.shift();
					ecg_data_queue.push(res.RR_data[x]);
					ecg_data_queue[0] = data_id;
					data_id++;
				}
//				console.log(res.last_pack_id);
			}
//			console.log(res);
//			if(res
//			console.log(message_length);
		}
//		console.log(serial_stream[x]);
	}
	if(processed_pos > 0)
		serial_stream = serial_stream.slice(processed_pos, serial_stream.length);
	if(serial_stream.length > 4096)
		serial_stream = serial_stream.slice(2048, serial_stream.length);
//	console.log(serial_stream.length);
//	console.log(data.length);
}

app.get('/',
  function (request, result){
    result.sendFile(__dirname + '/ecg.html');
  });

app.get('/get-ecg-data',
  function (request, result){	
    result.json(ecg_data_queue.slice(0, response_length)); } );

app.get('/get-ecg-object',
  function (request, result){	
    result.json(ecg_data); } );

app.get('/get-params-history',
  function (request, result){
  var last_hist = 0;
  if(request.query.hist_id !== undefined) last_hist = request.query.hist_id;
  var uecg_hist = {};
  uecg_hist.hist_pos = history_pos;
  uecg_hist.h_bpm = bpm_history.slice(0, history_pos - last_hist);
  uecg_hist.h_hrv = hrv_history.slice(0, history_pos - last_hist);
  uecg_hist.h_gsr = gsr_history.slice(0, history_pos - last_hist);
  uecg_hist.h_step_rate = step_rate_history.slice(0, history_pos - last_hist);
  uecg_hist.h_temp = temp_history.slice(0, history_pos - last_hist);
  uecg_hist.h_batt = batt_history.slice(0, history_pos - last_hist);
  uecg_hist.h_acc_x = acc_x_history.slice(0, history_pos - last_hist);
  uecg_hist.h_acc_y = acc_y_history.slice(0, history_pos - last_hist);
  uecg_hist.h_acc_z = acc_z_history.slice(0, history_pos - last_hist);

  var last_hist_rr = 0;
  if(request.query.rr_hist_id !== undefined) last_hist_rr = request.query.rr_hist_id;
  uecg_hist.hist_pos_rr = rr_history_pos;
  uecg_hist.h_rr_cur = rr_cur_history.slice(0, rr_history_pos - last_hist_rr);
  uecg_hist.h_rr_prev = rr_prev_history.slice(0, rr_history_pos - last_hist_rr);

    result.json(uecg_hist); } );




var upload_pending = 0;
var upload_sent_bytes = 0;

var upload_started = 0;
var last_response_time = 0;
var bin_data;
var response_pending = 0;
var need_resend = 0;
/*
void wait_device_radio()
{
	upload_sent_bytes = 0;
	open_device();
	upload_mode_usb = 0;
	upload_pending = 1;
	response_pending = 0;
	need_resend = 0;
	upload_started = 0;
}*/

var uart_prefix_serial = [223, 98];
var uart_prefix_radio_tx = [29, 115];
var uart_prefix_radio_rx = [79, 213];

const err_code_ok = 11;
const err_code_toolong = 100;
const err_code_wrongcheck = 101;
const err_code_wronglen = 102;
const err_code_packmiss = 103;
const err_code_timeout = 104;

var upload_pack_id = 0;

var last_err_code = 0;
var sent_packet = new ArrayBuffer(64);

var response_pos = 0;
var response_buf = Buffer.alloc(1);

fw_upload_process = function(data)
{
	if(last_err_code == err_code_timeout)
	{
		console.log("got bytes: " + data.length);
		console.log(data);
	}
	response_buf = Buffer.concat([response_buf, data]);
	var response_pos = response_buf.length;
	if(response_pos < 2) return;
	var processed_pos = 0;
//	console.log("RP: " + response_pos);
	for(var x = 0; x < response_pos-3-fw_upload_RF*5; x++) 
	{
		if(fw_upload_USB > 0)
			if(response_buf[x] == uart_prefix_serial[0] && response_buf[x+1] == uart_prefix_serial[1])
		{
			var pack_id = response_buf[x+2];
			var ret_code = response_buf[x+3];
			last_err_code = ret_code;
//			console.log("last_err_code " + last_err_code + " response_pos " + response_pos + " x " + x);
			if(pack_id == upload_pack_id && ret_code == err_code_ok)
			{
				response_pending = 2;
				need_resend = 0;
			}
			else
			{
				response_pending = 2;
				need_resend = 1;
			}
			processed_pos = x + 3;
		}

		if(fw_upload_USB == 0)
			if(response_buf[x] == uart_prefix_radio_rx[0] && response_buf[x+1] == uart_prefix_radio_rx[1])
		{
			//x+2 - rssi
			//x+3 - length
			//x+3 - protocol packet id
			//x+4 - uart_prefix_serial[0]
			//x+5 - uart_prefix_serial[1]
			var pack_id = response_buf[x+7];
			var ret_code = response_buf[x+8];
//			printf("radio pack: %d %d\n", pack_id, ret_code);
			console.log("radio pack: " + pack_id + " " + ret_code);
			last_err_code = ret_code;
			if(pack_id == upload_pack_id && ret_code == err_code_ok)
			{
				response_pending = 2;
				need_resend = 0;
			}
			else if(pack_id == upload_pack_id && ret_code == err_code_packmiss)
			{
				response_pending = 2;
				need_resend = 0;
			}
			else
			{
				response_pending = 2;
				need_resend = 1;
			}
			processed_pos = x + 8;
		}
	}
	if(processed_pos > 0)
		response_buf = response_buf.slice(processed_pos, response_buf.length);
	if(response_buf.length > 8192)
		response_buf = response_buf.slice(4096, response_buf.length);
}

var resend_cnt = 0;

process_upload = function()
{
	var cur_time = new Date().getTime();
	if(upload_pending == 0) return;
//	if(bin_file_length < 1 || bin_file == NULL) return; //file not ready
	if(resend_cnt > 10 && bin_data.byteLength - upload_sent_bytes < 32)
		response_pending = 2;
	if(resend_cnt > 200)
	{
		upload_pending = 0;
		console.log("upload failed");
	}
	if(response_pending)
	{
		if(response_pending == 2)
		{			
			if(upload_sent_bytes >= bin_data.byteLength)
			{
				upload_pending = 0;
				console.log("...done!");	
			}
//			if(bin_file_length > 0) gtk_progress_bar_set_fraction(pb_upload_progress, (float)upload_sent_bytes / (float)bin_file_length);
			response_pending = 0;
		}
		if(cur_time - last_response_time > 300)
		{
			last_response_time = cur_time;
			last_err_code = err_code_timeout;
			response_pending = 0;
			need_resend = 1;
		}
		return;
	}
	if(need_resend)
	{
		resend_cnt++;
		last_response_time = cur_time;
		if(last_err_code != err_code_timeout)
			console.log("...resend requested, error code " + last_err_code);
		else
			console.log("...timeout, resending, id: " + sent_packet[0]);

		if(fw_upload_USB > 0)
			send_data_serial(sent_packet, 33);
		else
			send_data_radio(sent_packet, 33);
		response_pending = 1;
		need_resend = 0;
		return;
	}
	resend_cnt = 0;
	if(!upload_started)
	{
		last_response_time = cur_time;
		console.log("starting upload...");	
		var upload_start_code = [0x10, 0xFC, 0xA3, 0x05, 0xC0, 0xDE, 0x11, 0x77];
		upload_pack_id = 100;
		var ppos = 0;
		for(var n = 0; n < 64; n++) sent_packet[n] = 0; //clear
		
		sent_packet[ppos++] = upload_pack_id;
		for(var n = 0; n < 8; n++)
			sent_packet[ppos++] = upload_start_code[n];
		var code_length = bin_data.byteLength;
		sent_packet[ppos++] = (code_length>>24)&0xFF;
		sent_packet[ppos++] = (code_length>>16)&0xFF;
		sent_packet[ppos++] = (code_length>>8)&0xFF;
		sent_packet[ppos++] = code_length&0xFF;

		console.log("size bytes: " + sent_packet[ppos-4] + " " + sent_packet[ppos-3] + " " + sent_packet[ppos-2] + " " + sent_packet[ppos-1]);

		if(fw_upload_USB > 0)
			send_data_serial(sent_packet, 33);
		else
			send_data_radio(sent_packet, 33);

		upload_started = 1;
		upload_sent_bytes = 0;
		response_pending = 1;
		need_resend = 0;
	}
	else
	{
		last_response_time = cur_time;
		upload_pack_id++;
		upload_pack_id = upload_pack_id&0xFF;
		var ppos = 0;
		for(var n = 0; n < 64; n++) sent_packet[n] = 0; //clear
		
		console.log("sending frame " + upload_pack_id + " bytes remains " + (bin_data.byteLength - upload_sent_bytes));
		
		sent_packet[ppos++] = upload_pack_id;
		for(var w = 0; w < 8; w++)
		{
			for(var n = 0; n < 4; n++)
				sent_packet[ppos++] = bin_data[upload_sent_bytes + w*4 + 3-n];
		}
		
		upload_sent_bytes += 32;
		
		if(fw_upload_USB > 0)
			send_data_serial(sent_packet, 33);
		else
			send_data_radio(sent_packet, 33);
		response_pending = 1;
		need_resend = 0;
	}
}

fw_upload_loop = function()
{
	process_upload();

	if(upload_requested && !upload_pending && !response_pending)
	{
		console.log("upload ended");

		fw_upload_USB = 0;
		fw_upload_RF = 0;

		port.close(function (err) {
			console.log('port closed', err);
			init_serial();
		});

	}
	else
		setTimeout(function() { fw_upload_loop(); }, 10);

}


var tbuf = Buffer.alloc(40);

var busy_writing = 0;

var waiting_buf;
var waiting_len;

retry_write = function()
{
	write_to_device(waiting_buf, waiting_len);
}

write_to_device = function(buf, len)
{
	if(busy_writing)
	{
		waiting_buf = buf;
		waiting_len = len;
		setTimeout(retry_write, 5);
		return;
	}
	busy_writing = 1;
//	var tbuf = Buffer.alloc(len);

	for(var x = 0; x < tbuf.byteLength; x++)
		tbuf[x] = 0;

	for(var x = 0; x < len; x++)
		tbuf[x] = buf[x];
	port.write(tbuf, function(err) {
	busy_writing = 0;
	if (err) {
		return console.log('Error on write: ', err.message)
	}});
}

var sndbuf = Buffer.alloc(256);


send_data_serial = function(data, len)
{
	var pp = 0;
	sndbuf[pp++] = uart_prefix_serial[0];
	sndbuf[pp++] = uart_prefix_serial[1];
	sndbuf[pp++] = len+3; //payload length + checksum
	for(var x = 0; x < len; x++)
		sndbuf[pp++] = data[x];
		
	var check_odd = 0;
	var check_tri = 0;
	var check_all = 0;
	for(var x = 3; x < pp; x++)
	{
		if((x-3)%2 > 0) check_odd += sndbuf[x];
		if((x-3)%3 > 0) check_tri += sndbuf[x];
		check_all += sndbuf[x];
	}
	sndbuf[pp++] = check_odd&0xFF;
	sndbuf[pp++] = check_tri&0xFF;
	sndbuf[pp++] = check_all&0xFF;
		
	sndbuf[pp++] = 0;
	write_to_device(sndbuf, pp);
}

send_data_radio = function(data, len)
{
	var pp = 0;
	sndbuf[pp++] = uart_prefix_radio_tx[0];
	sndbuf[pp++] = uart_prefix_radio_tx[1];
	sndbuf[pp++] = len+3; //payload length + checksum
	for(var x = 0; x < len; x++)
		sndbuf[pp++] = data[x];
	
	var check_odd = 0;
	var check_tri = 0;
	var check_all = 0;
	for(var x = 3; x < pp; x++)
	{
		if((x-3)%2 > 0) check_odd += sndbuf[x];
		if((x-3)%3 > 0) check_tri += sndbuf[x];
		check_all += sndbuf[x];
	}
	sndbuf[pp++] = check_odd&0xFF;
	sndbuf[pp++] = check_tri&0xFF;
	sndbuf[pp++] = check_all&0xFF;
	
	sndbuf[pp++] = 0;
	write_to_device(sndbuf, pp);
}

start_rf_upload = function()
{
//	parser.on('data', function(data) {
//		fw_upload_process(data);
//	});
	port.on('data', function(data) {
		fw_upload_process(data);
	});
	in_fw_upload = 1;

	upload_sent_bytes = 0;


	response_pending = 0;
	need_resend = 0;
	upload_started = 0;

	upload_pending = 1;
	upload_requested = 1;

	console.log("processing radio upload...");
	fw_upload_loop();
}

prepare_fw_mode_parser = function()
{
	parser.on('data', function(data) {
		fw_upload_process(data);
	});

	in_fw_upload = 1;

	response_pending = 0;
	need_resend = 0;
	upload_started = 0;

	upload_pending = 1;
	upload_requested = 1;

	console.log("port found, processing...");
	fw_upload_loop();
}

var fw_port_name = "";

wait_fw_upload_port = function(port_name_list, name_part)
{
	SerialPort.list().then( ports =>
	{
		var completed = 0;
		for(var p = 0; p < ports.length; p++)
		{
			if(port_name_list.indexOf(ports[p].path) < 0 && ports[p].path.includes(name_part))
			{
				fw_port_name = ports[p].path;
				setTimeout(function() {
					port = new SerialPort(fw_port_name, { baudRate: 921600 });
					parser = port.pipe(new ByteLength({length: 1}));
					prepare_fw_mode_parser();
				}, 200);
				completed = 1;
			}
		}
		if(completed == 0) wait_fw_upload_port(port_name_list, name_part);
	},
	err => { console.error('Error listing ports', err) } );

}
wait_fw_upload = function()
{
	name_part = "";
	if(process.platform == 'linux')
	{
		name_part = 'ttyUSB';
	}
	if(process.platform == 'darwin')
	{
		name_part = 'tty';
	}
	if(process.platform == 'win32' || process.platform == 'win64')
	{
		name_part = 'COM';
	}

	console.log('waiting for USB base to upload firmware...');
	SerialPort.list().then( ports =>
	{
		port_name_list = [];
		for(var p = 0; p < ports.length; p++)
			port_name_list[p] = ports[p].path;
		wait_fw_upload_port(port_name_list, name_part);
	},
	err => { console.error('Error listing ports', err) } );

}

process_base_upload = function(req, callback)
{
  bin_data = req.body;
  console.log("got array length: " + bin_data.byteLength);
//  console.log(bin_data);

  fw_upload_USB = 1;
  fw_upload_RF = 0;
	in_fw_upload = 1;

	if(port)
	{
		port.close(function (err) {
			console.log('port closed', err);
			wait_fw_upload();
		});
	}
	else
		wait_fw_upload();

  callback(200, "OK");
  return;
}

process_uecg_upload = function(req, callback)
{
  bin_data = req.body;
//  if(port)
//	  port.close();
  console.log("got array length: " + bin_data.byteLength);
//  console.log(bin_data);

  fw_upload_USB = 0;
  fw_upload_RF = 1;
  start_rf_upload();

  callback(200, "OK");
  return;
}

app.use(bodyParser.raw({type: 'application/octet-stream', limit : '2mb'}));


app.post('/upload-base-fw', function(req, res) {
	process_base_upload(req, function(status, message) {
		res.status(status).json(message);
	});
});

app.post("/upload-uecg-fw", function(req, res) {
  process_uecg_upload(req, function(status, message) {
      res.status(status).json(message);
    });
});

app.get('/get-fw-upload-progress',
  function (request, result){	
	obj = {};
	if(bin_data === undefined)
		obj.progress = 0;
	else
	{
		if(bin_data.byteLength == 0)
			obj.progress = 0;
		else
			obj.progress = upload_sent_bytes / bin_data.byteLength;
	}
    result.json(obj); } );



app.listen(8080);

