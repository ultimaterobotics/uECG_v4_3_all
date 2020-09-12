#include "mcp3911.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "fft_opt.h"

MCP3911_GAIN_REG gain_reg;
MCP3911_STATUSCOM_REG statcom_reg;
MCP3911_CONFIG_REG conf_reg;


float PI = 3.1415926; 

float sin_f(float x)
{

/*********************************************************
 * high precision sine/cosine
 *********************************************************/
	float res = 0;
	while (x < -3.14159265) x += 6.28318531;
	while(x >  3.14159265) x -= 6.28318531;

	if (x < 0)
	{
		res = 1.27323954 * x + .405284735 * x * x;

		if (res < 0)
			res = .225 * (res *-res - res) + res;
		else
			res = .225 * (res * res - res) + res;
	}
	else
	{
		res = 1.27323954 * x - 0.405284735 * x * x;

		if (res < 0)
			res = .225 * (res *-res - res) + res;
		else
			res = .225 * (res * res - res) + res;
	}
	return res;
}
float cos_f(float x)
{
	return sin_f(x + 1.57079632);
}

void init_spi(uint8_t pin_MISO, uint8_t pin_MOSI, uint8_t pin_SCK)
{
	NRF_SPI0->PSELSCK = pin_SCK;
	NRF_SPI0->PSELMOSI = pin_MOSI;
	NRF_SPI0->PSELMISO = pin_MISO;
	
	NRF_SPI0->FREQUENCY = 0x40000000; 
//	NRF_SPI0->FREQUENCY = 0x02000000; 
	/* 0x02000000 125 kbps
	 * 0x04000000 250 kbps
	 * 0x08000000 500 kbps
	 * 0x10000000 1 Mbps
	 * 0x20000000 2 Mbps
	 * 0x40000000 4 Mbps
	 * 0x80000000 8 Mbps */
	
	NRF_SPI0->CONFIG = 0b000; // 0bxx0 - SPI mode 0b00x - polarity, 0 - MSB first
	NRF_SPI0->ENABLE = 1;
}


void mcp_write_reg8(uint8_t reg, uint8_t value)
{
	uint8_t tmp;
	while(NRF_SPI0->EVENTS_READY) tmp = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	NRF_SPI0->TXD = (reg<<1);
	while(!NRF_SPI0->EVENTS_READY) ;
	tmp = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_SPI0->TXD = value;
	while(!NRF_SPI0->EVENTS_READY) ;
	tmp = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	tmp = tmp;
}

uint8_t mcp_read_reg8(uint8_t reg)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	NRF_SPI0->TXD = (reg<<1) | 1;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;
	NRF_SPI0->TXD = 0;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	return res;
}

void mcp_read_buf(uint8_t addr, uint8_t length, uint8_t *result)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	NRF_SPI0->TXD = (addr<<1) | 1;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	for(int x = 0; x < length; x++)
	{
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		result[x] = NRF_SPI0->RXD;
		NRF_SPI0->EVENTS_READY = 0;
	}
	NRF_GPIO->OUTSET = mcp.CS;
	res = res;
}

void mcp_write_buf(uint8_t addr, uint8_t length, uint8_t *data)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	NRF_SPI0->TXD = (addr<<1);
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	for(int x = 0; x < length; x++)
	{
		NRF_SPI0->TXD = data[x];
		while(!NRF_SPI0->EVENTS_READY) ;
		res = NRF_SPI0->RXD;
		NRF_SPI0->EVENTS_READY = 0;
	}
	NRF_GPIO->OUTSET = mcp.CS;
	res = res;
}

void mcp3911_write_reg16(uint8_t reg, uint16_t value)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	uint8_t treg = ((0b00)<<6) | (reg<<1) | 0;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	uint8_t shift = 8;
	for(int x = 0; x < 2; x++)
	{
		NRF_SPI0->TXD = (value>>shift)&0xFF;
		shift -= 8;
		while(!NRF_SPI0->EVENTS_READY) ;
		res = NRF_SPI0->RXD;
		NRF_SPI0->EVENTS_READY = 0;
	}
	NRF_GPIO->OUTSET = mcp.CS;
	res = res;
}

