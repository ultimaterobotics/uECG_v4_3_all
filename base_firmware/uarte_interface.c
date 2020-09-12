#include <stdio.h>
#include <stdarg.h>

#include "uart_interface.h"
#include "nrf.h"

uint8_t uart_buf[258];
uint8_t uart_rx_buf[65];
uint32_t uart_buf_length = 256;
volatile uint8_t uart_buf_pos = 0;
volatile uint8_t uart_rx_copied = 0;

void uart_init(int pin_TX, int pin_RX)
{
	NRF_UARTE0->PSEL.RTS = 0xFFFFFFFF;
	NRF_UARTE0->PSEL.CTS = 0xFFFFFFFF;
	NRF_UARTE0->PSEL.TXD = pin_TX;
	NRF_UARTE0->PSEL.RXD = pin_RX;
	
	NRF_GPIO->DIRSET = 1<<pin_TX;
//	nrf_gpio_cfg_output(pin_TX);
//	nrf_gpio_cfg_input(pin_RX, NRF_GPIO_PIN_NOPULL);

	NRF_UARTE0->RXD.PTR = uart_rx_buf;
	NRF_UARTE0->RXD.MAXCNT = 64;

//    NRF_GPIO->DIR |= (1 << pin_TX);
//    NRF_GPIO->DIR &= ~(1 << pin_RX);
//	NRF_UARTE0->BAUDRATE = 0x01D7E000; //115200
//	NRF_UARTE0->BAUDRATE = 0x03AFB000; //230400
//	NRF_UARTE0->BAUDRATE = 0x04000000; //250000
	NRF_UARTE0->BAUDRATE = 0x0F000000;// 0x0EBEDFA4; //921600
	NRF_UARTE0->CONFIG = 0; //no hardware control, no parity
	NRF_UARTE0->INTEN = (1<<4) | (1<<8);
	NRF_UARTE0->SHORTS = (1<<5); //RX->RX
	NRF_UARTE0->ENABLE = 0b1000;

//	NRF_UARTE0->TXD = 0;
//	NRF_UARTE0->TASKS_STARTTX = 1;
	
	NVIC_EnableIRQ(UARTE0_UART0_IRQn);
	
	NRF_UARTE0->TASKS_STARTRX = 1;
}

//uint8_t send_buf[256];
uint8_t send_buf1[256];
uint8_t send_buf2[256];
uint8_t send_buf3[256];
volatile uint8_t send_start = 0;
volatile uint8_t send_end = 0;
volatile uint8_t send_complete = 1;

volatile uint8_t last_copied_pos = 0;

volatile uint8_t buf1_used = 0;
volatile uint8_t buf2_used = 0;
volatile uint8_t buf3_used = 0;
int send_len1 = 0;
int send_len2 = 0;
int send_len3 = 0;

void uart_send(uint8_t *buf, int length)
{
	if(length > 255) return;

	if(buf1_used && buf2_used && buf3_used) return;
	if(buf2_used)
	{
		for(int x = 0; x < length; x++)
			send_buf3[x] = buf[x];
		send_len3 = length;
		buf3_used = 1;
		return;
	}
	if(buf1_used)
	{
		for(int x = 0; x < length; x++)
			send_buf2[x] = buf[x];
		send_len2 = length;
		buf2_used = 1;
		return;
	}
	for(int x = 0; x < length; x++)
		send_buf1[x] = buf[x];
	send_len1 = length;
	buf1_used = 1;
	NRF_UARTE0->TXD.PTR = send_buf1;
	NRF_UARTE0->TXD.MAXCNT = send_len1;
	NRF_UARTE0->TASKS_STARTTX = 1;
/*	
	uint8_t pos = send_end;
	for(int x = 0; x < length; x++)
	{
		send_buf[pos++] = buf[x];
		if(pos == send_start)
			send_start++;
	}
	send_end = pos;

	if(send_complete)
	{
		NRF_UARTE0->TXD = send_buf[send_start];
		send_start++;
		NRF_UARTE0->TASKS_STARTTX = 1;
		send_complete = 0;
	}*/
}

int uart_send_remains()
{
	if(send_end < send_start) return send_end - send_start + 256;
	else return send_end - send_start;
}

void UARTE0_UART0_IRQHandler()// UART0_IRQHandler()
{
    if(NRF_UARTE0->EVENTS_ENDTX) 
	{
		NRF_UARTE0->EVENTS_ENDTX = 0;
		if(buf3_used)
		{
			for(int x = 0; x < send_len2; x++)
				send_buf1[x] = send_buf2[x];
			send_len1 = send_len2;
			for(int x = 0; x < send_len3; x++)
				send_buf2[x] = send_buf3[x];
			send_len2 = send_len3;
			buf3_used = 0;
			NRF_UARTE0->TXD.PTR = send_buf1;
			NRF_UARTE0->TXD.MAXCNT = send_len1;
			NRF_UARTE0->TASKS_STARTTX = 1;
		}
		else if(buf2_used)
		{
			for(int x = 0; x < send_len2; x++)
				send_buf1[x] = send_buf2[x];
			send_len1 = send_len2;
			buf2_used = 0;
			NRF_UARTE0->TXD.PTR = send_buf1;
			NRF_UARTE0->TXD.MAXCNT = send_len1;
			NRF_UARTE0->TASKS_STARTTX = 1;
		}
		else
		{
			buf1_used = 0;
			send_complete = 1;
		}
    }  
	if(NRF_UARTE0->EVENTS_ENDRX) 
	{
		for(int x = last_copied_pos; x < 64; x++)
			uart_buf[uart_buf_pos++] = uart_rx_buf[x];
		last_copied_pos = 0;
		NRF_UARTE0->RXD.PTR = uart_rx_buf;
		NRF_UARTE0->RXD.MAXCNT = 64;
		NRF_UARTE0->EVENTS_ENDRX = 0;
		//handled by shorts
//		NRF_UARTE0->TASKS_STARTRX = 1; 
    }

}

uint8_t get_rx_position()
{
	if(NRF_UARTE0->RXD.AMOUNT != last_copied_pos)
	{
		for(int x = last_copied_pos; x < NRF_UARTE0->RXD.AMOUNT; x++)
			uart_buf[uart_buf_pos++] = uart_rx_buf[x];
		last_copied_pos = NRF_UARTE0->RXD.AMOUNT;
	}
	return uart_buf_pos;
}
uint8_t *get_rx_buf()
{
	return uart_buf;
}
uint32_t get_rx_buf_length()
{
	return uart_buf_length;
}

uint8_t umsg[256];

void uprintf( const char * format, ... )
{
	va_list args;

	va_start(args, format);
	int ulen = vsnprintf(umsg, 255, format, args);
	va_end(args);
	uart_send(umsg, ulen);
}