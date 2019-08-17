#include "ble_advertising.h"
#include "nrf.h"
#include "timer_core.h"

uint8_t packet[128];
uint8_t mac_addr[6] = {0xBA, 0xBE, 0x00, 0x00, 0x00, 0x88};
int pack_len = 0; //not including first 2 bytes - header and length 
int cur_channel = 37;

uint8_t rx_packet2[64];


//BLE ADV pack:
// [0..1] - header
// [2..7] - MAC
// [8...38] - AD data: 8 - AD type, 9 - AD data length, 10+ data

void prepare_adv_packet(uint8_t *payload, int payload_len)
{
//	cur_channel++;
	if(cur_channel > 39) cur_channel = 37;

	pack_len = 37;//21 + 16;//payload_len; //??? should it be?
	int ppos = 0;
	packet[ppos++] = 0b00100010;// 0x42;//advertising, not accepting connections
	packet[ppos++] = pack_len;

	packet[ppos++] = 0; //S0, S1, length

	uint32_t addr = NRF_FICR->DEVICEID[1];
	mac_addr[2] = (addr>>24)&0xFF;
	mac_addr[3] = (addr>>16)&0xFF;
	mac_addr[4] = (addr>>8)&0xFF;
	mac_addr[5] = (addr)&0xFF;
	for(int x = 0; x < 6; x++)
		packet[ppos++] = mac_addr[5-x];

	packet[ppos++] = 5;
	packet[ppos++] = 0x08;
	packet[ppos++] = 'u';
	packet[ppos++] = 'E';
	packet[ppos++] = 'C';
	packet[ppos++] = 'G';

	packet[ppos++] = 44;// payload_len-2;
		 
	for(int x = 0; x < payload_len; x++)
		packet[ppos++] = payload[x];

}


void config_ble_adv()
{
	
  // Start HFCLK from crystal oscillator. The radio needs crystal to function correctly.
//	NRF_CLOCK->TASKS_HFCLKSTART = 1;
//	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
//	radio_init(37);

//	return;

	NRF_RADIO->POWER = 1;
  // Configure radio with 2Mbit Nordic proprietary mode
//  NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;
  
	NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos;

  NRF_RADIO->PCNF0 = (
    (((1UL) << RADIO_PCNF0_S0LEN_Pos) & RADIO_PCNF0_S0LEN_Msk) |  /* Length of S0 field in bytes 0-1.    */
    (((2UL) << RADIO_PCNF0_S1LEN_Pos) & RADIO_PCNF0_S1LEN_Msk) |  /* Length of S1 field in bits 0-8.     */
    (((6UL) << RADIO_PCNF0_LFLEN_Pos) & RADIO_PCNF0_LFLEN_Msk)    /* Length of length field in bits 0-8. */
  );

  /* Packet configuration */
  NRF_RADIO->PCNF1 = (
    (((37UL) << RADIO_PCNF1_MAXLEN_Pos) & RADIO_PCNF1_MAXLEN_Msk)   |                      /* Maximum length of payload in bytes [0-255] */
    (((0UL) << RADIO_PCNF1_STATLEN_Pos) & RADIO_PCNF1_STATLEN_Msk)   |                      /* Expand the payload with N bytes in addition to LENGTH [0-255] */
    (((3UL) << RADIO_PCNF1_BALEN_Pos) & RADIO_PCNF1_BALEN_Msk)       |                      /* Base address length in number of bytes. */
    (((RADIO_PCNF1_ENDIAN_Little) << RADIO_PCNF1_ENDIAN_Pos) & RADIO_PCNF1_ENDIAN_Msk) |  /* Endianess of the S0, LENGTH, S1 and PAYLOAD fields. */
    (((1UL) << RADIO_PCNF1_WHITEEN_Pos) & RADIO_PCNF1_WHITEEN_Msk)                         /* Enable packet whitening */
  );

	// Configure packet with no S0,S1 or Length fields and 8-bit preamble.
	NRF_RADIO->PCNF0 = (6 << RADIO_PCNF0_LFLEN_Pos) |
			(1 << RADIO_PCNF0_S0LEN_Pos) |
			(2 << RADIO_PCNF0_S1LEN_Pos); 

//			(RADIO_PCNF0_S1INCL_Automatic << RADIO_PCNF0_S1INCL_Pos) |
//			(RADIO_PCNF0_PLEN_8bit << RADIO_PCNF0_PLEN_Pos);
  
//	NRF_RADIO->PCNF0 = (((1UL) << RADIO_PCNF0_S0LEN_Pos) & RADIO_PCNF0_S0LEN_Msk) |  /* Length of S0 field in bytes 0-1.    */
//		(((2UL) << RADIO_PCNF0_S1LEN_Pos) & RADIO_PCNF0_S1LEN_Msk) |  /* Length of S1 field in bits 0-8.     */
//		(((6UL) << RADIO_PCNF0_LFLEN_Pos) & RADIO_PCNF0_LFLEN_Msk);    /* Length of length field in bits 0-8. */

	// Configure static payload length. 4 bytes address, little endian with whitening enabled.
	NRF_RADIO->PCNF1 =  (37 << RADIO_PCNF1_MAXLEN_Pos) |
			(0 << RADIO_PCNF1_STATLEN_Pos) |
			(3  << RADIO_PCNF1_BALEN_Pos) | 
			(RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos) |
			(RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);

	// Configure address Prefix0 + Base0
	//0x8E89BED6 - bed 6 advertising address
	NRF_RADIO->BASE0   = 0x89BED600;
	NRF_RADIO->PREFIX0 = 0x8E;// << RADIO_PREFIX0_AP0_Pos;

	// Use logical address 0 (BASE0 + PREFIX0 byte 0)
	NRF_RADIO->TXADDRESS = 0;// << RADIO_TXADDRESS_TXADDRESS_Pos;
	NRF_RADIO->RXADDRESSES = 1;// << RADIO_TXADDRESS_TXADDRESS_Pos;
  
	// Initialize CRC
//	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos) |
//			(RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos);
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos) |
			(RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos);

	NRF_RADIO->CRCPOLY = 0x00065B;
	NRF_RADIO->CRCINIT = 0x555555;
  

  // Enable fast rampup, new in nRF52
