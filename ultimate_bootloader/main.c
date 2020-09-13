#include "nrf.h"
#include "fast_radio.h"
#include "timer_core.h"
#include "uart_interface.h"
#include <stdio.h>
//#include <stdarg.h>

#define BOOTLOADER_AREA_SIZE 0x4000
#define FLASH_CACHE_AREA 0x40000

/*
void uprintf( const char * format, ... )
//void uprint(char *txt)
{
	va_list args;

	va_start(args, format);
	int ulen = vsnprintf(umsg, 255, format, args);
	va_end(args);
	uart_send(umsg, ulen);
}*/

/*

openocd -f interface/stlink-v2.cfg  -f target/nrf52.cfg

telnet 127.0.0.1 4444

halt                                     
nrf5 mass_erase                         
flash write_image erase "_build/nrf52832_xxaa.hex"
reset

debug:

$ arm-none-eabi-gdb nrf52832_xxaa.out
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) step

*/

static inline void start_code_fix()
{
    __asm volatile(
			"mov   lr, #0x3331\t\n"
			"MOV   R5, #0x3331\t\n"
			"BX    R5\t\n"
    );
}

static inline void start_code_test(uint32_t start_addr, uint32_t *ret)
{
	uint32_t saddr = start_addr;
	uint32_t raddr;
    __asm volatile(
			"mov   lr, %[saddr]\t\n"
			"MOV   R5, %[saddr]\t\n"
			"MOV   %[raddr], R5\t\n"
			: [raddr] "=r" (raddr)
			: [saddr] "r" (saddr)
    );
	*ret = raddr;
}

static inline void start_code2(uint32_t start_addr)
{
	uint32_t saddr = start_addr;
    __asm volatile(
			"mov   lr, %[saddr]\t\n"
			"MOV   R5, %[saddr]\t\n"
			"BX    R5\t\n"
			:
			: [saddr] "r" (saddr)
    );
}

static inline void interrupts_disable(void)
{
    uint32_t interrupt_setting_mask;
    uint32_t irq = 0; // We start from first interrupt, i.e. interrupt 0.

    // Fetch the current interrupt settings.
    interrupt_setting_mask = NVIC->ISER[0];

    for (; irq < 32; irq++)
    {
        if (interrupt_setting_mask & (0x01 << irq))
        {
            // The interrupt was enabled, and hence disable it.
            NVIC_DisableIRQ((IRQn_Type)irq);
        }
    }
}

int binary_exec(uint32_t address)
{
    int i;
    __disable_irq();
// Disable IRQs
	for (i = 0; i < 8; i ++) NVIC->ICER[i] = 0xFFFFFFFF;
// Clear pending IRQs
	for (i = 0; i < 8; i ++) NVIC->ICPR[i] = 0xFFFFFFFF;

// -- Modify vector table location
// Barriars
	__DSB();
	__ISB();
// Change the vector table
//	uint32_t adr = address>>1;
//	adr = adr<<1;
	SCB->VTOR = (BOOTLOADER_AREA_SIZE & 0x1ffff80);
//	SCB->VTOR = (0x20001000 & 0x1ffff80);
// Barriars 
	__DSB();
	__ISB();

	__enable_irq();

// -- Load Stack & PC
//	start_code_fix();
	uint32_t tst;
	start_code_test(address, &tst);
//	uprintf("asm code test: addr %d res %d\n", address, tst);
//	delay_ms(200);
	start_code2(address);
//	binexec(address);
	return 0;
}

#define FLASH_READFLAG 0x00
#define FLASH_WRITEFLAG 0x01
#define FLASH_ERASEFLAG 0x02


void erase_page(uint32_t page)
{
    // Turn on flash write
    while (!NRF_NVMC->READY) ;
    NRF_NVMC->CONFIG = FLASH_ERASEFLAG;
    while (!NRF_NVMC->READY) ;

    // Erase page.
    NRF_NVMC->ERASEPAGE = page;
    while (!NRF_NVMC->READY) ;

    NRF_NVMC->CONFIG = FLASH_READFLAG;
    while (!NRF_NVMC->READY) ;
}

void write_word(uint32_t address, uint32_t value)
{
    // Enable write.
    while (!NRF_NVMC->READY) ;
	NRF_NVMC->CONFIG = FLASH_WRITEFLAG; 
    while (!NRF_NVMC->READY) ;

    *(uint32_t*)address = value;
    while (!NRF_NVMC->READY) ;

	NRF_NVMC->CONFIG = FLASH_READFLAG; 
    while (!NRF_NVMC->READY) ;
}


uint8_t uart_in_packet[300];
uint8_t pack_resp[16];
uint8_t radio_resp[16];

uint8_t last_uart_pos = 0;
uint32_t uart_buf_length;

