#include "leds.h"

uint8_t led_pins[3];
uint32_t led_pin_mask[3];
uint8_t led_driver;
uint32_t led_driver_mask;
uint32_t led_set_mask = 0;

void start_leds_timer()
{
	NRF_TIMER4->MODE = 0;
	NRF_TIMER4->BITMODE = 3; //32 bits
	NRF_TIMER4->PRESCALER = 4;
	NRF_TIMER4->CC[0] = 15000; //r
	NRF_TIMER4->CC[1] = 80; //g
	NRF_TIMER4->CC[2] = 10; //b
	NRF_TIMER4->CC[3] = 21110; //driver, not used with timer anymore
	NRF_TIMER4->CC[4] = 16384;
	NRF_TIMER4->SHORTS = 0b10000; //clear on compare 4

	NRF_TIMER4->INTENSET = 0b11111<<16; 
	NVIC_EnableIRQ(TIMER4_IRQn);

	NRF_TIMER4->TASKS_START = 1;
}

void TIMER4_IRQHandler(void)
{
	if(NRF_TIMER4->EVENTS_COMPARE[4])
	{
		NRF_TIMER4->EVENTS_COMPARE[4] = 0;
		NRF_GPIO->OUTCLR = led_pin_mask[0] | led_pin_mask[1] | led_pin_mask[2];//led_set_mask;
	}
	if(NRF_TIMER4->EVENTS_COMPARE[3])
	{
		NRF_TIMER4->EVENTS_COMPARE[3] = 0;
	}
	if(NRF_TIMER4->EVENTS_COMPARE[2])
	{
		NRF_TIMER4->EVENTS_COMPARE[2] = 0;
		NRF_GPIO->OUTSET = led_pin_mask[2];
	}
	if(NRF_TIMER4->EVENTS_COMPARE[1])
	{
		NRF_TIMER4->EVENTS_COMPARE[1] = 0;
		NRF_GPIO->OUTSET = led_pin_mask[1];
	}
	if(NRF_TIMER4->EVENTS_COMPARE[0])
	{
		NRF_TIMER4->EVENTS_COMPARE[0] = 0;
		NRF_GPIO->OUTSET = led_pin_mask[0];
	}
//	NRF_GPIO->OUTCLR = led_pin_mask[1];
//	NRF_GPIO->OUTCLR = led_set_mask;
}


void leds_init()
{
	uint8_t lp[8] = {6, 7, 8};
	led_driver = 19;
	led_driver_mask = 1<<led_driver;
	NRF_GPIO->DIRSET = led_driver_mask;
	NRF_GPIO->OUTCLR = led_driver_mask;
	
	for(int x = 0; x < 3; x++)
	{
		led_pins[x] = lp[x];
		led_pin_mask[x] = 1<<led_pins[x];
//		nrf_gpio_cfg_output(led_pins[x]);
		NRF_GPIO->DIRSET = led_pin_mask[x];
	}	
	start_leds_timer();
//	NRF_GPIO->OUTSET = led_pin_mask[0];//led_set_mask;
//	NRF_GPIO->OUTCLR = led_pin_mask[1];
}
 
int val_to_cc(int val)
{
	int v2 = val*val;
	v2 >>= 2;
	if(v2 == 0) v2 = 1;
	if(v2 > 16384) v2 = 16384; 
	return v2;
}

void leds_set_driver(int val)
{
	if(val)
		NRF_GPIO->OUTSET = led_driver_mask;
	else
		NRF_GPIO->OUTCLR = led_driver_mask;
}

void leds_set(int r, int g, int b)
{
//	NRF_TIMER4->CC[0] = 16380;
//	NRF_TIMER4->CC[1] = 8000;
//	NRF_TIMER4->CC[2] = 16000;
//	return ;
//	if(r != -1) r = 255 - r;
//	if(g != -1) g = 255 - g;
//	if(b != -1) b = 255 - b;
	if(r == 0)
	{
		NRF_GPIO->OUTSET = led_pin_mask[0];
		led_pin_mask[0] = 0;
	}
	else led_pin_mask[0] = 1<<led_pins[0];
	if(g == 0)
	{
		NRF_GPIO->OUTSET = led_pin_mask[1];
		led_pin_mask[1] = 0;
	}
	else led_pin_mask[1] = 1<<led_pins[1];
	if(b == 0)
	{
		NRF_GPIO->OUTSET = led_pin_mask[2];
		led_pin_mask[2] = 0;
	}
	else led_pin_mask[2] = 1<<led_pins[2];
	
	NRF_TIMER4->CC[0] = val_to_cc(r);
	NRF_TIMER4->CC[1] = val_to_cc(g);
	NRF_TIMER4->CC[2] = val_to_cc(b);
	return;
	
	if(r == 0)
		led_set_mask &= ~led_pin_mask[0];
	else if(r > 0)
	{
		led_set_mask |= led_pin_mask[0];
		NRF_TIMER4->CC[0] = val_to_cc(r);
	}

	if(g == 0)
		led_set_mask &= ~led_pin_mask[1];
	else if(g > 0)
	{
		led_set_mask |= led_pin_mask[1];
		NRF_TIMER4->CC[1] = val_to_cc(g);
	}

	if(b == 0)
		led_set_mask &= ~led_pin_mask[2];
	else if(b > 0)
	{
		led_set_mask |= led_pin_mask[2];
		NRF_TIMER4->CC[2] = val_to_cc(b);
	}
}
