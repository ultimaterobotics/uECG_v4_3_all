#include "fft.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float PI = 3.1415926;

/*
void sfft_butterfly()
{
	//128: 189
	//179-15-128 = 36
	//64: 132   122 - 15 - 64 = 43
	//256: 317  307 - 15 - 256 = 36
	sp_mid_avg = 15 + fft_pos + fft_cnt; 
	fft_cnt++;
	if(fft_cnt > 10000) fft_cnt = 0;
	fft_is_pending = 0;
	fft_pos = 0;

	int length;
	int x, y, s;
	length = fftSZX; 

//	float sum2R = 0, sum2I = 0;
	for(x = 0; x < length; ++ x)
	{
		temp[x] = data[iRev[x]]; //Re
		temp[x+length] = 0;
//		sum2R += temp[x]*temp[x];
	}
	
// algorithm iterative-fft is
//    input: Array a of n complex values where n is a power of 2
//    output: Array A the DFT of a

//    bit-reverse-copy(a,A)
//    n ← a.length 
//    for s = 1 to log(n)
//        m ← 2s
//        ωm ← exp(−2πi/m) 
//        for k = 0 to n-1 by m
//            ω ← 1
//            for j = 0 to m/2 – 1
//                t ← ω A[k + j + m/2]
//                u ← A[k + j]
//                A[k + j] ← u + t
//                A[k + j + m/2] ← u – t
//                ω ← ω ωm
//   
//    return A
	
	float *aR = temp;
	float *aI = temp + length;
	int m = 1;
	for(int s = 0; s < pow2; s++)
	{
		float r = -2.0 * PI / (float)m;
		float wmR = cos_f(r);
		float wmI = -sin_f(r);
		for(int k = 0; k < length-1; k += m)
		{
			float wR = 1.0;
			float wI = 0;
			int m2 = m/2;
			for(int j = 0; j < m2 - 1; j++)
			{
				float tR = wR * aR[k + j + m2] - wI * aI[k + j + m2];
				float tI = wI * aR[k + j + m2] + wR * aI[k + j + m2];
				float uR = aR[k + j];
				float uI = aI[k + j];
				aR[k + j] = uR + tR;
				aI[k + j] = uI + tI;
				aR[k + j + m2] = uR - tR;
				aI[k + j + m2] = uI - tI;
				float wrr = wR * wmR - wI * wmI;
				float wii = wI * wmR + wR * wmI;
				wR = wrr;
				wI = wii;
			}
		}
		m *= 2;
	}
	float avg2R = 0, avg2I = 0, avg_mid = 0;
	for(x = 0; x < length; ++ x)
	{
//		spctr_r[x] = temp[x];
//		spctr_i[x] = -temp[x+length];
		avg2R += temp[x]*temp[x];
		if(x > 1 && x < 12 && x != 6 && x != 7) avg_mid += temp[x]*temp[x];
//		avg2I += temp[x+length]*temp[x+length];
	}

	float vv = avg_mid*0.0001;
	if(vv < 1) vv = 0; 
	if(vv > 60000) vv = 0;
	sp_mid_avg = vv;
//	sp_mid_avg = avg_mid + 15;
	fft_is_pending = 0;
	fft_pos = 0;
	return;
}*/

