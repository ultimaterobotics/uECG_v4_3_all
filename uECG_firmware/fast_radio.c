#include "fast_radio.h"
#include "timer_core.h"
#include "nrf.h"

#define PACKET0_S1_SIZE             (8UL)  //!< S1 size in bits
#define PACKET0_S0_SIZE             (1UL)  //!< S0 size in bytes
#define PACKET0_PAYLOAD_SIZE        (8UL)  //!< payload size in bits
#define PACKET1_BASE_ADDRESS_LENGTH (4UL)  //!< base address length in bytes
uint32_t pack_max_length = 64;  //!< static length in bytes
uint32_t pack_payload = 64;  //!< payload size in bytes

uint32_t rx_packet_counter = 0;
uint32_t last_processed_rx_packet = 0;
uint32_t last_end_event_time = 0;
uint32_t last_rx_time = 0;

uint8_t current_mode = 0; // 0 - off, 1 - rx, 2 - tx, 3 - tx->rx
uint8_t rx_packet[256];

void fr_disable()
{
	if(NRF_RADIO->STATE == 0) return; //already disabled
	
    NRF_RADIO->SHORTS          = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
//    NRF_RADIO->TEST            = 0;
    NRF_RADIO->TASKS_DISABLE   = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0) ;
    NRF_RADIO->EVENTS_DISABLED = 0;
}

void fr_poweroff()
{
	fr_disable();
	NRF_RADIO->POWER = 0;
}

void fr_init(int packet_length)
{
	pack_payload = packet_length;
	if(pack_payload > pack_max_length) pack_max_length = pack_payload;
    // Radio config
    NRF_RADIO->TXPOWER   = (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos);
    NRF_RADIO->FREQUENCY = 21UL; 
//    NRF_RADIO->MODE      = (RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos);
//    NRF_RADIO->MODE      = (RADIO_MODE_MODE_Nrf_1Mbit << RADIO_MODE_MODE_Pos);
    NRF_RADIO->MODE      = (RADIO_MODE_MODE_Nrf_250Kbit << RADIO_MODE_MODE_Pos);

    // Radio address config
    NRF_RADIO->PREFIX0     = 0x44332211UL;  // Prefix byte of addresses 3 to 0
    NRF_RADIO->PREFIX1     = 0x88776655UL;  // Prefix byte of addresses 7 to 4
    NRF_RADIO->BASE0       = 0x0EE60DA7UL;  // Base address for prefix 0
    NRF_RADIO->BASE1       = 0x0EE6050CUL;  // Base address for prefix 1-7
    NRF_RADIO->TXADDRESS   = 0x0UL;        // Set device address 0 to use when transmitting
    NRF_RADIO->RXADDRESSES = 0b00000010;        // receive on address 1

    // Packet configuration
    NRF_RADIO->PCNF0 = (PACKET0_S1_SIZE << RADIO_PCNF0_S1LEN_Pos) |
                       (PACKET0_S0_SIZE << RADIO_PCNF0_S0LEN_Pos) |
                       (PACKET0_PAYLOAD_SIZE << RADIO_PCNF0_LFLEN_Pos); //lint !e845 "The right argument to operator '|' is certain to be 0"

    // Packet configuration
    NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
                       (RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos)        |
                       (PACKET1_BASE_ADDRESS_LENGTH << RADIO_PCNF1_BALEN_Pos)    |
                       (pack_max_length << RADIO_PCNF1_STATLEN_Pos)        |
                       (pack_payload << RADIO_PCNF1_MAXLEN_Pos); //lint !e845 "The right argument to operator '|' is certain to be 0"

    // CRC Config
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos); // Number of checksum bits
	NRF_RADIO->CRCINIT = 0b10101010;   // Initial value
	NRF_RADIO->CRCPOLY = 0b100000111;  // CRC poly: x^8+x^2+x^1+1
	
	NRF_RADIO->INTENSET = 0b01000; //END event
//	NRF_RADIO->INTENSET = 0b01010; //END & ADDRESS events
	NVIC_EnableIRQ(RADIO_IRQn);
}

void fr_mode_rx_only()
{
	if(NRF_RADIO->STATE & 0b01000) fr_disable(); //tx state
	else if(!(NRF_RADIO->STATE & 0b011)) fr_disable(); //states 1, 2, 3 responsible for RX
	
	NRF_RADIO->POWER = 1;
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->TASKS_RXEN = 1;
	current_mode = 1;
}
void fr_mode_tx_only()
{
	if(!(NRF_RADIO->STATE & 0b01000)) fr_disable(); //non-tx state
	else if(!(NRF_RADIO->STATE & 0b011)) fr_disable(); //states 9, 10, 11 responsible for TX
	NRF_RADIO->POWER = 1;
	
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
	NRF_RADIO->TASKS_TXEN = 1;
	current_mode = 2;
}
void fr_mode_tx_then_rx()
{
	if(!(NRF_RADIO->STATE & 0b01000)) fr_disable(); //non-tx state
	else if(!(NRF_RADIO->STATE & 0b011)) fr_disable(); //states 9, 10, 11 responsible for TX
//	fr_disable();
	NRF_RADIO->POWER = 1;
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
	NRF_RADIO->TASKS_TXEN = 1;
	for(int x = 0; x < 20; x++) ; //to make sure radio enters TXEN sequence before enabling DISABLED_RXEN shortcut
	NRF_RADIO->SHORTS |= RADIO_SHORTS_DISABLED_RXEN_Msk;
//	NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
	current_mode = 3;
}

void fr_send(uint8_t *pack)
{
	NRF_RADIO->PACKETPTR = (uint32_t)pack;
	fr_mode_tx_only();
}
void fr_listen()
{
	last_processed_rx_packet = rx_packet_counter;
	NRF_RADIO->PACKETPTR = (uint32_t)rx_packet;
	fr_mode_rx_only();
//	NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
}
void fr_send_and_listen(uint8_t *pack)
{
	NRF_RADIO->PACKETPTR = (uint32_t)pack;
	fr_mode_tx_then_rx();
}
int fr_has_new_packet()
{
	return (last_processed_rx_packet != rx_packet_counter);
}
uint32_t fr_get_packet(uint8_t *pack)
{
	for(int x = 0; x < pack_payload; x++)
		pack[x] = rx_packet[x];
	last_processed_rx_packet = rx_packet_counter;
	return last_rx_time;
}

uint32_t irq_cnt = 0;

void fr_irq_handler()
{
	irq_cnt++;
	if(NRF_RADIO->EVENTS_END && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk))
	{
		NRF_RADIO->EVENTS_END = 0;
		last_end_event_time = micros();
		if(current_mode == 1)
		{
			rx_packet_counter++;
			last_rx_time = last_end_event_time;
		}
		if(current_mode == 3) //tx event ended, switched to rx using shorts
		{
			NRF_RADIO->PACKETPTR = (uint32_t)rx_packet;
			current_mode = 1;
		}
	}
}

uint32_t fr_get_irq_cnt()
{
	return irq_cnt;
}