void mcp3911_write_reg8(uint8_t reg, uint8_t value)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	uint8_t treg = ((0b00)<<6) | (reg<<1) | 0;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_SPI0->TXD = value;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	res = res;
}

uint8_t mcp3911_read_reg8(uint8_t reg)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b00)<<6) | (reg<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;
	NRF_SPI0->TXD = 0;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	return res;
}

uint32_t mcp3911_read_reg24(uint8_t reg)
{
	uint32_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b00)<<6) | (reg<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	res = 0;
	for(int x = 0; x < 3; x++)
	{
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		res = res<<8;
		res |= NRF_SPI0->RXD;
	}
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	return res;
}

int16_t mcp3911_read_reg16(uint8_t reg)
{
	int16_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b00)<<6) | (reg<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	res = 0;
	for(int x = 0; x < 2; x++)
	{
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		res = res<<8;
		res |= NRF_SPI0->RXD;
	}
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	return res;
}

void mcp3911_update_config()
{
	gain_reg.PGA_CH0 = mcp.gain;
	gain_reg.PGA_CH1 = mcp.gain;
	
	conf_reg.OSR = mcp.osr;
	conf_reg.PRE = mcp.amclk_prescaler;
	
	mcp3911_write_reg8(MCP3911_GAIN, gain_reg.reg);
	mcp3911_write_reg16(MCP3911_CONFIG, conf_reg.reg);
}

void start_mcp_clock()
{
	NRF_TIMER1->MODE = 0;
	NRF_TIMER1->BITMODE = 1;
	NRF_TIMER1->PRESCALER = 0b0;
	NRF_TIMER1->CC[0] = 0b01;
	NRF_TIMER1->CC[1] = 0b01;
	NRF_TIMER1->SHORTS = 0b10; //clear on compare 1

	NRF_GPIOTE->CONFIG[1] = (1<<20) | (0b11 << 16) | (16 << 8) | 0b11; //toggle PIN_NUM<<8 (0) TASK
//	NRF_GPIOTE->CONFIG[1] = (1<<20) | (0b11 << 16) | (9 << 8) | 0b11; //toggle PIN_NUM<<8 (0) TASK

	NRF_PPI->CHENSET = 1<<5;
	NRF_PPI->CHG[0] |= 1<<5;
	NRF_PPI->CH[5].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[0];
	NRF_PPI->CH[5].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];

	NRF_TIMER1->TASKS_START = 1;
}

void stop_mcp_clock()
{
	NRF_TIMER1->TASKS_STOP = 1;
}

volatile uint8_t mcp_has_new_data = 0;
uint32_t skin_pin_mask = 0;
uint8_t skin_measuring = 1;

int filtered_value = 0;
float filtered_skin = 0;
int filter_depth = 8;
int filter_buf[64];
int filter_temp[64];
int filter_pos = 0;
uint8_t filter_mode = 1;


float sin50_w = 0;
float cos50_w = 0;
float sin60_w = 0;
float cos60_w = 0;

float sin50_w2 = 0;
float cos50_w2 = 0;
float sin60_w2 = 0;
float cos60_w2 = 0;

float tt = 0;
float tt60 = 0;
float avg_m = 0.997;
float av = 0;

int last_cv1 = 0, last_cv2 = 0;

int use_fft = 0;
int fft_size = 64;
float fft_buf1_r[128];
float fft_buf1_i[128];
float fft_buf2_r[128];
float fft_buf2_i[128];
float fft_spectr[128];
volatile float *fft_buf_r = fft_buf1_r;
volatile float *fft_buf_i = fft_buf1_i;
volatile float *fft_stored_r = fft_buf2_r;
volatile float *fft_stored_i = fft_buf2_i;
volatile int cur_fft_buf = 0;
volatile int cur_fft_pos = 0;
volatile int fft_data_ready = 0;

float *get_cur_fft_buf(int real_im) //0 real 1 im
{
	if(cur_fft_buf == 0)
	{
		if(real_im == 0) return fft_buf1_r;
		else return fft_buf1_i;
	} 
	else
	{
		if(real_im == 0) return fft_buf2_r;
		else return fft_buf2_i;
	}
}

