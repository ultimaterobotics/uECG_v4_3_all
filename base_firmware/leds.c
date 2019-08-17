#include "leds.h"

uint8_t leds_pwm[3];

void leds_start_pwm_timer()
{
	NRF_TIMER0->CC[0] = 8;
	NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
	NRF_TIMER0->BITMODE = 1; //8-bit
//	NRF_TIMER2->PRESCALER = 4; //16x prescaler
	NRF_TIMER0->PRESCALER = 7;

	NRF_TIMER0->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos); 
	NRF_TIMER0->SHORTS = 0;
//	NRF_TIMER2->SHORTS = 1; //COMPARE[0] -> CLEAR
	
	NVIC_EnableIRQ(TIMER0_IRQn);
	NRF_TIMER0->TASKS_START = 1;
}
void leds_pause_pwm()
{
	NVIC_DisableIRQ(TIMER0_IRQn);
}
void leds_resume_pwm()
{
	NVIC_EnableIRQ(TIMER0_IRQn);
}

uint8_t pwm_counter = 0;

void TIMER0_IRQHandler(void)
{
	NRF_TIMER0->EVENTS_COMPARE[0] = 0;
	NRF_TIMER0->TASKS_CLEAR = 1;
	pwm_counter += 2;
	
	for(int x = 0; x < 3; x++)
		if(pwm_counter < leds_pwm[x])
			NRF_GPIO->OUTSET = led_pin_mask[x];
		else
			NRF_GPIO->OUTCLR = led_pin_mask[x];
}

void leds_stop_pwm_timer()
{
	NRF_TIMER0->TASKS_SHUTDOWN = 1;
}
void leds_powerup_pwm_timer()
{
	NRF_TIMER0->TASKS_START = 1;
}

void leds_init()
{
	uint8_t lp[8] = {26, 27, 28};
	
	for(int x = 0; x < 3; x++)
	{
		led_pins[x] = lp[x];
		led_pin_mask[x] = 1<<led_pins[x];
		nrf_gpio_cfg_output(led_pins[x]);
	}	
	leds_start_pwm_timer();
}

void leds_set_pwm(uint8_t led, uint8_t value)
{
	if(led > 2) return;
	int v = value;
	v *= v;
	v >>= 8;
	leds_pwm[led] = v;
}