void sfft_butterfly(SFFTools *ft, int xy, float *dat_r, float *dat_i, float *spct_r, float *spct_i)
{
	int length;
	int x, y, s;
	int pow2;
	float *temp;
	int *iRev;
	if(xy == 0) //X
	{
		length = ft->fftSZX;
		pow2 = ft->fftpowX;
		temp = ft->tempX;
		iRev = ft->iRevX;
	}
	else if(xy == 1) //Y
	{
		length = ft->fftSZY;
		pow2 = ft->fftpowY;
		temp = ft->tempY;
		iRev = ft->iRevY;
	}
	else
	{
		printf("SFFTools sfft_butterfly: wrong xy value (%d)\n", xy);
		return;
	}
	float sum2R = 0, sum2I = 0;
	for(x = 0; x < length; ++ x)
	{
		temp[x] = dat_r[iRev[x]]; //Re
		temp[x+length] = dat_i[iRev[x]];      //Im
		sum2R += temp[x]*temp[x];
		sum2I += temp[x+length]*temp[x+length];
	}

	float *aR = temp;
	float *aI = temp + length;
	int m = 1;
	for(int s = 0; s < pow2; s++)
	{
		float r = -2.0 * PI / (float)m;
		float wmR = cos(r);
		float wmI = -sin(r);
		for(int k = 0; k < length-1; k += m)
		{
			float wR = 1.0;
			float wI = 0;
			int m2 = m/2;
			for(int j = 0; j < m2 - 1; j++)
			{
				float tR = wR * aR[k + j + m2] - wI * aI[k + j + m2];
				float tI = wI * aR[k + j + m2] + wR * aI[k + j + m2];
				float uR = aR[k + j];
				float uI = aI[k + j];
				aR[k + j] = uR + tR;
				aI[k + j] = uI + tI;
				aR[k + j + m2] = uR - tR;
				aI[k + j + m2] = uI - tI;
				float wrr = wR * wmR - wI * wmI;
				float wii = wI * wmR + wR * wmI;
				wR = wrr;
				wI = wii;
			}
		}
		m *= 2;
	}
/*	float avg2R = 0, avg2I = 0, avg_mid = 0;

	int btSepar = 1;
	int btWidth = 1;
	int btLength = length;
	float wnR, wnI;
	float mLength = 1.0 / (float)length;
	for(s = 0; s < pow2; ++ s)
	{
		btWidth = btSepar;
		btSepar<<=1;
//		float r = y*btLength *2*PI *mLength;
		float r;// = PI / (float)btWidth;
//		wnR = cos(r);
//		wnI = -sin(r);
		for(y = 0; y < btWidth; ++ y)
		{
			x = y;
			r = x * 2 * PI / (float)btSepar;
			btLength >>=  1;
			wnR = cos(r);
			wnI = -sin(r);
			while(x < length)
			{
				int xx = x + btWidth;
				float tRe, tIm;
				tRe = temp[xx] * wnR - temp[xx+length] * wnI;
				tIm = temp[xx] * wnI + temp[xx+length] * wnR;
				temp[xx] = temp[x] - tRe;
				temp[xx+length] = temp[x+length] - tIm;
				temp[x] = temp[x] + tRe;
				temp[x+length] = temp[x+length] + tIm;
				x += btSepar;
			}
		}
	}*/
	float avg2R = 0, avg2I = 0;
	if(xy == 0)
	{
		for(x = 0; x < length; ++ x)
		{
			spct_r[x] = temp[x];
			spct_i[x] = -temp[x+length];
			avg2R += temp[x]*temp[x];
			avg2I += temp[x+length]*temp[x+length];
		}
	}
	if(xy == 1)
	{
		for(x = 0; x < length; ++ x)
		{
			int nx = length-x;
			if(nx == length) nx = 0;
			spct_r[nx] = temp[x];
			spct_i[nx] = -temp[x+length];
			avg2R += temp[x]*temp[x];
			avg2I += temp[x+length]*temp[x+length];
		}
	}
}
/*
void CFTools::fft_CoolTk_rec(float *d_r, float *d_i, float *s_r, float *s_i, int N, int s)
{
	if(N == 1)
	{
		s_r[0] = d_r[0];
		s_i[0] = d_i[0];
		return;
	}
	int N2 = N/2;
	fft_CoolTk_rec(d_r, d_i, s_r, s_i, N2, s*2);
	fft_CoolTk_rec(d_r+s, d_i+s, s_r+N2, s_i+N2, N2, s*2);
	float kN = -PI/(float)N2;
	for(int x = 0; x < N2; ++x)
	{
		float k = (float)x*kN;
		s_r[x] += cos(k)*s_r[x+N2];
		s_i[x] -= sin(k)*s_i[x+N2];
	}
}

void CFTools::fft_CoolTk(int N, float *dat_r, float *dat_i, float *spct_r, float *spct_i)
{
	fft_CoolTk_rec(dat_r, dat_i, spct_r, spct_i, N, 1);
}
 
void CFTools::FFT_wiki(float *Rdat, float *Idat, int N, int LogN, int inverse)
{
	// parameters error check:
	if((Rdat == NULL) || (Idat == NULL)) return;
	if((N > 16384) || (N < 1)) return;
	if((LogN < 2) || (LogN > 14)) return;
 
	register int  i, j, n, k, io, ie, in, nn;
	float         ru, iu, rtp, itp, rtq, itq, rw, iw, sr;
 
	static const float Rcoef[14] =
	{  -1.0000000000000000F,  0.0000000000000000F,  0.7071067811865475F,
		0.9238795325112867F,  0.9807852804032304F,  0.9951847266721969F,
		0.9987954562051724F,  0.9996988186962042F,  0.9999247018391445F,
		0.9999811752826011F,  0.9999952938095761F,  0.9999988234517018F,
		0.9999997058628822F,  0.9999999264657178F
	};
	static const float Icoef[14] =
	{   0.0000000000000000F, -1.0000000000000000F, -0.7071067811865474F,
		-0.3826834323650897F, -0.1950903220161282F, -0.0980171403295606F,
		-0.0490676743274180F, -0.0245412285229122F, -0.0122715382857199F,
		-0.0061358846491544F, -0.0030679567629659F, -0.0015339801862847F,
		-0.0007669903187427F, -0.0003834951875714F
	};
 
	nn = N >> 1;
	ie = N;
	for(n=1; n<=LogN; n++)
	{
		rw = Rcoef[LogN - n];
		iw = Icoef[LogN - n];
		if(inverse) iw = -iw;
		in = ie >> 1;
		ru = 1.0F;
		iu = 0.0F;
		for(j=0; j<in; j++)
		{
			for(i=j; i<N; i+=ie)
			{
				io       = i + in;
				rtp      = Rdat[i]  + Rdat[io];
				itp      = Idat[i]  + Idat[io];
				rtq      = Rdat[i]  - Rdat[io];
				itq      = Idat[i]  - Idat[io];
				Rdat[io] = rtq * ru - itq * iu;
				Idat[io] = itq * ru + rtq * iu;
				Rdat[i]  = rtp;
				Idat[i]  = itp;
			}
			sr = ru;
			ru = ru * rw - iu * iw;
			iu = iu * rw + sr * iw;
		}
		ie >>= 1;
	}

	for(j=i=1; i<N; i++)
	{
		if(i < j)
		{
			io       = i - 1;
			in       = j - 1;
			rtp      = Rdat[in];
			itp      = Idat[in];
			Rdat[in] = Rdat[io];
			Idat[in] = Idat[io];
			Rdat[io] = rtp;
			Idat[io] = itp;
		}
		k = nn;
		while(k < j)
		{
			j   = j - k;
			k >>= 1;
		}
		j = j + k;
	}

	if(!inverse) return;

	rw = 1.0F / N;

	for(i=0; i<N; i++)
	{
		Rdat[i] *= rw;
		Idat[i] *= rw;
	}
}*/

