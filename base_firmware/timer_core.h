#include <stdint.h>

void start_time();
void pause_time();
void resume_time();
void stop_time();
volatile uint32_t micros();
uint32_t millis();
uint32_t seconds();
void delay_ms(uint32_t ms);
void schedule_event(uint32_t steps_dt, void (*tm_event)(void));