int push_mcp_filter(int cv, int cv2)
{
	if(filter_mode == 0)
	{
		filtered_value = cv;
		return 1;
	}
	if(filter_mode == 1)
	{
		if(skin_measuring)
			;
//			cv = last_cv1;
		else
			last_cv1 = cv;
		float vv = cv;
		float vv2 = cv2;
		float M_PI = 3.1415926;
		sin50_w *= avg_m;
		cos50_w *= avg_m;
		float sf50 = sin_f(2*M_PI*tt);
		float cf50 = cos_f(2*M_PI*tt);
		sin50_w += vv * sf50 * (1.0 - avg_m);
		cos50_w += vv * cf50 * (1.0 - avg_m);

		sin50_w2 *= avg_m;
		cos50_w2 *= avg_m;
		sin50_w2 += vv2 * sf50 * (1.0 - avg_m);
		cos50_w2 += vv2 * cf50 * (1.0 - avg_m);


		float sf60 = sin_f(2*M_PI*tt60);
		float cf60 = cos_f(2*M_PI*tt60);
		sin60_w *= avg_m;
		cos60_w *= avg_m;
		sin60_w += vv * sf60 * (1.0 - avg_m);
		cos60_w += vv * cf60 * (1.0 - avg_m);

		sin60_w2 *= avg_m;
		cos60_w2 *= avg_m;
		sin60_w2 += vv2 * sf60 * (1.0 - avg_m);
		cos60_w2 += vv2 * cf60 * (1.0 - avg_m);
		
		float filt_v;
		float filt_v2;
		if(sin50_w*sin50_w + cos50_w*cos50_w > sin60_w*sin60_w + cos60_w*cos60_w)
		{
			filt_v = vv - 2.0*(sin50_w * sf50 + cos50_w * cf50);
			filt_v2 = vv2 - 2.0*(sin50_w2 * sf50 + cos50_w * cf50);			
		}
		else
		{
			filt_v = vv - 2.0*(sin60_w * sf60 + cos60_w * cf60);
			filt_v2 = vv2 - 2.0*(sin60_w2 * sf60 + cos60_w2 * cf60);
		}
		tt += 0.0512295;
		tt60 += 0.06147541;
		if(tt > 1) tt -= 2;
		if(tt60 > 1) tt60 -= 2;
		filtered_value = filt_v;
		if(skin_measuring)
		{
			filtered_skin *= 0.9;
			filtered_skin += 0.1*vv2;//filt_v2;
		}
		if(use_fft)
		{
			fft_buf_r[cur_fft_pos] = filtered_value;
			fft_buf_i[cur_fft_pos] = 0;
			cur_fft_pos++;
			if(cur_fft_pos >= fft_size)
			{
				cur_fft_pos = 0;
				fft_data_ready = 1;
				if(cur_fft_buf == 0)
				{
					cur_fft_buf = 1;
					fft_buf_r = fft_buf2_r;
					fft_buf_i = fft_buf2_i;
					fft_stored_r = fft_buf1_r;
					fft_stored_i = fft_buf1_i;
				}
				else
				{
					cur_fft_buf = 0;
					fft_buf_r = fft_buf1_r;
					fft_buf_i = fft_buf1_i;
					fft_stored_r = fft_buf2_r;
					fft_stored_i = fft_buf2_i;
				}
			}
		}
		return 1;
	}
	filter_buf[filter_pos++] = cv;
	if(filter_pos >= filter_depth)
	{
		for(int i1 = 0; i1 < filter_depth; i1++)
			filter_temp[i1] = filter_buf[i1];
		for(int i1 = 0; i1 < filter_depth; i1++)
			for(int i2 = i1+1; i2 < filter_depth; i2++)
			{
				if(filter_temp[i2] > filter_temp[i1])
				{
					int tt = filter_temp[i1];
					filter_temp[i1] = filter_temp[i2];
					filter_temp[i2] = tt;
				}
			}
		filtered_value = (filter_temp[filter_depth/2 - 1] + filter_temp[filter_depth/2])/2;
		filter_pos = 0;
		return 1;
	}
	return 0;
}

