#include "nrf_gpio.h"
#include "nrf.h"

uint8_t led_pins[3];
uint32_t led_pin_mask[3];

void leds_init();
void leds_set_pwm(uint8_t led, uint8_t value);
