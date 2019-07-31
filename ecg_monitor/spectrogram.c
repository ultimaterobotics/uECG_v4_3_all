#include <string.h>
#include "spectrogram.h"
#include <math.h>
#include <stdlib.h>

int str_eq(const char *inputText1, const char *inputText2)
{
	int x = 0;
	if(inputText1 == NULL && inputText2 == NULL) return 1;
	if(inputText1 == NULL || inputText2 == NULL) return 0;
	while(inputText1[x] != 0 && inputText2[x] != 0)
	{
		if(inputText1[x] != inputText2[x]) return 0;
		++x;
	}
	if(inputText1[x] != inputText2[x]) return 0;
	return 1;
}

int col_r, col_g, col_b;

void val2col(SSpectrogramView *sv, float val)
{
	float tb = 10.0*sv->color_scale*0.01;
	float tg = 100.0*sv->color_scale*0.01;
	float tr = 1000.0*sv->color_scale*0.01;
	float wg = tg-tb;
	float wr = tr-tg;
	int *r = &col_r;
	int *g = &col_g;
	int *b = &col_b;
	if(val < tb)
	{
		*r = 0;
		*g = 0;
		*b = val/tb*255;
		return;
	}
	if(val < tg)
	{
		*r = 0;
		*b = (tg-val-tb)/wg*255;
		*g = (val-tb)/wg*255;
		return;
	}
	if(val < tr)
	{
		*b = 0;
		*g = (tr-val-tg)/wr*255;
		*r = (val-tg)/wr*255;
		return;
	}
	*r = 255;
	*g = val/tr*2.5;
	*b = val/tr*25.5;
	if(*g > 255) *g = 255;
	if(*b > 255) *b = 255;
}


void spg_calc_stat(SSpectrogramView *sv)
{
	for(int y = 0; y < sv->fft_hsize; y++)
	{
		sv->mean[y] = 0;
		sv->sdv[y] = 0.00000001;
	}
	for(int x = 0; x < sv->spg_length; x++)
	{
		float *sp = sv->data + sv->fft_hsize*x;
		for(int y = 0; y < sv->fft_hsize; y++)
			sv->mean[y] += sp[y];
	}
	for(int y = 0; y < sv->fft_hsize; y++)
		sv->mean[y] /= (float)sv->spg_length;
	for(int x = 0; x < sv->spg_length; x++)
	{
		float *sp = sv->data + sv->fft_hsize*x;
		for(int y = 0; y < sv->fft_hsize; y++)
		{
			float ds = sp[y] - sv->mean[y];
			sv->sdv[y] += ds*ds;
		}
	}
	for(int y = 0; y < sv->fft_hsize; y++)
	{
		sv->sdv[y] /= (float)sv->spg_length;
		sv->sdv[y] = sqrt(sv->sdv[y]);
	}
}

void spg_init(SSpectrogramView *sv, int length, int size)
{
	sv->spg_length = length;
	sv->fft_hsize = size;
	sv->data_size = length*sv->fft_hsize;
	if(sv->data_size < 1) sv->data_size = 1;
	sv->data = (float*)malloc(sv->data_size*sizeof(float));
	sv->mean = (float*)malloc(sv->fft_hsize*sizeof(float));
	sv->sdv = (float*)malloc(sv->fft_hsize*sizeof(float));
	for(int x = 0; x < sv->data_size; ++x) sv->data[x] = 0;
	sv->data_pos = 0;
	sv->scaling_type = 0;
	sv->start_freq = 0;
	sv->color_scale = 300;
	sv->end_freq = sv->fft_hsize;
}
void spg_delete(SSpectrogramView *sv)
{
	free(sv->data);
	free(sv->mean);
	free(sv->sdv);
}

