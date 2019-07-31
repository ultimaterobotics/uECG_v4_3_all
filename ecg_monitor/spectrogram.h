#include <string.h>
#include <stdint.h>

typedef struct SSpectrogramView
{
	int fft_hsize;
	int spg_length;
	int data_size;
	float *data;
	float *mean;
	float *sdv;
	int data_pos; 
	float color_scale;
	int scaling_type;
	int start_freq;
	int end_freq;
	int DX, DY;
	int SX, SY;
}SSpectrogramView;

void spg_calc_stat(SSpectrogramView *sv);
void spg_init(SSpectrogramView *sv, int length, int size);
void spg_delete(SSpectrogramView *sv);
void spg_set_viewport(SSpectrogramView *sv, int dx, int dy, int sizeX, int sizeY);
void spg_set_parameter_str(SSpectrogramView *sv, const char *name, const char *value);
void spg_set_parameter_float(SSpectrogramView *sv, const char *name, float value);
void spg_clear(SSpectrogramView *sv);
void spg_add_spectr(SSpectrogramView *sv, float *spectr);
float *spg_get_spectr(SSpectrogramView *sv, int hist_depth);
void spg_draw(SSpectrogramView *sv, uint8_t* drawPix, int w, int h);
void spg_draw_diff(SSpectrogramView *sv, uint8_t* drawPix, int w, int h, SSpectrogramView *sv2);
int spg_get_x(SSpectrogramView *sv);
int spg_get_y(SSpectrogramView *sv);
int spg_get_size_x(SSpectrogramView *sv);
int spg_get_size_y(SSpectrogramView *sv);
int spg_get_length(SSpectrogramView *sv);