int pack_length = 0;
int pack_state = 0;
int pack_begin = 0;

int code_uploading = 0;

uint8_t upload_start_code[8] = {0x10, 0xFC, 0xA3, 0x05, 0xC0, 0xDE, 0x11, 0x77};

uint8_t pack_prefix[2] = {223, 98};

uint8_t last_pack_id = 0;
int remaining_bytes = 0;
uint32_t written_words = 0;

enum
{
	err_code_ok = 11,
	err_code_toolong = 100,
	err_code_wrongcheck,
	err_code_wronglen,
	err_code_packmiss
};

void parse_in_packet(int len, uint8_t *pack)
{
//	uprintf(" lastpos %d upos %d len %d\n", last_uart_pos, get_rx_position(), len);
	if(len != 36)
	{
		pack_resp[0] = pack_prefix[0];
		pack_resp[1] = pack_prefix[1];
		pack_resp[2] = last_pack_id;
		pack_resp[3] = err_code_wronglen;
		uart_send(pack_resp, 4);
		return; //accept only fixed length packets
	}
	if(code_uploading == 0)
	{
		int ppos = 0;
		last_pack_id = pack[ppos++];
		for(int n = 0; n < 8; n++)
		{
			if(upload_start_code[n] != pack[ppos++])
			{
				uprintf("upload start code not found!\n");
				return; //no start code - ignore
			}
		}
		remaining_bytes = pack[ppos++];
		remaining_bytes <<= 8;
		remaining_bytes += pack[ppos++];
		remaining_bytes <<= 8;
		remaining_bytes += pack[ppos++];
		remaining_bytes <<= 8;
		remaining_bytes += pack[ppos++];
		
		uint8_t check_odd = 0;
		uint8_t check_tri = 0;
		uint8_t check_all = 0;
		for(int x = 0; x < 33; x++)
		{
			if(x%2) check_odd += pack[x];
			if(x%3) check_tri += pack[x];
			check_all += pack[x];
		}
		int check_ok = 1;
		if(check_odd != pack[33]) check_ok = 0;
		if(check_tri != pack[34]) check_ok = 0;
		if(check_all != pack[35]) check_ok = 0;
				
		if(remaining_bytes >= 0x3C000) //too long
		{
			pack_resp[0] = pack_prefix[0];
			pack_resp[1] = pack_prefix[1];
			pack_resp[2] = last_pack_id;
			pack_resp[3] = err_code_toolong;
			uart_send(pack_resp, 4);
			return;
		}
		if(!check_ok)
		{
			pack_resp[0] = pack_prefix[0];
			pack_resp[1] = pack_prefix[1];
			pack_resp[2] = last_pack_id;
			pack_resp[3] = err_code_wrongcheck;
			uart_send(pack_resp, 4);
			return;
		}
		uint32_t page_size = NRF_FICR->CODEPAGESIZE;
		uint32_t start_page = BOOTLOADER_AREA_SIZE / page_size;
		uint32_t end_page = (BOOTLOADER_AREA_SIZE + remaining_bytes) / page_size + 1;
		uprintf("page size %d, start page %d, end page %d\n", page_size, start_page, end_page);
		for(int p = start_page; p < end_page; p++)
			erase_page(p*page_size);
		
		code_uploading = 1;
		written_words = 0;
		pack_resp[0] = pack_prefix[0];
		pack_resp[1] = pack_prefix[1];
		pack_resp[2] = last_pack_id;
		pack_resp[3] = err_code_ok;
		uart_send(pack_resp, 4);
	}
	else
	{
		uint8_t check_odd = 0;
		uint8_t check_tri = 0;
		uint8_t check_all = 0;
		for(int x = 0; x < 33; x++)
		{
			if(x%2) check_odd += pack[x];
			if(x%3) check_tri += pack[x];
			check_all += pack[x];
		}
		int check_ok = 1;
		if(check_odd != pack[33]) check_ok = 0;
		if(check_tri != pack[34]) check_ok = 0;
		if(check_all != pack[35]) check_ok = 0;
		if(!check_ok)
		{
			pack_resp[0] = pack_prefix[0];
			pack_resp[1] = pack_prefix[1];
			pack_resp[2] = last_pack_id;
			pack_resp[3] = err_code_wrongcheck;
			uart_send(pack_resp, 4);
			return;
		}
		
		uint8_t exp_pack = last_pack_id+1;
		int ppos = 0;
		if(exp_pack != pack[ppos++])
		{
			pack_resp[0] = pack_prefix[0];
			pack_resp[1] = pack_prefix[1];
			pack_resp[2] = last_pack_id;
			pack_resp[3] = err_code_packmiss;
			uart_send(pack_resp, 4);
//			delay_ms(5);
			return;
		}
		last_pack_id = exp_pack;
		
		uint32_t code_words[8];
		for(int x = 0; x < 8; x++)
		{
			code_words[x] = pack[ppos++];
			code_words[x] <<= 8;
			code_words[x] += pack[ppos++];
			code_words[x] <<= 8;
			code_words[x] += pack[ppos++];
			code_words[x] <<= 8;
			code_words[x] += pack[ppos++];
		}

		for(int x = 0; x < 8; x++)
		{
			write_word(BOOTLOADER_AREA_SIZE + 4*written_words, code_words[x]);

			written_words++;
			remaining_bytes -= 4;
			
			if(remaining_bytes <= 0) break;

			if((written_words%1000) == 1)
				NRF_GPIO->OUTCLR = 1<<26;
			if((written_words%1000) == 500)
				NRF_GPIO->OUTSET = 1<<26;
			
		}

		if(remaining_bytes <= 0)
		{
			code_uploading = 0;
		}

		pack_resp[0] = pack_prefix[0];
		pack_resp[1] = pack_prefix[1];
		pack_resp[2] = last_pack_id;
		pack_resp[3] = err_code_ok;
		uart_send(pack_resp, 4);
	}
}