void sfftX(SFFTools *ft, float *dat_r, float *dat_i, float *spct_r, float *spct_i, int inverse)
{
	sfft_butterfly(ft, 0, dat_r, dat_i, spct_r, spct_i);
}
void sfftY(SFFTools *ft, float *dat_r, float *dat_i, float *spct_r, float *spct_i, int inverse)
{
	sfft_butterfly(ft, 1, dat_r, dat_i, spct_r, spct_i);
//	fft_CoolTk(fftSZY, dat_r, dat_i, spct_r, spct_i);
//	memcpy(spct_r, dat_r, fftSZY*sizeof(float));
//	memcpy(spct_i, dat_i, fftSZY*sizeof(float));
//	FFT_wiki(spct_r, spct_i, fftSZY, fftpowY, inverse);
}

void sfft_1D_init(SFFTools *ft, int size)
{
	if(size < 2) return;
	ft->dataSZX = size;
	int pow2 = 0;
	while(size >> pow2 > 1) pow2++;
	if(1<<pow2 < size) pow2++;
	ft->fftpowX = pow2;
	ft->fftSZX = 1<<pow2;
	ft->dat1d_r = (float*)malloc(ft->fftSZX*sizeof(float));
	ft->dat1d_i = (float*)malloc(ft->fftSZX*sizeof(float));
	ft->spct1d_r = (float*)malloc(ft->fftSZX*sizeof(float));
	ft->spct1d_i = (float*)malloc(ft->fftSZX*sizeof(float));
	ft->phasecorr1d_r = (float*)malloc(ft->fftSZX*sizeof(float));
	ft->phasecorr1d_i = (float*)malloc(ft->fftSZX*sizeof(float));
	ft->iRevX = (int*)malloc(ft->fftSZX*sizeof(int));
	ft->tempX = (float*)malloc(ft->fftSZX*4*sizeof(float));
	ft->store1D = (float*)malloc(ft->fftSZX*4*sizeof(float));

	for(int x = 0; x < ft->fftSZX; ++ x)
	{
		int y = 0;
		int tx = x;
		for(int z = 0; z < pow2; ++ z)
		{
			y<<=1;
			y += tx&1;
			tx>>=1;
		}
		ft->iRevX[x] = y;
	}
}
void sfft_1D(SFFTools *ft, float *data_r, float *data_i, int inverse)
{
	memset(ft->dat1d_r, 0, ft->fftSZX*sizeof(float));
	memset(ft->dat1d_i, 0, ft->fftSZX*sizeof(float));
	if(data_r != NULL)
		memcpy(ft->dat1d_r, data_r, ft->dataSZX*sizeof(float));
	if(data_i != NULL)
		memcpy(ft->dat1d_i, data_i, ft->dataSZX*sizeof(float));
	sfftX(ft, ft->dat1d_r, ft->dat1d_i, ft->spct1d_r, ft->spct1d_i, inverse);
}
void sfft_2D_init(SFFTools *ft, int sizeX, int sizeY)
{
	if(sizeX < 2) return;
	if(sizeY < 2) return;
	sfft_1D_init(ft, sizeX);

	ft->dataSZY = sizeY;
	int pow2 = 0;
	while(sizeY >> pow2 > 1) pow2++;
	if(1<<pow2 < sizeY) pow2++;
	ft->fftpowY = pow2;
	ft->fftSZY = 1<<pow2;
	ft->tmpd_r = (float*)malloc(ft->fftSZY*sizeof(float));
	ft->tmpd_i = (float*)malloc(ft->fftSZY*sizeof(float));
	ft->tmps_r = (float*)malloc(ft->fftSZY*sizeof(float));
	ft->tmps_i = (float*)malloc(ft->fftSZY*sizeof(float));

	ft->dat2d_r = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));
	ft->dat2d_i = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));
	ft->tdat2d_r = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));
	ft->tdat2d_i = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));
	ft->spct2d_r = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));
	ft->spct2d_i = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));
	ft->phasecorr2d_r = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));
	ft->phasecorr2d_i = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));

	ft->tmp2d_r = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));
	ft->tmp2d_i = (float*)malloc(ft->fftSZX*ft->fftSZY*sizeof(float));

	ft->tempY = (float*)malloc(4*ft->fftSZY*sizeof(float));
	ft->store2D = (float*)malloc(4*ft->fftSZX*ft->fftSZY*sizeof(float));

	ft->iRevY = (int*)malloc(ft->fftSZY*sizeof(int));

	for(int x = 0; x < ft->fftSZY; ++ x)
	{
		int y = 0;
		int tx = x;
		for(int z = 0; z < pow2; ++ z)
		{
			y<<=1;
			y += tx&1;
			tx>>=1;
		}
		ft->iRevY[x] = y;
	}
}
void sfft_2D(SFFTools *ft, float *data_r, float *data_i, int inverse)
{
	memset(ft->dat2d_r, 0, ft->fftSZX*ft->fftSZY*sizeof(float));
	memset(ft->dat2d_i, 0, ft->fftSZX*ft->fftSZY*sizeof(float));
	memset(ft->spct2d_r, 0, ft->fftSZX*ft->fftSZY*sizeof(float));
	memset(ft->spct2d_i, 0, ft->fftSZX*ft->fftSZY*sizeof(float));

	for(int x = 0; x < ft->dataSZY; ++x)
	{
		if(data_r != NULL)
			memcpy(ft->dat2d_r+x*ft->fftSZX, data_r+x*ft->dataSZX, ft->dataSZX*sizeof(float));
		if(data_i != NULL)
			memcpy(ft->dat2d_i+x*ft->fftSZX, data_i+x*ft->dataSZX, ft->dataSZX*sizeof(float));
	}
	for(int x = 0; x < ft->fftSZY; ++x)
	{
		sfftX(ft, ft->dat2d_r+x*ft->fftSZX, ft->dat2d_i+x*ft->fftSZX, ft->spct2d_r+x*ft->fftSZX, ft->spct2d_i+x*ft->fftSZX, inverse);
		memcpy(ft->tdat2d_r+x*ft->fftSZX, ft->spct2d_r+x*ft->fftSZX, ft->fftSZX*sizeof(float));
		memcpy(ft->tdat2d_i+x*ft->fftSZX, ft->spct2d_i+x*ft->fftSZX, ft->fftSZX*sizeof(float));
	}
	for(int x = 0; x < ft->fftSZX; ++x)
	{
		for(int y = 0; y < ft->fftSZY; ++y)
		{
			ft->tmpd_r[y] = ft->tdat2d_r[y*ft->fftSZX+x];
			ft->tmpd_i[y] = ft->tdat2d_i[y*ft->fftSZX+x];
		}
		sfftY(ft, ft->tmpd_r, ft->tmpd_i, ft->tmps_r, ft->tmps_i, inverse);
		for(int y = 0; y < ft->fftSZY; ++y)
		{
			ft->spct2d_r[y*ft->fftSZX+x] = ft->tmps_r[y];
			ft->spct2d_i[y*ft->fftSZX+x] = ft->tmps_i[y];
		}
	}
}
void sfft_store_spectr1D(SFFTools *ft)
{
	memcpy(ft->store1D, ft->spct1d_r, ft->fftSZX*sizeof(float));
	memcpy(ft->store1D+ft->fftSZX, ft->spct1d_i, ft->fftSZX*sizeof(float));
}
void sfft_store_spectr2D(SFFTools *ft)
{
	memcpy(ft->store2D, ft->spct2d_r, ft->fftSZX*ft->fftSZY*sizeof(float));
	memcpy(ft->store2D+ft->fftSZX*ft->fftSZY, ft->spct2d_i, ft->fftSZX*ft->fftSZY*sizeof(float));
}
void sfft_store_data1D(SFFTools *ft)
{
	memcpy(ft->store1D+2*ft->fftSZX, ft->dat1d_r, ft->fftSZX*sizeof(float));
	memcpy(ft->store1D+3*ft->fftSZX, ft->dat1d_i, ft->fftSZX*sizeof(float));
}
void sfft_store_data2D(SFFTools *ft)
{
	memcpy(ft->store2D+2*ft->fftSZX*ft->fftSZY, ft->dat2d_r, ft->fftSZX*ft->fftSZY*sizeof(float));
	memcpy(ft->store2D+3*ft->fftSZX*ft->fftSZY, ft->dat2d_i, ft->fftSZX*ft->fftSZY*sizeof(float));
}
void sfft_restore_spectr1D(SFFTools *ft)
{
	memcpy(ft->spct1d_r, ft->store1D, ft->fftSZX*sizeof(float));
	memcpy(ft->spct1d_i, ft->store1D+ft->fftSZX, ft->fftSZX*sizeof(float));
}
void sfft_restore_spectr2D(SFFTools *ft)
{
	memcpy(ft->spct2d_r, ft->store2D, ft->fftSZX*ft->fftSZY*sizeof(float));
	memcpy(ft->spct2d_i, ft->store2D+ft->fftSZX*ft->fftSZY, ft->fftSZX*ft->fftSZY*sizeof(float));
}
void sfft_restore_data1D(SFFTools *ft)
{
	memcpy(ft->dat1d_r, ft->store1D+2*ft->fftSZX, ft->fftSZX*sizeof(float));
	memcpy(ft->dat1d_i, ft->store1D+3*ft->fftSZX, ft->fftSZX*sizeof(float));
}
void sfft_restore_data2D(SFFTools *ft)
{
	memcpy(ft->dat2d_r, ft->store2D+2*ft->fftSZX*ft->fftSZY, ft->fftSZX*ft->fftSZY*sizeof(float));
	memcpy(ft->dat2d_i, ft->store2D+3*ft->fftSZX*ft->fftSZY, ft->fftSZX*ft->fftSZY*sizeof(float));
}

