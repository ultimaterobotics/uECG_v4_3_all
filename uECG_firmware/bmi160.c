#include "bmi160.h"
#include "leds.h"
#include "timer_core.h"

#define BMI_ERR_REG			0x02
#define BMI_PMU_STATUS		0x03
#define BMI_DATA_MAG		0x04
#define BMI_DATA_GYRO		0x0C
#define BMI_DATA_ACC		0x12
#define BMI_SENSORTIME		0x18
#define BMI_STATUS			0x1B
#define BMI_INT_STATUS		0x1C
#define BMI_TEMPERATURE		0x20
#define BMI_FIFO_LENGTH		0x22
#define BMI_FIFO_DATA		0x24
#define BMI_ACC_CONF		0x40
#define BMI_ACC_RANGE		0x41
#define BMI_GYR_CONF		0x42
#define BMI_GYR_RANGE		0x43
#define BMI_MAG_CONF		0x44
#define BMI_FIFO_DOWNS		0x45
#define BMI_FIFO_CONF		0x46
#define BMI_INT_EN			0x50
#define BMI_INT_OUT_CTRL	0x53
#define BMI_INT_LATCH		0x54
#define BMI_INT_MAP			0x55
#define BMI_INT_DATA		0x58

#define BMI_INT_TAP			0x63
//...
#define BMI_STEP_CNT		0x78
#define BMI_STEP_CONF		0x7A
#define BMI_CMD				0x7E

void bmi_write_reg8(uint8_t reg, uint8_t value)
{
	uint8_t tmp;
	while(NRF_SPI0->EVENTS_READY) tmp = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = bmi.CS;

	NRF_SPI0->TXD = reg;
	while(!NRF_SPI0->EVENTS_READY) ;
	tmp = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_SPI0->TXD = value;
	while(!NRF_SPI0->EVENTS_READY) ;
	tmp = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = bmi.CS;
	tmp = tmp;
}

uint8_t bmi_read_reg8(uint8_t reg)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = bmi.CS;

	NRF_SPI0->TXD = reg | (1<<7);
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;
	NRF_SPI0->TXD = 0;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = bmi.CS;
	return res;
}

void bmi_read_buf(uint8_t addr, uint8_t length, uint8_t *result)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = bmi.CS;

	NRF_SPI0->TXD = addr | (1<<7);
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
	NRF_GPIO->OUTSET = bmi.CS;
	res = res;
}

void bmi_read_fifo(int16_t *ax, int16_t *ay, int16_t *az, int len)
{
	uint8_t tbuf[12];

	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = bmi.CS;

	NRF_SPI0->TXD = BMI_FIFO_DATA | (1<<7);
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	for(int x = 0; x < len; x++)
	{
		for(int v = 0; v < 6; v++)
		{
			NRF_SPI0->TXD = 0;
			while(!NRF_SPI0->EVENTS_READY) ;
			tbuf[v] = NRF_SPI0->RXD;
			NRF_SPI0->EVENTS_READY = 0;
		}
		ax[x] = (tbuf[1]<<8)|tbuf[0];
		ay[x] = (tbuf[3]<<8)|tbuf[2];
		az[x] = (tbuf[5]<<8)|tbuf[4];
	}
	NRF_GPIO->OUTSET = bmi.CS;
	res = res;
}


//uint8_t raw_data[12];
uint8_t stat;

uint8_t bmi160_get_status()
{
	return bmi_read_reg8(0x00);
}