void check_uart_packet()
{
	uint8_t pos = last_uart_pos;
	uint8_t upos = get_rx_position();
	if(pos == upos) return;
	int unproc_length = 0;
	unproc_length = upos - pos;
	last_uart_pos = upos;
	uart_buf_length = get_rx_buf_length();
	if(unproc_length < 0) unproc_length += uart_buf_length;
	uint8_t *ubuf = get_rx_buf();
	
	for(uint8_t pp = pos; pp != upos; pp++)
	{
//		if(pp >= uart_buf_length) pp = 0;
		if(pack_state == 0 && ubuf[pp] == pack_prefix[0])
		{
			pack_state = 1;
			continue;
		}
		if(pack_state == 1)
		{
			if(ubuf[pp] == pack_prefix[1]) pack_state = 2;
			else pack_state = 0;
			continue;
		}
		if(pack_state == 2)
		{
			pack_length = ubuf[pp];
			if(pack_length >= uart_buf_length-2) //something is wrong or too long packet
			{
				pack_length = 0;
				pack_state = 0;
				continue;
			}
			pack_begin = pp+1;
			if(pack_begin >= uart_buf_length) pack_begin = 0;
			pack_state = 3;
			continue;
		}
		if(pack_state == 3)
		{
			int len = pp - pack_begin;
			if(len < 0) len += uart_buf_length;
			if(len >= pack_length)
			{
				uint8_t bg = pack_begin;
				for(int x = 0; x < pack_length; x++)
				{
					uart_in_packet[x] = ubuf[bg++];
					//if(bg >= uart_buf_length) bg = 0;
				}
				parse_in_packet(pack_length, uart_in_packet);
				pack_state = 0;
				pack_length = 0;
			}
		}
	}
}


uint32_t launch_address = 0;

void process_boot_update()//0x50fd
{
	NRF_GPIO->OUTSET = 1<<26;
	uint32_t def_addr = FLASH_CACHE_AREA + BOOTLOADER_AREA_SIZE;
	uint32_t base_addr = BOOTLOADER_AREA_SIZE;
	uint32_t code_size = 0;
	uint32_t test_size = 0;
	int done = 0;
	if(*(uint32_t*)(def_addr) == 0) done = 1; //marked as 0
	while(!done)
	{
		while(*(uint32_t*)(def_addr + code_size + test_size) == 0xFFFFFFFF)
		{
			test_size++;
			if(test_size > 100)
				break;
		}
		if(test_size < 100)
		{
			code_size += test_size;
		}
		test_size = 0;
		if(*(uint32_t*)(def_addr + code_size) == 0xFFFFFFFF) break;
		code_size++;
		if(code_size >= 0x15000) break;
	}
	uprintf("code size %d\n", code_size);
	if(code_size < 10)
	{
		launch_address = *(uint32_t*)(base_addr-4);
		uprintf("launch address read: 0x%X\n", launch_address);
		delay_ms(240);
		volatile int y;
		int x = 0;
		while(x < 10)
		{
			x++;
			NRF_GPIO->OUTCLR = 1<<26;
			y = 0;
			while(y < 500000) y++;
			NRF_GPIO->OUTSET = 1<<26;
			y = 0;
			while(y < 500000) y++;
		}
		return;
	}
	uint32_t page_size = NRF_FICR->CODEPAGESIZE;
	uint32_t start_page = base_addr / page_size;
	uint32_t end_page = (base_addr + code_size) / page_size + 1;
	uprintf("page size %d, start page %d, end page %d\n", page_size, start_page, end_page);
	for(int p = start_page; p <= end_page; p++)
	{
		if(p%2)
			NRF_GPIO->OUTSET = 1<<26;
		else
			NRF_GPIO->OUTCLR = 1<<26;
		erase_page(p*page_size);
	}
	uint32_t val0 = *(uint32_t*)def_addr;
	launch_address = *(uint32_t*)(def_addr+4);
//	launch_address += base_addr;
	uprintf("launch address calc: 0x%X\n", launch_address);
	
	write_word(base_addr-4, launch_address);
	write_word(base_addr, val0);
	write_word(def_addr, 0);
	uprintf("launch address after write: 0x%X\n", *(uint32_t*)(base_addr-4));
	NRF_GPIO->OUTCLR = 1<<26;
	for(int n = 1; n < code_size/4; n++)
	{
		uint32_t vv = *(uint32_t*)(def_addr + n*4);
		write_word(base_addr + 4*n, vv);
		if((n%200) == 1)
			NRF_GPIO->OUTCLR = 1<<26;
		if((n%200) == 100)
			NRF_GPIO->OUTSET = 1<<26;
	}
}