void sfft_calc_phase_corr2D(SFFTools *ft, SFFTools *fft2)
{
	float norm2 = 0;
	for(int x = 0; x < ft->fftSZX; ++x)
	{
		for(int y = 0; y < ft->fftSZY; ++y)
		{
			float r1 = ft->spct2d_r[y*ft->fftSZX+x];
			float r2 = fft2->spct2d_r[y*fft2->fftSZX+x];
			float i1 = ft->spct2d_i[y*ft->fftSZX+x];
			float i2 = fft2->spct2d_i[y*fft2->fftSZX+x];
			float mult_r = r1*r2 + i1*i2;
			float mult_i = r2*i1 - r1*i2;
			float eps = 0.000000;
			float div = mult_r*mult_r + mult_i*mult_i + eps;
//			norm2 += div;
			if(div == 0) div = 1;
			div = 1.0 / sqrt(div);
			ft->tmp2d_r[y*ft->fftSZX+x] = div*mult_r;
			ft->tmp2d_i[y*ft->fftSZX+x] = div*mult_i;
//			printf("%g ", div*mult_r);
		}
//		printf("\n");
	}
//	printf("phasecorr2d: norm2=%g\n", norm2);
	sfft_2D(ft, ft->tmp2d_r, ft->tmp2d_i, 1);
//	fft2D(tmp2d_r, tmp2d_i);
	memcpy(ft->phasecorr2d_r, ft->spct2d_r, ft->fftSZX*ft->fftSZY*sizeof(float));
	memcpy(ft->phasecorr2d_i, ft->spct2d_i, ft->fftSZX*ft->fftSZY*sizeof(float));
}