void spg_set_viewport(SSpectrogramView *sv, int dx, int dy, int sizeX, int sizeY)
{
	sv->DX = dx;
	sv->DY = dy;
	sv->SX = sizeX;
	sv->SY = sizeY;
}
void spg_set_parameter_str(SSpectrogramView *sv, const char *name, const char *value)
{
	if(str_eq(name, "scaling"))
	{
		if(str_eq(value, "normal")) sv->scaling_type = 0; 
		if(str_eq(value, "adaptive")) sv->scaling_type = 1; 
	}
}
void spg_set_parameter_float(SSpectrogramView *sv, const char *name, float value)
{
	if(str_eq(name, "color scale")) sv->color_scale = value;
	if(str_eq(name, "from frequency")) sv->start_freq = value;
	if(str_eq(name, "to frequency")) sv->end_freq = value;
	if(sv->end_freq < 2) sv->end_freq = 2;
	if(sv->end_freq >= sv->fft_hsize) sv->end_freq = sv->fft_hsize;
	if(sv->start_freq >= sv->end_freq) sv->start_freq = sv->end_freq-1;
	if(sv->start_freq < 0) sv->start_freq = 0;
}
void spg_clear(SSpectrogramView *sv)
{
	for(int x = 0; x < sv->data_size; ++x) sv->data[x] = 0;
}
void spg_add_spectr(SSpectrogramView *sv, float *spectr)
{
	memcpy(sv->data + sv->data_pos*sv->fft_hsize, spectr, sv->fft_hsize*sizeof(float));
	sv->data_pos++;
	if(sv->data_pos >= sv->spg_length) sv->data_pos = 0;
	spg_calc_stat(sv);
}
float *spg_get_spectr(SSpectrogramView *sv, int hist_depth)
{
	int p = sv->data_pos - hist_depth;
	while(p < 0) p += sv->spg_length;
	return sv->data + p*sv->fft_hsize;
}
void spg_draw(SSpectrogramView *sv, uint8_t* drawPix, int w, int h)
{
	float mSX = 1.0 / (float)(sv->SX-1);
	float mSY = 1.0 / (float)(sv->SY-1);
	for(int x = 0; x < sv->SX; ++x)
	{
		float rp = x; 
		rp *= mSX;
		rp *= sv->spg_length;
		int i1 = rp;
		int i2 = i1+1;
		if(i2 >= sv->spg_length) i2 = sv->spg_length-1;
		float cx2 = rp - i1, cx1 = i2 - rp;
		float *sp1 = spg_get_spectr(sv, i1+1);
		float *sp2 = spg_get_spectr(sv, i2+1);
//		if(x == sv->SX-1)
//			printf("%d %d %d\n", x, sv->start_freq, sv->end_freq);
//		continue;
		for(int y = 0; y < sv->SY; y++)
		{
			float ry = y;
			ry *= mSY;
			float rpy = sv->start_freq + ry*(sv->end_freq - sv->start_freq);
			int y1 = rpy;
			int y2 = y1 + 1;
			if(y2 >= sv->fft_hsize) y2 = sv->fft_hsize-1;
			float cy2 = rpy - y1, cy1 = y2 - rpy;

			float vx1y1 = sp1[y1];
			float vx2y1 = sp2[y1];
			float vx1y2 = sp1[y2];
			float vx2y2 = sp2[y2];
//			float v = cx1*cy1*vx1y1 + cx2*cy1*vx2y1 + cx1*cy2*vx1y2 + cx2*cy2*vx2y2;
			float v = vx1y1;
			if(sv->scaling_type == 1) 
			{
				if(sv->mean[y1] != 0)
					v = 1000.0 * (v / sv->mean[y1]);// / sdv[y1];
				else v = 0;
			}
			int xx = sv->DX + sv->SX-1 - x;
			int yy = sv->DY + sv->SY - y;
			int idx = (yy*w + xx);
			if(xx >= 0 && xx < w && yy >= 0 && yy < h)
			{
				val2col(sv, v);
				drawPix[idx*3 + 0] = col_r;
				drawPix[idx*3 + 1] = col_g;
				drawPix[idx*3 + 2] = col_b;
//				((unsigned int*)drawPix)[idx] = (col_r<<16) | (col_g<<8) | col_b;
			}
		}
	}
}
void spg_draw_diff(SSpectrogramView *sv, uint8_t* drawPix, int w, int h, SSpectrogramView *sv2)
{
	float mSX = 1.0 / (float)(sv->SX-1);
	float mSY = 1.0 / (float)(sv->SY-1);
	for(int x = 0; x < sv->SX; ++x)
	{
		float rp = x; 
		rp *= mSX;
		rp *= sv->spg_length;
		int i1 = rp;
		int i2 = i1+1;
		if(i2 >= sv->spg_length) i2 = sv->spg_length-1;
		float cx2 = rp - i1, cx1 = i2 - rp;
		float *sp11 = spg_get_spectr(sv, i1+1);
		float *sp12 = spg_get_spectr(sv, i2+1);
		
		float *sp21 = spg_get_spectr(sv2, i1+1);
		float *sp22 = spg_get_spectr(sv2, i2+1);
		
		float avg11 = 0, avg12 = 0, avg21 = 0, avg22 = 0;
		float sdv11 = 0.00001, sdv12 = 0.00001, sdv21 = 0.00001, sdv22 = 0.00001;
		for(int n = 0; n < sv->fft_hsize; n++)
		{
			avg11 += sp11[n];
			avg12 += sp12[n];
			avg21 += sp21[n];
			avg22 += sp22[n];
		}
		avg11 /= (float)sv->fft_hsize;
		avg12 /= (float)sv->fft_hsize;
		avg21 /= (float)sv->fft_hsize;
		avg22 /= (float)sv->fft_hsize;
		for(int n = 0; n < sv->fft_hsize; n++)
		{
			sdv11 += (sp11[n] - avg11)*(sp11[n] - avg11);
			sdv12 += (sp12[n] - avg12)*(sp12[n] - avg12);
			sdv21 += (sp21[n] - avg21)*(sp21[n] - avg21);
			sdv22 += (sp22[n] - avg22)*(sp22[n] - avg22);
		}
		sdv11 = sqrt(sdv11/(float)sv->fft_hsize);
		sdv12 = sqrt(sdv12/(float)sv->fft_hsize);
		sdv21 = sqrt(sdv21/(float)sv->fft_hsize);
		sdv22 = sqrt(sdv22/(float)sv->fft_hsize);
		
		for(int y = 0; y < sv->SY; y++)
		{
			float ry = y;
			ry *= mSY;
			float rpy = sv->start_freq + ry*(sv->end_freq - sv->start_freq);
			int y1 = rpy;
			int y2 = y1 + 1;
			if(y2 >= sv->fft_hsize) y2 = sv->fft_hsize-1;
			float cy2 = rpy - y1, cy1 = y2 - rpy;

			float vx1y1 = sp11[y1] - ((sp21[y1] - avg21) * sdv11 / sdv21 + avg11);
			float vx2y1 = sp12[y1] - ((sp22[y1] - avg22) * sdv12 / sdv22 + avg12);
			float vx1y2 = sp11[y2] - ((sp21[y2] - avg21) * sdv11 / sdv21 + avg11);
			float vx2y2 = sp12[y2] - ((sp22[y2] - avg22) * sdv12 / sdv22 + avg12);
			float v = cx1*cy1*vx1y1 + cx2*cy1*vx2y1 + cx1*cy2*vx1y2 + cx2*cy2*vx2y2;
			if(v < 0) v = -v;
			if(sv->scaling_type == 1) 
				v = 1000.0 * (v / sv->mean[y1]);// / sdv[y1];
			int xx = sv->DX + sv->SX-1 - x;
			int yy = sv->DY + sv->SY - y;
			int idx = (yy*w + xx);
			if(xx >= 0 && xx < w && yy >= 0 && yy < h)
			{
				val2col(sv, v);
				((unsigned int*)drawPix)[idx] = (col_r<<16) | (col_g<<8) | col_b;
			}
		}
	}
}
int spg_get_x(SSpectrogramView *sv) {return sv->DX;}
int spg_get_y(SSpectrogramView *sv) {return sv->DY;}
int spg_get_size_x(SSpectrogramView *sv) {return sv->SX;}
int spg_get_size_y(SSpectrogramView *sv) {return sv->SY;}
int spg_get_length(SSpectrogramView *sv) {return sv->spg_length;}