float sqrt_fast(float x)
{
	if(x < 0) return -1;
	if(x == 0) return 0;
	float aa = x;
	float bb = 1;
	float mult = 1;
	int m_iters = 0;
	while(aa > 100 && m_iters++ < 20)
	{
		aa *= 0.01;
		mult *= 10;
	}
	while(aa < 0.01 && m_iters++ < 20)
	{
		aa *= 100;
		mult *= 0.1;
	}
	for(int nn = 0; nn < 6; ++nn)
	{
		float avg = 0.5*(aa+bb);
		bb *= aa/avg;
		aa = avg;
	}  
	return mult*0.5*(aa+bb);
}


#define FPU_EXCEPTION_MASK               0x0000009F                      //!< FPU exception mask used to clear exceptions in FPSCR register.
#define FPU_FPSCR_REG_STACK_OFF          0x40                            //!< Offset of FPSCR register stacked during interrupt handling in FPU part stack.

void FPU_IRQHandler(void)
{
    // Prepare pointer to stack address with pushed FPSCR register.
    uint32_t * fpscr = (uint32_t * )(FPU->FPCAR + FPU_FPSCR_REG_STACK_OFF);
    // Execute FPU instruction to activate lazy stacking.
    (void)__get_FPSCR();
    // Clear flags in stacked FPSCR register.
    *fpscr = *fpscr & ~(FPU_EXCEPTION_MASK);
}

int mcp_fft_process()
{
	if(!fft_data_ready) return 0;
	fft_data_ready = 0;
	fft_radix8_butterfly_64((float*)fft_stored_r, (float*)fft_stored_i);
	for(int x = 0; x < 32; x++)
	{
		float vv = fft_stored_r[x]*fft_stored_r[x] + fft_stored_i[x]*fft_stored_i[x];
		fft_spectr[x] = sqrt_fast(vv);
	}
	return 1;
}
float *mcp_fft_get_spectr()
{
	return fft_spectr;
}


int mcp_get_filtered_value()
{
	return filtered_value;
}

int mcp_get_filtered_skin()
{
	int res = filtered_skin;
	return res;
}

void mcp_fft_mode(int onoff)
{
    NVIC_SetPriority(FPU_IRQn, 5);
    NVIC_ClearPendingIRQ(FPU_IRQn);
    NVIC_EnableIRQ(FPU_IRQn);	
	use_fft = onoff;
}

void mcp_set_filter_mode(int use_filter)
{
	filter_mode = use_filter;
}


void GPIOTE_IRQHandler(void)
{
	if (NRF_GPIOTE->EVENTS_IN[0] == 1)
	{
		NRF_GPIOTE->EVENTS_IN[0] = 0;
		mcp.cvalue[0] = mcp3911_read_reg16(MCP3911_CH0);
		mcp.cvalue[1] = mcp3911_read_reg16(MCP3911_CH1);
		if(push_mcp_filter(mcp.cvalue[0], mcp.cvalue[1]))
			mcp_has_new_data = 1;
	}
}

int init_ok = 0;

