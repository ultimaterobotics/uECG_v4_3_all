#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SFFTools
{
	int *iRevX;
	int *iRevY;

	float *dat2d_r;
	float *dat2d_i;
	float *tdat2d_r;
	float *tdat2d_i;
	float *spct2d_r;
	float *spct2d_i;
	float *phasecorr2d_r;
	float *phasecorr2d_i;

	float *dat1d_r;
	float *dat1d_i;
	float *spct1d_r;
	float *spct1d_i;
	float *phasecorr1d_r;
	float *phasecorr1d_i;

	float *tmpd_r;
	float *tmpd_i;
	float *tmps_r;
	float *tmps_i;

	float *tmp2d_r;
	float *tmp2d_i;

	float *tempX;
	float *tempY;

	float *store1D;
	float *store2D;

	int dataSZX;
	int dataSZY;
	int fftSZX;
	int fftSZY;
	int fftpowX;
	int fftpowY;
}SFFTools;

void sfft_butterfly(SFFTools *ft, int xy, float *dat_r, float *dat_i, float *spct_r, float *spct_i);
//	void sfft_CoolTk(int xy, float *dat_r, float *dat_i, float *spct_r, float *spct_i);
//	void sfft_CoolTk_rec(float *d_r, float *d_i, float *s_r, float *s_i, int N, int s);
//	void FFT_wiki(float *Rdat, float *Idat, int N, int LogN, int inverse = 0);

void sfftX(SFFTools *ft, float *dat_r, float *dat_i, float *spct_r, float *spct_i, int inverse);
void sfftY(SFFTools *ft, float *dat_r, float *dat_i, float *spct_r, float *spct_i, int inverse);

void sfft_1D_init(SFFTools *ft, int size);
void sfft_1D(SFFTools *ft, float *data_r, float *data_i, int inverse);
float *sfft_get_data1D_r(SFFTools *ft);
float *sfft_get_data1D_i(SFFTools *ft);
float *sfft_get_spect1D_r(SFFTools *ft);
float *sfft_get_spect1D_i(SFFTools *ft);
float *sfft_get_phase_corr1D_r(SFFTools *ft);
float *sfft_get_phase_corr1D_i(SFFTools *ft);
int sfft_get_spct_SZX(SFFTools *ft);
int sfft_get_spct_SZY(SFFTools *ft);
void sfft_2D_init(SFFTools *ft, int sizeX, int sizeY);
void sfft_2D(SFFTools *ft, float *data_r, float *data_i, int inverse);
float *sfft_get_data2D_r(SFFTools *ft);
float *sfft_get_data2D_i(SFFTools *ft);
float *sfft_get_spect2D_r(SFFTools *ft);
float *sfft_get_spect2D_i(SFFTools *ft);
float *sfft_get_phase_corr2D_r(SFFTools *ft);
float *sfft_get_phase_corr2D_i(SFFTools *ft);

void sfft_calc_phase_corr2D(SFFTools *ft, SFFTools *fft2);
void sfft_calc_phase_corr1D(SFFTools *ft, SFFTools *fft2);

void sfft_store_spectr1D(SFFTools *ft);
void sfft_store_spectr2D(SFFTools *ft);
void sfft_store_data1D(SFFTools *ft);
void sfft_store_data2D(SFFTools *ft);
void sfft_restore_spectr1D(SFFTools *ft);
void sfft_restore_spectr2D(SFFTools *ft);
void sfft_restore_data1D(SFFTools *ft);
void sfft_restore_data2D(SFFTools *ft);

void sfft_delete(SFFTools *ft); 

/*
class CFTracker
{
private:
	CFTools **ffts;
	int trackLen;
	int lastFT;
	float *shiftsX;
	float *shiftsY;
	float *shiftsR;
	float *avgSX;
	float *avgSY;
	int dim;
	float curDX;
	float curDY;
	void filterShifts();
	void dimFFT(float *data_r, float *data_i, int num);
public:
	CFTracker() {};
	~CFTracker();
	void init1D(int size, int trackerLength);
	void init2D(int sizeX, int sizeY, int trackerLength);
	void addFrame(float *data_r, float *data_i);
	int getTrackLength() {return trackLen;};
	CFTools* getCurFTool() {return ffts[lastFT];};
	CFTools* getFToolN(int n);
	float getFilteredDX();
	float getFilteredDY();
};
*/