void sfft_calc_phase_corr1D(SFFTools *ft, SFFTools *fft2)
{
	float norm2 = 0;
	for(int x = 0; x < ft->fftSZX; ++x)
	{
		float mult_r = ft->spct1d_r[x]*fft2->spct1d_r[x] + ft->spct1d_i[x]*fft2->spct1d_i[x];
		float mult_i = -ft->spct1d_r[x]*fft2->spct1d_i[x] + ft->spct1d_i[x]*fft2->spct1d_r[x];
		float div = mult_r*mult_r + mult_i*mult_i;
		if(div == 0) div = 1;
		div = 1.0 / sqrt(div);
		ft->tempX[x] = div*mult_r;
		ft->tempX[x+ft->fftSZX] = div*mult_i;
//		norm2 += mult_r*mult_r + mult_i*mult_i;
	}
//	norm2 /= (float)fftSZX;
//	float norm = sqrt(norm2);
//	if(norm < 0.00001) norm = 0.00001; //never possible for normal vectors
//	norm = 1.0 / norm;
//	printf("PhaseCorr: norm = %g\n", norm);
//	for(int x = 0; x < fftSZX; ++x)
//	{
//		tempX[x] *= norm;
//		tempX[x+fftSZX] *= norm;
//	}
	sfft_1D(ft, ft->tempX, ft->tempX+ft->fftSZX, 1);
	memcpy(ft->phasecorr1d_r, ft->spct1d_r, ft->fftSZX*sizeof(float));
	memcpy(ft->phasecorr1d_i, ft->spct1d_i, ft->fftSZX*sizeof(float));
}

