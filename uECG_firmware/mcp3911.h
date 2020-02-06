#include <stdint.h>

#define MCP3911_CH0		0x00
#define MCP3911_CH1		0x03
#define MCP3911_MOD		0x06
#define MCP3911_PHASE	0x07
#define MCP3911_GAIN	0x09
#define MCP3911_STATCOM	0x0A
#define MCP3911_CONFIG	0x0C
#define MCP3911_OFF0	0x0E
#define MCP3911_GC0		0x11
#define MCP3911_OFF1	0x14
#define MCP3911_GC1		0x17
#define MCP3911_VREFCAL	0x1A

typedef struct {
    union {
        uint8_t reg;
        struct {
            unsigned PGA_CH0 : 3;
            unsigned PGA_CH1 : 3;
            unsigned BOOST : 2;
        };
    };
} MCP3911_GAIN_REG;

typedef struct {
    union {
        uint16_t reg;
        struct {
			unsigned : 1;
			unsigned CLKEXT : 1;
			unsigned VREFEXT : 1;
			unsigned : 1;
			unsigned SHUTDOWN : 2;
			unsigned RESET : 2;
			unsigned AZ_FREQ : 1;
			unsigned DITHER : 2;
			unsigned OSR : 3;
			unsigned PRE : 2;
        };
    };
} MCP3911_CONFIG_REG;
 
typedef struct {
    union {
        uint16_t reg;
        struct {
			unsigned : 1;
			unsigned EN_GAINCAL: 1;
			unsigned EN_OFFCAL: 1;
			unsigned WIDTH: 2;
			unsigned WRITE: 1;
			unsigned READ: 2;
			unsigned DRSTATUS: 2;
			unsigned DRMODE: 2;
            unsigned DR_HIZ : 1;
			unsigned : 1;
			unsigned MODOUT: 2;
        };
    };
} MCP3911_STATUSCOM_REG;


typedef struct sMCP3911
{
	uint32_t CS;
	int cvalue[2];
	uint8_t gain;
	uint8_t osr;
	uint8_t amclk_prescaler;
}sMCP3911;

sMCP3911 mcp;

void init_mcp3911(uint8_t pin_MISO, uint8_t pin_MOSI, uint8_t pin_SCK, uint8_t pin_CS, uint8_t pin_INT);
int mcp3911_is_ok();
void mcp3911_update_config();
uint8_t mcp3911_read();
void mcp3911_turnoff();
void set_skin_pin(uint8_t pin);
void set_skin_onoff(uint8_t is_on);
int mcp_get_filtered_value();
int mcp_get_filtered_skin();
void mcp_set_filter_mode(int use_filter);

void start_mcp_clock();
void stop_mcp_clock();

void mcp_fft_mode(int onoff);
int mcp_fft_process();
float *mcp_fft_get_spectr();
float sqrt_fast(float x);