void init_mcp3911(uint8_t pin_MISO, uint8_t pin_MOSI, uint8_t pin_SCK, uint8_t pin_CS, uint8_t pin_INT)
{
/*	nrf_gpio_cfg_output(pin_MOSI);
	nrf_gpio_cfg_output(pin_SCK);
	nrf_gpio_cfg_input(pin_MISO, NRF_GPIO_PIN_NOPULL);*/
	nrf_gpio_cfg_output(pin_CS);
//	nrf_gpio_cfg_input(pin_INT, NRF_GPIO_PIN_PULLUP);
	
	NRF_GPIOTE->CONFIG[0] = (0b10 << 16) | (pin_INT << 8) | 0b01; //HiToLow<<16 | PIN_NUM<<8 | EVENT
	NRF_GPIOTE->INTENSET = 0b001; //interrupt channel 0
	NVIC_EnableIRQ(GPIOTE_IRQn);
		
	mcp.CS = 1<<pin_CS;
	NRF_GPIO->OUTSET = mcp.CS;

	init_spi(pin_MISO, pin_MOSI, pin_SCK);
	
	mcp.gain = 0b101;
	mcp.osr = 0b101;
	mcp.amclk_prescaler = 0b01;
	
	gain_reg.PGA_CH0 = mcp.gain;
	gain_reg.PGA_CH1 = 0;//mcp.gain;
	gain_reg.BOOST = 0b01; //lower power

	conf_reg.DITHER = 0b00;
//	conf_reg.EN_GAINCAL = 0;
//	conf_reg.EN_OFFCAL = 0;
	conf_reg.OSR = mcp.osr;
	conf_reg.PRE = mcp.amclk_prescaler; // 0b11 = 8, 0b10 = 4, 0b01 = 2, 0 = 1
	conf_reg.SHUTDOWN = 0;
	conf_reg.VREFEXT = 0;

//  statcom_reg.WIDTH_DATA = 0b10; //32 bits
//	statcom_reg.WIDTH = 0b11; //24 bits
	statcom_reg.WIDTH = 0b00; //16 bits
//	statcom_reg.WIDTH_DATA = 0b01; //24 bits
	statcom_reg.DR_HIZ = 1;
//	statcom_reg.DR_LINK = 1;
//	statcom_reg.EN_CRCCOM = 0;
//	statcom_reg.EN_INT = 0;
	statcom_reg.READ = 0b10;
//	statcom_reg.WIDTH_CRC = 0;
	statcom_reg.WRITE = 1;


	mcp3911_write_reg8(MCP3911_GAIN, gain_reg.reg);	
	mcp3911_write_reg16(MCP3911_STATCOM, statcom_reg.reg);	
	mcp3911_write_reg16(MCP3911_CONFIG, conf_reg.reg);
	
	uint16_t conf_reg_val = mcp3911_read_reg16(MCP3911_CONFIG);
	if(conf_reg_val == conf_reg.reg)
	{
		init_ok = 1;
	}
	start_mcp_clock();
	
	return;
}

void get_mcp_data_test()
{
	int res = 0;
	mcp.cvalue[0] = mcp3911_read_reg24(MCP3911_CH0);
	mcp.cvalue[0] >>= 8;
	mcp.cvalue[1] = mcp3911_read_reg24(MCP3911_CH1);
	mcp.cvalue[1] >>= 8;
	return;
//	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b01)<<6) | (MCP3911_CH0<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	for(int ch = 0; ch < 2; ch++)
	{
		for(int x = 0; x < 3; x++)
		{
			NRF_SPI0->EVENTS_READY = 0;
			NRF_SPI0->TXD = 0;
			while(!NRF_SPI0->EVENTS_READY) ;
			res = res<<8;
			res |= NRF_SPI0->RXD;
		}
/*		int16_t rr;
		if(res < 0)
		{
			rr = (-res)>>8;
			rr = -rr;
		}
		else
			rr = (res)>>8;*/
		mcp.cvalue[ch] = res;
	}
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
//	return res;
}
void get_mcp_data()
{
	get_mcp_data_test();
	mcp_has_new_data = 0;
}
uint32_t last_ms = 0;
uint8_t mcp_data_ready()
{
/*	uint32_t ms = millis();
	if(ms - last_ms > 1)
	{
		last_ms = ms;
		return 1;
	}*/
//	return mcp_has_new_data;
	uint32_t st = mcp3911_read_reg16(MCP3911_STATCOM);
	uint8_t str = (st>>8)&0b11;
	return (str == 0);	
}

uint8_t mcp3911_read()
{
	if(mcp_has_new_data)
	{
		mcp_has_new_data = 0;
		return 1;
	}
	return 0;
//	if(!mcp_data_ready()) return 0;
//	get_mcp_data();
//	return 1;
}


int mcp3911_is_ok()
{
	return init_ok;
}

void mcp3911_turnoff()
{
	NVIC_DisableIRQ(GPIOTE_IRQn);

	gain_reg.PGA_CH0 = 0;
	gain_reg.PGA_CH1 = 0;
	gain_reg.BOOST = 0;
	mcp3911_write_reg8(MCP3911_GAIN, gain_reg.reg);	
	
	conf_reg.reg = 0;
	conf_reg.SHUTDOWN = 0b11;
	conf_reg.VREFEXT = 1;
	conf_reg.CLKEXT = 1;
	mcp3911_write_reg16(MCP3911_CONFIG, conf_reg.reg);
	stop_mcp_clock();
}