void sfft_delete(SFFTools *ft)
{
	free(ft->iRevX); 
	free(ft->iRevY); 

	free(ft->dat2d_r);
	free(ft->dat2d_i);
	free(ft->tdat2d_r);
	free(ft->tdat2d_i);
	free(ft->spct2d_r);
	free(ft->spct2d_i);
	free(ft->phasecorr2d_r);
	free(ft->phasecorr2d_i);

	free(ft->dat1d_r);
	free(ft->dat1d_i);
	free(ft->spct1d_r);
	free(ft->spct1d_i);
	free(ft->phasecorr1d_r);
	free(ft->phasecorr1d_i);

	free(ft->tmpd_r);
	free(ft->tmpd_i);
	free(ft->tmps_r);
	free(ft->tmps_i);

	free(ft->tmp2d_r);
	free(ft->tmp2d_i);

	free(ft->tempX);
	free(ft->tempY);

	free(ft->store1D);
	free(ft->store2D);
}

float *sfft_get_data1D_r(SFFTools *ft) {return ft->dat1d_r;}
float *sfft_get_data1D_i(SFFTools *ft) {return ft->dat1d_i;}
float *sfft_get_spect1D_r(SFFTools *ft) {return ft->spct1d_r;}
float *sfft_get_spect1D_i(SFFTools *ft) {return ft->spct1d_i;}
float *sfft_get_phase_corr1D_r(SFFTools *ft) {return ft->phasecorr1d_r;}
float *sfft_get_phase_corr1D_i(SFFTools *ft) {return ft->phasecorr1d_i;}
int sfft_get_spct_SZX(SFFTools *ft) {return ft->fftSZX;}
int sfft_get_spct_SZY(SFFTools *ft) {return ft->fftSZY;}

float *sfft_get_data2D_r(SFFTools *ft) {return ft->dat2d_r;}
float *sfft_get_data2D_i(SFFTools *ft) {return ft->dat2d_i;}
float *sfft_get_spect2D_r(SFFTools *ft) {return ft->spct2d_r;}
float *sfft_get_spect2D_i(SFFTools *ft) {return ft->spct2d_i;}
float *sfft_get_phase_corr2D_r(SFFTools *ft) {return ft->phasecorr2d_r;}
float *sfft_get_phase_corr2D_i(SFFTools *ft) {return ft->phasecorr2d_i;}

