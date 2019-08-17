/**
 */

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "leds.h"
#include "fast_radio.h"
#include "timer_core.h"
#include "uart_interface.h"

//openocd -f interface/stlink-v2.cfg -f target/nrf52.cfg
//flash write_image erase build/nrf52832_xxaa.hex

uint8_t pin_led1 = 26;
uint8_t pin_led2 = 27;
uint8_t pin_led3 = 28;

uint8_t pin_tx = 5;
uint8_t pin_rx = 4;


int data_packet_len = 64;
int sync_packet_len = 64;
uint8_t sync_packet[120];
uint8_t data_packet[120];

uint16_t cycle_id = 0;

uint8_t cycle_packet[260];

uint32_t last_uart_pos = 0;

uint32_t uart_buf_length;

int pack_length = 0;
int pack_state = 0;
int pack_begin = 0;

uint8_t ble_mode = 0;
uint8_t ble_timed = 0;

void check_uart_packet()
{
	int pos = last_uart_pos;
	int upos = get_rx_position();
	if(pos == upos) return;
	int unproc_length = 0;
	unproc_length = upos - pos;
	if(unproc_length < 0) unproc_length += uart_buf_length;
	uint8_t *ubuf = get_rx_buf();
	
	for(int pp = pos; pp != upos; pp++)
	{
		if(pp >= uart_buf_length) pp = 0;
		if(pack_state == 0 && ubuf[pp] == 29)
		{
			pack_state = 1;
			continue;
		}
		if(pack_state == 1)
		{
			if(ubuf[pp] == 115) pack_state = 2;
			else pack_state = 0;
			continue;
		}
		if(pack_state == 2)
		{
			pack_length = ubuf[pp];
			if(pack_length >= uart_buf_length-2) //something is wrong or too long packet
			{
				pack_length = 0;
				pack_state = 0;
				continue;
			}
			pack_begin = pp+1;
			if(pack_begin >= uart_buf_length) pack_begin = 0;
			pack_state = 3;
			continue;
		}
		if(pack_state == 3)
		{
			int len = pp - pack_begin;
			if(len < 0) len += uart_buf_length;
			if(len >= pack_length)
			{
				int bg = pack_begin;
				for(int x = 0; x < pack_length; x++)
				{
					sync_packet[x] = ubuf[bg++];
					if(bg >= uart_buf_length) bg = 0;
				}
				int cmd_packet = 0;
				if(sync_packet[0] == 177 && sync_packet[1] == 103 && sync_packet[2] == 39)
				{
					cmd_packet = 1;
					fr_ble_mode(37);
					fr_listen();
					ble_mode = 1;
					ble_timed = 0;
				}
				if(sync_packet[0] == 177 && sync_packet[1] == 103 && (sync_packet[2] >= 40 && sync_packet[2] <= 42))
				{
					cmd_packet = 1;
					fr_disable();
					if(sync_packet[2] == 40) fr_ble_mode(37);
					if(sync_packet[2] == 41) fr_ble_mode(38);
					if(sync_packet[2] == 42) fr_ble_mode(39);
					fr_listen();
					ble_mode = 1;
					ble_timed = 1;
				}
				if(sync_packet[0] == 103 && sync_packet[1] == 177 && sync_packet[2] == 39)
				{
					cmd_packet = 1;
					fr_init(data_packet_len);
					fr_listen();
					ble_mode = 0;
				}

				if(!cmd_packet)
					fr_send_and_listen(sync_packet);
				else
					uart_send(sync_packet, pack_length+2);
				pack_state = 0;
			}
		}
	}
	last_uart_pos = upos;
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
	NRF_CLOCK->TASKS_HFCLKSTOP = 1;
}

int main(void)
{
	NRF_UICR->NFCPINS = 0;
	fast_clock_start();

	uart_buf_length = get_rx_buf_length();

	start_time();
	
	leds_init();
	
	leds_set_pwm(0, 220);
	leds_set_pwm(1, 0);
	leds_set_pwm(2, 200);
	delay_ms(1000);

	fr_init(data_packet_len);
	fr_mode_tx_only();

	uart_init(pin_tx, pin_rx);
	
	for(int n = 0; n < 50; n++)
		cycle_packet[n] = 0;

	int data_count = 0;
	uint32_t last_rx_packet_time;
	
	fr_listen();
	while (1)
	{
		if(ble_mode)
		{
			if(fr_has_new_packet2())
			{
				last_rx_packet_time = fr_get_packet2(data_packet);
				data_count++;
				if(NRF_RADIO->CRCSTATUS == 0)
				{
					fr_listen();
					continue;
				}
				uint8_t message_length = 37;
				uint8_t rssi_state = NRF_RADIO->RSSISAMPLE;
				cycle_packet[0] = 79;
				cycle_packet[1] = 213;
				cycle_packet[2] = rssi_state;
				int st_pos = 3;
				if(ble_timed)
				{
					cycle_packet[3] = (last_rx_packet_time>>24)&0xFF;
					cycle_packet[4] = (last_rx_packet_time>>16)&0xFF;
					cycle_packet[5] = (last_rx_packet_time>>8)&0xFF;
					cycle_packet[6] = (last_rx_packet_time)&0xFF;
					st_pos = 7;
				}

				for(int x = 0; x < message_length; x++)
					cycle_packet[st_pos+x] = data_packet[x];

				uart_send(cycle_packet, st_pos+message_length);
				fr_listen();
				
			}
		}
		else
		{
			if(fr_has_new_packet())
			{
				last_rx_packet_time = fr_get_packet(data_packet);
				data_count++;
				uint8_t message_length = data_packet[0];
				uint8_t rssi_state = NRF_RADIO->RSSISAMPLE;

				cycle_packet[0] = 79;
				cycle_packet[1] = 213;
				cycle_packet[2] = rssi_state;

				for(int x = 0; x < message_length; x++)
					cycle_packet[3+x] = data_packet[x];
				if(uart_send_remains() == 0)
					uart_send(cycle_packet, 3+message_length);
				fr_listen();
			}
		}
		check_uart_packet();
		{
			int dc5 = data_count%510;
			int v = dc5 - 255;
			if(v < 0) v = -v;
			if(ble_mode)
			{
				leds_set_pwm(2, v);
				leds_set_pwm(1, 120);
			}
			else
			{
				leds_set_pwm(1, v);
				leds_set_pwm(2, 120);
			}
		}
	}
}

