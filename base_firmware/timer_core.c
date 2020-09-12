#include "timer_core.h"
#include "nrf.h"
#include "nrf_gpio.h"

#define TIME_RESOLUTION 250
//32, 16 or 8
#define TIMS_PER_MS 4
//31 for 32, 63 for 16, 125 for 8


volatile uint32_t mcs_time = 0;
volatile uint32_t ms_time = 0;
volatile uint32_t s_time = 0;
volatile uint32_t ms_counter = 0;
volatile uint32_t s_counter = 0;
volatile uint8_t timer_on = 0;

volatile uint8_t phase = 0;

void start_time()
{
	NRF_TIMER2->CC[0] = TIME_RESOLUTION;
	NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;
	NRF_TIMER2->BITMODE = 1; //8-bit
	NRF_TIMER2->PRESCALER = 4; //for 16MHz clock

	NRF_TIMER2->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos); 
//	NRF_TIMER2->SHORTS = 0;
	NRF_TIMER2->SHORTS = 1; //COMPARE[0] -> CLEAR
	
//	NVIC_SetPriority(TIMER2_IRQn, 0);	
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
void stop_time()
{
	NRF_TIMER2->TASKS_STOP = 1;
	timer_on = 0;
}

volatile uint32_t micros()
{
	NRF_TIMER2->TASKS_CAPTURE[2] = 1;
	volatile uint32_t tm = NRF_TIMER2->CC[2];
	tm = NRF_TIMER2->CC[2];
	return mcs_time + tm;
}
uint32_t millis()
{
	return ms_time;
}
uint32_t seconds()
{
	return s_time;
}

void (*timed_event)(void) = 0;

volatile uint8_t in_schedule = 0;

void schedule_event(uint32_t steps_dt, void (*tm_event)(void))
{
//	if(in_schedule) return;
//	in_schedule = 1;
	NRF_TIMER3->TASKS_STOP = 1;
	
	timed_event = *tm_event;

	NRF_TIMER3->TASKS_CLEAR = 1;

	NRF_TIMER3->MODE = TIMER_MODE_MODE_Timer;
	NRF_TIMER3->BITMODE = 3; //32 bits
	NRF_TIMER3->PRESCALER = 0; //16 steps per uS
	NRF_TIMER3->CC[0] = steps_dt;

	NRF_TIMER3->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos); 
	NRF_TIMER3->SHORTS = 1<<8; //compare->stop
	NVIC_EnableIRQ(TIMER3_IRQn);
	NRF_TIMER3->TASKS_START = 1;
}

void TIMER3_IRQHandler(void)
{
	NRF_TIMER3->EVENTS_COMPARE[0] = 0;
	NRF_TIMER3->TASKS_STOP = 1;
	timed_event();
	in_schedule = 0;
}

void TIMER2_IRQHandler(void)
{
	NRF_TIMER2->EVENTS_COMPARE[0] = 0;
//	NRF_TIMER2->TASKS_CLEAR = 1;
	mcs_time += TIME_RESOLUTION;
	ms_counter++;
	if(ms_counter >= TIMS_PER_MS)
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

void delay_ms(uint32_t ms)
{
	if(!timer_on) return;
	volatile uint32_t end = ms_time+ms;
	while(ms_time != end) ;
	return;
}