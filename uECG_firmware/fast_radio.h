#include <stdint.h>

void fr_disable();
void fr_poweroff();
void fr_init(int packet_length);
void fr_mode_rx_only();
void fr_mode_tx_only();
void fr_mode_tx_then_rx();
void fr_send(uint8_t *pack);
void fr_listen();
void fr_send_and_listen(uint8_t *pack);
int fr_has_new_packet();
uint32_t fr_get_packet(uint8_t *pack);

uint32_t fr_get_irq_cnt();
void fr_irq_handler();
