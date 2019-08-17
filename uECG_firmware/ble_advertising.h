#include <stdint.h>

void prepare_adv_packet(uint8_t *payload, int payload_len);
void config_ble_adv();
void send_adv_packet();
void fr_disable_ble();
uint32_t get_last_rx_time();
void ble_irq_handler();
