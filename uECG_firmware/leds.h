#include "nrf_gpio.h"
#include "nrf.h"

void leds_init();
void leds_set(int r, int g, int b); //0-255, -1 -> don't change
void leds_set_driver(int val);