uint8_t bmi160_read()
{
	NVIC_DisableIRQ(GPIOTE_IRQn);
	uint8_t status = bmi_read_reg8(BMI_STATUS);
	NVIC_EnableIRQ(GPIOTE_IRQn);
	stat = status;
	if((status & 0b11000000) == 0) return 0;
	NVIC_DisableIRQ(GPIOTE_IRQn);
	bmi_read_buf(BMI_DATA_GYRO, 12, bmi.raw_data);
	NVIC_EnableIRQ(GPIOTE_IRQn);
	int16_t v;
	uint8_t *pbuf = bmi.raw_data;
	v = *pbuf++; v += (*pbuf++) << 8;
	bmi.rwX = v;
	bmi.wX = bmi.rawG2SI * (float)v;
	v = *pbuf++; v += (*pbuf++) << 8;
	bmi.rwY = v;
	bmi.wY = bmi.rawG2SI * (float)v;
	v = *pbuf++; v += (*pbuf++) << 8;
	bmi.rwZ = v;
	bmi.wZ = bmi.rawG2SI * (float)v;
	v = *pbuf++; v += (*pbuf++) << 8;
	bmi.raX = v;
	bmi.aX = bmi.rawA2SI * (float)v;
	v = *pbuf++; v += (*pbuf++) << 8;
	bmi.raY = v;
	bmi.aY = bmi.rawA2SI * (float)v;
	v = *pbuf++; v += (*pbuf++) << 8;
	bmi.raZ = v;
	bmi.aZ = bmi.rawA2SI * (float)v;

	bmi.data_id++;
	return 1;
}

void bmi160_read_steps()
{
	uint16_t step_cnt;
	bmi_read_buf(BMI_STEP_CNT, 2, &step_cnt);
	bmi.step_cnt = step_cnt;
}

float bmi160_read_temp()
{
	int16_t temp_val;
	bmi_read_buf(BMI_TEMPERATURE, 2, &temp_val);
	bmi.T = 23 + (float)temp_val / 512.0;
	return bmi.T;
}

void bmi160_stop()
{
//	lsm_write_reg8(LSM_CTRL1_XL, 0b0); //disabled
//	lsm_write_reg8(LSM_CTRL2_G, 0b0); //disabled

	bmi_write_reg8(BMI_CMD, 0b00010000); //set acc power mode - suspend
	delay_ms(100);
	bmi_write_reg8(BMI_CMD, 0b00010100); //set gyro power mode - suspend
	delay_ms(100);

	NRF_SPI0->ENABLE = 0;
}

void bmi160_resume()
{
	NRF_SPI0->ENABLE = 1;
	delay_ms(1);
	bmi_write_reg8(BMI_CMD, 0b00010001); //set acc power mode - normal 01
	delay_ms(100);
	bmi_write_reg8(BMI_CMD, 0b00010101); //set gyro power mode - normal 01
	delay_ms(100);
}

void bmi160_normal_mode()
{
	bmi_write_reg8(BMI_CMD, 0xB6); //reset
	delay_ms(100);
	bmi_write_reg8(BMI_CMD, 0b00010001); //set acc power mode - normal 01
	delay_ms(100);
	bmi_write_reg8(BMI_CMD, 0b00010101); //set gyro power mode - normal 01
	delay_ms(100);

	bmi_write_reg8(BMI_FIFO_DOWNS, 0);
	bmi_write_reg8(BMI_FIFO_CONF+1, 0); //turn off

	bmi_write_reg8(BMI_ACC_CONF, 0b10100111); //010 bwp, 50 Hz, undersampling enabled
	delay_ms(1);
	bmi_write_reg8(BMI_ACC_RANGE, 0b00000101); //+-4g
	delay_ms(1);
	bmi_write_reg8(BMI_GYR_CONF, 0b00100111); //10 bwp, 50 Hz
	delay_ms(1);
	bmi_write_reg8(BMI_GYR_RANGE, 0b00000010); //+-500 dps
	delay_ms(1);
	bmi_write_reg8(BMI_STEP_CONF, 0b00010101); //steps
	delay_ms(1);
	bmi_write_reg8(BMI_STEP_CONF+1, 0b00001011); //steps
	delay_ms(1);

	bmi_write_reg8(BMI_INT_EN+1, 1<<4); //drdy interrupt
	bmi_write_reg8(BMI_INT_OUT_CTRL, 0b00001010); //int 1 enabled, active high
	bmi_write_reg8(BMI_INT_MAP+1, 1<<7); //drdy on int1
//	bmi_write_reg8(BMI_INT_LATCH, 0b1000); //40 ms latch
	bmi_write_reg8(BMI_INT_LATCH, 0); //no latch
}


