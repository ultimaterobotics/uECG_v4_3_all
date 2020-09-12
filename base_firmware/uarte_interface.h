#include <stdint.h>

void uart_init(int pin_TX, int pin_RX);
void uart_send(uint8_t *buf, int length);
int uart_send_remains();
uint8_t get_rx_position();
uint8_t *get_rx_buf();
uint32_t get_rx_buf_length();
void uprintf( const char * format, ... );