typedef struct SSFT
{
	float *data;
	float *ww;
	int data_length;
	int freq_count;
	int *freq_periods;
	int data_pos;
	float sample_rate;
	double *spectr_r;
	double *spectr_i;
	float *spectr;
}SSFT;
	
float sft_get_val(SSFT *st, int depth);
void sft_init(SSFT *st, int size, float sample_rate, int freqs, float *f_vals, float *f_periods);
void sft_delete(SSFT *st);
void sft_add_point(SSFT *st, float val);
void sft_full_recalc(SSFT *st);
float * sft_get_spectr(SSFT *st);
void sft_clear(SSFT *st);


