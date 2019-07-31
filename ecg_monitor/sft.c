#include "sft.h"
#include <math.h>
#include <stdlib.h>

float sft_get_val(SSFT *st, int depth)
{
	if(depth >= st->data_length) depth = st->data_length - 1;
	int pp = st->data_pos - depth;
	if(pp < 0) pp += st->data_length;
	return st->data[pp];
}

void sft_init(SSFT *st, int size, float s_rate, int freqs, float *f_vals, float *f_periods)
{
	st->data_length = size;
	st->data = (float*)malloc(size*sizeof(float)*2);
	st->data_pos = 0;
	for(int x = 0; x < st->data_length; x++)
		st->data[x] = 0;
	
	st->sample_rate = s_rate;
	st->freq_periods = (int*)malloc(freqs*sizeof(int));
	st->ww = (float*)malloc(freqs*sizeof(float));
	st->spectr_r = (double*)malloc(freqs*sizeof(double));
	st->spectr_i = (double*)malloc(freqs*sizeof(double));
	st->spectr = (float*)malloc(freqs*sizeof(float));
	
	st->freq_count = freqs;
	for(int x = 0; x < st->freq_count; x++)
	{
		if(f_vals[x] < 0.001) f_vals[x] = 0.001;
//		ww[x] = f_vals[x] / (2.0 * 3.141592653589793 * sample_rate);
		st->ww[x] = f_vals[x] / st->sample_rate;
		st->spectr_i[x] = 0;
		st->spectr_r[x] = 0;
		st->spectr[x] = 0;
		if(f_periods == 0)
			st->freq_periods[x] = size;
		else
		{
			int pp = f_periods[x] * st->sample_rate / f_vals[x];
			if(pp <= size) st->freq_periods[x] = pp;
			else st->freq_periods[x] = size;
		}
	}
}

void sft_delete(SSFT *st)
{
	free(st->data);
	free(st->freq_periods);
	free(st->ww);
	free(st->spectr_r);
	free(st->spectr_i);
	free(st->spectr);
}

void sft_full_recalc(SSFT *st)
{
	for(int F = 0; F < st->freq_count; F++)
	{
		st->spectr_r[F] = 0;
		st->spectr_i[F] = 0;
		int mH = st->freq_periods[F]-1;
		if(mH < 1) mH = 1;
		for(int x = 1; x < mH; x++)
		{
			double H = x;
			double vH = sft_get_val(st, x);
			double fr = -2.0*3.141592653589793 * st->ww[F];
			double cfrH = cos(fr*H);
			double sfrH = sin(fr*H);
			st->spectr_r[F] += vH * cfrH;
			st->spectr_i[F] += vH * sfrH;
		}
		st->spectr[F] = sqrt(st->spectr_r[F]*st->spectr_r[F] + st->spectr_i[F]*st->spectr_i[F]);
	}
}

void sft_add_point(SSFT *st, float val)
{
	for(int F = 0; F < st->freq_count; F++)
	{
		double H = st->freq_periods[F]-1;
		if(H < 1) H = 1;
		double vH = sft_get_val(st, H);
		double fr = -2.0*3.141592653589793 * st->ww[F];
		double cfr = cos(fr);
		double sfr = sin(fr);
		double cfrH = cos(fr*H);
		double sfrH = sin(fr*H);
		double ns_r = (st->spectr_r[F] - vH * cfrH + val)*0.9999;
		double ns_i = (st->spectr_i[F] - vH * sfrH)*0.9999;
		st->spectr_r[F] = ns_r*cfr - ns_i*sfr;
		st->spectr_i[F] = ns_r*sfr + ns_i*cfr;
		st->spectr[F] = sqrt(st->spectr_r[F]*st->spectr_r[F] + st->spectr_i[F]*st->spectr_i[F]);
	}
	st->data[st->data_pos] = val;
	st->data_pos++;
	if(st->data_pos >= st->data_length)
		st->data_pos = 0;
//	full_recalc();
}
float * sft_get_spectr(SSFT *st)
{
	return st->spectr;
}
void sft_clear(SSFT *st)
{
	for(int x = 0; x < st->data_length; x++)
		st->data[x] = 0;
	for(int F = 0; F < st->freq_count; F++)
	{
		st->spectr[F] = 0;
		st->spectr_i[F] = 0;
		st->spectr_r[F] = 0;
	}
}