void bmi160_lp_mode()
{
	bmi_write_reg8(BMI_CMD, 0b00010000); //set acc power mode - suspend
	delay_ms(10);
	bmi_write_reg8(BMI_CMD, 0b00010100); //set gyro power mode - suspend
	delay_ms(10);

	bmi_write_reg8(BMI_CMD, 0b00010010); //set acc power mode - low
	delay_ms(10);

	bmi_write_reg8(BMI_FIFO_DOWNS , 0b01010000); //pre-filtered acc data at 50hz
	
	bmi_write_reg8(BMI_FIFO_CONF+1, 0b01000000); //only acc data

}

void bmi160_acc_mode()
{
	bmi_write_reg8(BMI_CMD, 0xB6); //reset
	delay_ms(100);
	bmi_write_reg8(BMI_CMD, 0b00010001); //set acc power mode - normal 01
	delay_ms(100);
	bmi_write_reg8(BMI_CMD, 0b00010100); //set gyro power mode - off 00
	delay_ms(100);
	bmi_write_reg8(BMI_CMD, 0b00010001); //set acc power mode - normal 01
	delay_ms(100);

	bmi_write_reg8(BMI_FIFO_DOWNS, 0);
	bmi_write_reg8(BMI_FIFO_CONF+1, 0); //turn off

	bmi_write_reg8(BMI_ACC_CONF, 0b10100111); //010 bwp, 50 Hz, undersampling enabled
	delay_ms(1);
	bmi_write_reg8(BMI_ACC_RANGE, 0b00000101); //+-4g
	bmi.rawA2SI = 9.8 * 4.0 / 32768.0;// 4g mode
	delay_ms(1);
	bmi_write_reg8(BMI_GYR_CONF, 0b00100111); //10 bwp, 50 Hz
	delay_ms(1);
	bmi_write_reg8(BMI_GYR_RANGE, 0b00000010); //+-500 dps
	delay_ms(1);
	bmi_write_reg8(BMI_INT_EN+1, 1<<4); //drdy interrupt
	bmi_write_reg8(BMI_INT_OUT_CTRL, 0b00001010); //int 1 enabled, active high
	bmi_write_reg8(BMI_INT_MAP+1, 1<<7); //drdy on int1
//	bmi_write_reg8(BMI_INT_LATCH, 0b1000); //40 ms latch
	bmi_write_reg8(BMI_INT_LATCH, 0); //no latch

	bmi_write_reg8(BMI_STEP_CONF, 0b00010101); //steps
	bmi_write_reg8(BMI_STEP_CONF+1, 0b00001011); //steps
	
}

void bmi160_init(uint8_t pin_MISO, uint8_t pin_MOSI, uint8_t pin_SCK, uint8_t pin_CS, uint8_t pin_INT)
{
	nrf_gpio_cfg_output(pin_MOSI);
	nrf_gpio_cfg_output(pin_SCK);
	nrf_gpio_cfg_output(pin_CS);
	nrf_gpio_cfg_input(pin_MISO, NRF_GPIO_PIN_NOPULL);
	bmi.CS = 1<<pin_CS;
	NRF_GPIO->OUTSET = bmi.CS;
	bmi.data_id = 0;

	delay_ms(100);
	
	bmi.rawA2SI = 9.8 * 4.0 / 32768.0;// 4g mode
	bmi.rawG2SI = 0.000266048;// 500 dps mode
	
//	init_spi(pin_MISO, pin_MOSI, pin_SCK);
	
	bmi160_acc_mode();
}

int bmi160_is_ok()
{
	uint8_t who_am_i = bmi_read_reg8(0);
	if(who_am_i == 0b11010001)
		return 1;
	return 0;
}