void SystemInit(void)
{
//    SystemCoreClock = __SYSTEM_CLOCK_64M;
}


int main() 
{ 
//	start_code_fix();
//	nrf_bootloader_init();
//	NRF_MWU->INTENCLR = 0xFFFFFFFF;
//	NRF_MWU->NMIENCLR = 0xFFFFFFFF;
//	NRF_GPIO->DIRSET = 1<<26;
//	nrf_bootloader_app_start(0x8000);
/*	int x = 0;
	volatile int y = 0;
	NRF_GPIO->OUTCLR = 1<<6;
	while(y < 3000000) y++;
	while(x++ <  4)
	{
		NRF_GPIO->OUTCLR = 1<<6;
		y = 0;
		while(y < 1000000) y++;
		NRF_GPIO->OUTSET = 1<<6;
		y = 0;
		while(y < 1000000) y++;
	}*/
//	interrupts_disable();
//	__disable_irq();  
//	SCB->VTOR = 0x8000;   
//0x32b4 - reset handler
//	((void (*)(void)) 0xB2B4)();
//	bootloader_util_reset(0x8000);
//	start_code();
//	start_code(0x8000);
//	void (*fptr)(void); 
//	fptr = (void (*)(void))0x4004; 
//	fptr();	
//	start_code(0x4000); 
//	bootloader_util_reset(0x4000);

/*
	if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk) == (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos))
	{ 
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos; 
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy){} 
		NRF_UICR->NFCPINS &= ~UICR_NFCPINS_PROTECT_Msk; 
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy){} 
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos; 
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy){} 
		NVIC_SystemReset(); 
	}*/
	
//	bootloader_util_reset(0x4000);
/*	volatile int y;
	int x = 0;
	while(x < 3)
	{
		x++;
		NRF_GPIO->OUTCLR = 1<<26;
		y = 0;
		while(y < 1000000) y++;
		NRF_GPIO->OUTSET = 1<<26;
		y = 0;
		while(y < 1000000) y++;

//		start_code_fix();
	}
//	binary_exec(0x1001);
//	SCB->VTOR = SCB->VTOR + 0x1000;
//	start_code(0x1000); 
	//bootloader_util_reset(0x1000);
//	start_code(0x1000);*/

//	start_code_fix();
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {}
	NRF_GPIO->DIRSET = 1<<26;
//	binary_exec(launch_address);
	start_time();
			
	uart_init(5, 4);
	fr_init(64);
	fr_listen();
	
	uint32_t boot_start = millis();
	int pack_st = 0;
	
	while(millis() - boot_start < 5000 || code_uploading)
	{
		check_uart_packet();
		if(fr_has_new_packet())
		{
			if(pack_st)
				NRF_GPIO->OUTCLR = 1<<26;				
			else
				NRF_GPIO->OUTSET = 1<<26;				
			pack_st = !pack_st;
			fr_get_packet(uart_in_packet);
			parse_in_packet(36, uart_in_packet);
			radio_resp[0] = 6;
			radio_resp[1]++;
			for(int x = 0; x < 4; x++)
				radio_resp[2+x] = pack_resp[x];
			fr_send_and_listen(radio_resp);
		}
	}
	
	fr_disable();
	launch_address = *(uint32_t*)(BOOTLOADER_AREA_SIZE+4);
	delay_ms(100);
	
	uprintf("boot start at 0x%X\n", launch_address);
		
//	process_fwupdate();
//	uprintf("resetting...\n");
	delay_ms(50);
	binary_exec(launch_address);
}