/*
CFTracker::~CFTracker()
{
	for(int x = 0; x < trackLen; ++x)
		delete ffts[x];
	delete[] ffts;
	delete[] shiftsX;
	delete[] shiftsY;
}
void CFTracker::init1D(int size, int trackerLength)
{
	trackLen = trackerLength;
	if(trackLen < 1) trackLen = 1;
	if(trackLen > 100) trackLen = 100;
	ffts = new CFTools*[trackLen];
	shiftsX = new float[trackLen*trackLen];
	shiftsY = new float[trackLen*trackLen];
	shiftsR = new float[trackLen*trackLen];
	avgSX = new float[trackLen*trackLen];
	avgSY = new float[trackLen*trackLen];
	for(int n = 0; n < trackLen; ++n) shiftsX[n] = shiftsY[n] = 0;
	lastFT = 0;
	dim = 1;
	for(int x = 0; x < trackLen; ++x)
	{
		ffts[x] = new CFTools();
		ffts[x]->fft1Dinit(size);
	}
}
void CFTracker::init2D(int sizeX, int sizeY, int trackerLength)
{
	trackLen = trackerLength;
	if(trackLen < 1) trackLen = 1;
	if(trackLen > 100) trackLen = 100;
	ffts = new CFTools*[trackLen];
	shiftsX = new float[trackLen*trackLen];
	shiftsY = new float[trackLen*trackLen];
	shiftsR = new float[trackLen*trackLen];
	avgSX = new float[trackLen*trackLen];
	avgSY = new float[trackLen*trackLen];
	for(int n = 0; n < trackLen*trackLen; ++n) shiftsX[n] = shiftsY[n] = 0;
	lastFT = 0;
	dim = 2;
	for(int x = 0; x < trackLen; ++x)
	{
		ffts[x] = new CFTools();
		ffts[x]->fft2Dinit(sizeX, sizeY);
	}
}

CFTools* CFTracker::getFToolN(int n)
{
	int tn = lastFT-n;
	if(tn < 0) tn += trackLen;
	return ffts[tn];
}

void CFTracker::dimFFT(float *data_r, float *data_i, int num)
{
	if(dim == 1) ffts[num]->fft1D(data_r, data_i);
	if(dim == 2) ffts[num]->fft2D(data_r, data_i);
};

void CFTracker::addFrame(float *data_r, float *data_i)
{
	lastFT++;
	if(lastFT >= trackLen) lastFT = 0;
	dimFFT(data_r, data_i, lastFT);
	memcpy(shiftsX+trackLen, shiftsX, trackLen*(trackLen-1)*sizeof(float));
	memcpy(shiftsY+trackLen, shiftsY, trackLen*(trackLen-1)*sizeof(float));
	memcpy(shiftsR+trackLen, shiftsR, trackLen*(trackLen-1)*sizeof(float));
	shiftsX[0] = 0;
	shiftsY[0] = 0;
	shiftsR[0] = 1;
	for(int px = 1; px < trackLen; ++px)
	{
		int compN = lastFT - px;
		if(compN < 0) compN += trackLen;
		shiftsX[px] = 0;
		shiftsY[px] = 0;
		if(dim == 1)
		{
			ffts[lastFT]->storeSpectr1D();
			ffts[lastFT]->storeData1D();
			ffts[lastFT]->calcPhaseCorr1D(ffts[compN]);
			float *pc_r = ffts[lastFT]->getPhaseCorr1D_r();
			float *pc_i = ffts[lastFT]->getPhaseCorr1D_i();
			float maxValL = 0;
			int maxX = 0;
			int sizeX = ffts[lastFT]->getSpctSZX();
			for(int x = 0; x < sizeX; ++x)
			{
				float val = sqrt(pc_r[x]*pc_r[x] + pc_i[x]*pc_i[x]);
				if(val > maxValL)
				{
					maxValL = val;
					maxX = x;
				}
			}
			if(maxX > sizeX/2) maxX = maxX - sizeX;
			shiftsX[px] = maxX;
			shiftsY[px] = 0;
			shiftsR[px] = 1;
			ffts[lastFT]->restoreSpectr1D();
			ffts[lastFT]->restoreData1D();
		}
		if(dim == 2)
		{
			ffts[lastFT]->storeSpectr2D();
			ffts[lastFT]->storeData2D();
			ffts[lastFT]->calcPhaseCorr2D(ffts[compN]);
			float *pc_r = ffts[lastFT]->getPhaseCorr2D_r();
			float *pc_i = ffts[lastFT]->getPhaseCorr2D_i();
			float maxValL = 0;
			int maxX = 0, maxY = 0;
			int sizeX = ffts[lastFT]->getSpctSZX();
			int sizeY = ffts[lastFT]->getSpctSZY();
			float avgVal = 0, avgZ = 0;
			for(int x = 0; x < sizeX; ++x)
				for(int y = 0; y < sizeY; ++y)
			{
				int cidx = y*sizeX + x;
//				float val = sqrt(pc_r[cidx]*pc_r[cidx] + pc_i[cidx]*pc_i[cidx]);
				float val = (pc_r[cidx]*pc_r[cidx] + pc_i[cidx]*pc_i[cidx]);
				avgVal += val;
				avgZ++;
				if(val > maxValL)
				{
					maxValL = val;
					maxX = x;
					maxY = y;
				}
			}
			avgVal = sqrt(avgVal / avgZ);
			int cnt095 = 0;
			int cnt08 = 0;
			int cnt05 = 0;
			float secondMax = 0;
			for(int x = 0; x < sizeX*sizeY; ++x)
			{
				float val = (pc_r[x]*pc_r[x] + pc_i[x]*pc_i[x]);
				if(val > secondMax && val < maxValL) secondMax = val;
				if(val > 0.95*maxValL) cnt095++;
				if(val > 0.8*maxValL) cnt08++;
				if(val > 0.5*maxValL) cnt05++;
			}
			if(maxX > sizeX/2) maxX = maxX - sizeX;
			if(maxY > sizeY/2) maxY = maxY - sizeY;
			if(cnt095 > 3) maxX = maxY = 0;
			if(cnt08 > 15) maxX = maxY = 0;
			if(cnt05 > sizeX*2) maxX = maxY = 0;
			shiftsX[px] = maxX;
			shiftsY[px] = maxY;
			if(maxValL > 0)
//				shiftsR[px] = 1.0 - (secondMax / maxValL)*(secondMax / maxValL);
				shiftsR[px] = 1.0 - avgVal / sqrt(maxValL);
			else
				shiftsR[px] = 0;
			shiftsR[px] *= shiftsR[px];
			shiftsR[px] *= shiftsR[px];
//			printf("trk2d: px %d, lastFT %d, compN %d, mx %d, my %d, val %g\n", px, lastFT, compN, maxX, maxY, maxValL);
			ffts[lastFT]->restoreSpectr2D();
			ffts[lastFT]->restoreData2D();
			return;
		}
	}
}
void shellSort(float *arr, int len) 
{
	int H[3]={7,3,1};
	int Hn = 3;
	int h, i, j;
	for (h = 0; h < Hn; ++h) 
	{
		int st = H[h];
		for (i = st; i < len; i++) 
		{
			float t = arr[i];
			for (j = i; j >= st && t < arr[j - st]; j -= st) 
			{
				arr[j] = arr[j-st];
			}
			arr[j] = t;
		}
	}
}
float getMedian(float *arr, int len)
{
	if(len < 2) return arr[0];
	float *tmp = new float[len];
	for(int x = 0; x < len; ++x)
		tmp[x] = arr[x];
	shellSort(tmp, len);
	float ret = 0;
	if(len%2 == 0)
		ret = 0.5*(tmp[len/2-1]+tmp[len/2]);
	else
		ret = 0.33333*(tmp[len/2-1] + tmp[len/2]+tmp[len/2+1]);
	delete[] tmp;
	return ret;
}
void CFTracker::filterShifts()
{
	curDX = 0;
	curDY = 0;
	for(int x = 0; x < trackLen*trackLen; ++x)
	{
		avgSX[x] = shiftsX[x];
		avgSY[x] = shiftsY[x];
	}
	for(int r = trackLen-2; r >= 0; --r)
	{
		for(int c = trackLen-2; c >= 1; --c)
		{
			int idx = r*trackLen + c;
			int idxP = r*trackLen + c + 1;
			int idxM = (r+1)*trackLen + c;
			float avgM = shiftsR[idx] / (shiftsR[idxM] + shiftsR[idxP] + 0.001);//(float)(r+1) / (float)trackLen;
			if(avgM > 1) avgM = 1;
			avgSX[idx] *= avgM;
			avgSY[idx] += (1.0 - avgM)*(avgSX[idxP] - avgSX[idxM]);
			avgSY[idx] *= avgM;
			avgSY[idx] += (1.0 - avgM)*(avgSX[idxP] - avgSX[idxM]);
		}
	}
	float curZ = 0.0001;
	for(int r = 0; r <trackLen-1; ++r)
	{
		float minR = shiftsR[r];
		if(shiftsR[r+1] < minR) minR = shiftsR[r+1];
		curDX += (shiftsX[r+1] - shiftsX[r]) * minR;
		curDY += (shiftsY[r+1] - shiftsY[r]) * minR;
		curZ += minR;
	}
	curDX /= curZ;
	curDY /= curZ;
}
float CFTracker::getFilteredDX()
{
	filterShifts();
	return curDX;
}
float CFTracker::getFilteredDY()
{
	filterShifts();
	return curDY;
}*/

