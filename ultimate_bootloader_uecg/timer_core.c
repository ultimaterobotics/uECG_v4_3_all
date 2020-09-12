#include "timer_core.h"
#include "nrf.h"

volatile uint32_t mcs_time = 0;
volatile uint32_t ms_time = 0;
volatile uint32_t s_time = 0;
volatile uint32_t ms_counter = 0;
volatile uint32_t s_counter = 0;
volatile uint8_t phase = 0;
volatile uint8_t timer_on = 0;

void start_time()
{
	NRF_TIMER2->CC[0] = 4;
	NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;
	NRF_TIMER2->BITMODE = 1; //8-bit
	NRF_TIMER2->PRESCALER = 4; //16x prescaler
//	NRF_TIMER2->PRESCALER = 7;

	NRF_TIMER2->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos); 
//	NRF_TIMER2->SHORTS = 0;
	NRF_TIMER2->SHORTS = 1; //COMPARE[0] -> CLEAR
	
	NVIC_EnableIRQ(TIMER2_IRQn);
	NRF_TIMER2->TASKS_START = 1;
	
	timer_on = 1;
}
void pause_time()
{
	NVIC_DisableIRQ(TIMER2_IRQn);
	timer_on = 0;
}
void resume_time()
{
	NVIC_EnableIRQ(TIMER2_IRQn);
	timer_on = 1;
}

uint32_t micros()
{
	return mcs_time;
}
uint32_t millis()
{
	return ms_time;
}
uint32_t seconds()
{
	return s_time;
}

void TIMER2_IRQHandler(void)
{
	NRF_TIMER2->EVENTS_COMPARE[0] = 0;
//	NRF_TIMER2->TASKS_CLEAR = 1;
	mcs_time += 4;
//	mcs_time += 32;
	ms_counter++;
	if(ms_counter >= 125)
	{
//		phase++;
//		if(phase > 3)
//		{
//			phase = 0;
//			return; //once per 4 cycles skip one 32-mcs cycle so average millis = 1000
//		}
		ms_time++;
		ms_counter = 0;
		s_counter++;
		if(s_counter >= 1000)
		{
			s_time++;
			s_counter = 0;
		}
	}
}

void shutdown_time()
{
	NRF_TIMER2->TASKS_SHUTDOWN = 1;
	timer_on = 0;
}
void powerup_time()
{
	NRF_TIMER2->TASKS_START = 1;
	timer_on = 1;
}

void delay_ms(uint32_t ms)
{
	if(!timer_on) return;
	volatile uint32_t end = ms_time+ms;
	while(ms_time != end) ;
	return;
}