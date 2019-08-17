#include <stdint.h>

void start_time();
void pause_time();
void resume_time();
void shutdown_time();
void powerup_time();
uint32_t micros();
uint32_t millis();
uint32_t seconds();
void delay_ms(uint32_t ms);