//  NRF_RADIO->MODECNF0 = (RADIO_MODECNF0_DTX_B0 << RADIO_MODECNF0_DTX_Pos) |
//                        (RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos);
                        
	// 4dBm output power
	NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_ADDRESS = 0;	
	
	for(int x = 0; x < 64; x++) rx_packet2[x] = 0;
	
	NRF_RADIO->INTENSET = 0b01000; //END event
	NVIC_EnableIRQ(RADIO_IRQn);
}

void prepare_transmission()
{
	NRF_RADIO->EVENTS_READY = 0U;
	NRF_RADIO->TASKS_TXEN   = 1; 											// Enable radio and wait for ready.
	while (NRF_RADIO->EVENTS_READY == 0U){}
}

void fr_disable_ble()
{
	if(NRF_RADIO->STATE == 0) return; //already disabled
	
    NRF_RADIO->SHORTS          = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
//    NRF_RADIO->TEST            = 0;
    NRF_RADIO->TASKS_DISABLE   = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0) ;
    NRF_RADIO->EVENTS_DISABLED = 0;
}

int ble_mode = 0;

void send_adv_packet()
{
	cur_channel++;
	if(cur_channel > 39) cur_channel = 37;
	if(cur_channel == 37)
		NRF_RADIO->FREQUENCY = 2;// << RADIO_FREQUENCY_FREQUENCY_Pos; //ch37
	if(cur_channel == 38)
		NRF_RADIO->FREQUENCY = 26;// << RADIO_FREQUENCY_FREQUENCY_Pos; //ch38
	if(cur_channel == 39)
		NRF_RADIO->FREQUENCY = 80;// << RADIO_FREQUENCY_FREQUENCY_Pos; //ch39

	NRF_RADIO->DATAWHITEIV = cur_channel;

	// Configure address of the packet and logic address to use
	NRF_RADIO->PACKETPTR = (uint32_t)packet;//&packet[0];
  
//	prepare_transmission();
	//nrf_gpio_pin_set(LED_RED);		

	if(!(NRF_RADIO->STATE & 0b01000)) fr_disable_ble(); //non-tx state
	else if(!(NRF_RADIO->STATE & 0b011)) fr_disable_ble(); //states 9, 10, 11 responsible for TX

	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
	NRF_RADIO->TASKS_TXEN = 1;
//	for(int x = 0; x < 20; x++) ; //to make sure radio enters TXEN sequence before enabling DISABLED_RXEN shortcut
//	NRF_RADIO->SHORTS |= RADIO_SHORTS_DISABLED_RXEN_Msk;
	ble_mode = 3;
	return;
}

uint32_t irq_cnt2 = 0;
uint32_t rx_packet_counter2 = 0;
uint32_t last_end_event_time2 = 0;

uint8_t rx_packet2[64];

void ble_irq_handler()
{
//			NRF_GPIO->OUTSET = (1<<28);
	irq_cnt2++;
	if(NRF_RADIO->EVENTS_END && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk))
	{
		NRF_RADIO->EVENTS_END = 0;
		last_end_event_time2 = millis();
		if(ble_mode == 0) //got connection in RX mode
		{
			rx_packet_counter2++;
		}
		if(ble_mode == 3) //tx event ended, switched to rx using shorts
		{
			NRF_RADIO->PACKETPTR = (uint32_t)rx_packet2;
			NRF_RADIO->EVENTS_DISABLED = 0U;    
			NRF_RADIO->TASKS_DISABLE   = 1U; // Disable the radio - don't want RX
			ble_mode = 0;
//			uart_send(rx_packet2, 64);
		}
	}
}
