#include "fft_opt.h"

//const float exp_table[128] = {
const float exp_table[130] = {
    1.000000000f,  0.000000000f,
    0.995184727f,  0.098017140f,
    0.980785280f,  0.195090322f,
    0.956940336f,  0.290284677f,
    0.923879533f,  0.382683432f,
    0.881921264f,  0.471396737f,
    0.831469612f,  0.555570233f,
    0.773010453f,  0.634393284f,
    0.707106781f,  0.707106781f,
    0.634393284f,  0.773010453f,
    0.555570233f,  0.831469612f,
    0.471396737f,  0.881921264f,
    0.382683432f,  0.923879533f,
    0.290284677f,  0.956940336f,
    0.195090322f,  0.980785280f,
    0.098017140f,  0.995184727f,
    0.000000000f,  1.000000000f,
   -0.098017140f,  0.995184727f,
   -0.195090322f,  0.980785280f,
   -0.290284677f,  0.956940336f,
   -0.382683432f,  0.923879533f,
   -0.471396737f,  0.881921264f,
   -0.555570233f,  0.831469612f,
   -0.634393284f,  0.773010453f,
   -0.707106781f,  0.707106781f,
   -0.773010453f,  0.634393284f,
   -0.831469612f,  0.555570233f,
   -0.881921264f,  0.471396737f,
   -0.923879533f,  0.382683432f,
   -0.956940336f,  0.290284677f,
   -0.980785280f,  0.195090322f,
   -0.995184727f,  0.098017140f,
   -1.000000000f,  0.000000000f,
   -0.995184727f, -0.098017140f,
   -0.980785280f, -0.195090322f,
   -0.956940336f, -0.290284677f,
   -0.923879533f, -0.382683432f,
   -0.881921264f, -0.471396737f,
   -0.831469612f, -0.555570233f,
   -0.773010453f, -0.634393284f,
   -0.707106781f, -0.707106781f,
   -0.634393284f, -0.773010453f,
   -0.555570233f, -0.831469612f,
   -0.471396737f, -0.881921264f,
   -0.382683432f, -0.923879533f,
   -0.290284677f, -0.956940336f,
   -0.195090322f, -0.980785280f,
   -0.098017140f, -0.995184727f,
   -0.000000000f, -1.000000000f,
    0.098017140f, -0.995184727f,
    0.195090322f, -0.980785280f,
    0.290284677f, -0.956940336f,
    0.382683432f, -0.923879533f,
    0.471396737f, -0.881921264f,
    0.555570233f, -0.831469612f,
    0.634393284f, -0.773010453f,
    0.707106781f, -0.707106781f,
    0.773010453f, -0.634393284f,
    0.831469612f, -0.555570233f,
    0.881921264f, -0.471396737f,
    0.923879533f, -0.382683432f,
    0.956940336f, -0.290284677f,
    0.980785280f, -0.195090322f,
    0.995184727f, -0.098017140f,
	0, 0
};

void fft_radix8_butterfly_64(float *buf_r, float *buf_i)
{
	uint32_t ia1, ia2, ia3, ia4, ia5, ia6, ia7;
	uint32_t i1, i2, i3, i4, i5, i6, i7, i8;
	uint32_t id;
	uint32_t n2, j;

	float r1, r2, r3, r4, r5, r6, r7, r8;
	float t1, t2;
	float s1, s2, s3, s4, s5, s6, s7, s8;
	float p1, p2, p3, p4;
	float cos2, cos3, cos4, cos5, cos6, cos7, cos8;
	float sin2, sin3, sin4, sin5, sin6, sin7, sin8;
	const float C81 = 0.70710678118f;
	
	int coeff_shift = 1;

	int fft_len = 64; //fixed length function
	n2 = fft_len; 

	while(1) //break condition later
	{
		n2 = n2 >> 3; //if n2 >= 8 - repeat
		i1 = 0;

		i2 = i1 + n2;
		i3 = i2 + n2;
		i4 = i3 + n2;
		i5 = i4 + n2;
		i6 = i5 + n2;
		i7 = i6 + n2;
		i8 = i7 + n2;
		r1 = buf_r[i1] + buf_r[i5];
		r5 = buf_r[i1] - buf_r[i5];
		r2 = buf_r[i2] + buf_r[i6];
		r6 = buf_r[i2] - buf_r[i6];
		r3 = buf_r[i3] + buf_r[i7];
		r7 = buf_r[i3] - buf_r[i7];
		r4 = buf_r[i4] + buf_r[i8];
		r8 = buf_r[i4] - buf_r[i8];
		t1 = r1 - r3;
		r1 = r1 + r3;
		r3 = r2 - r4;
		r2 = r2 + r4;
		buf_r[i1] = r1 + r2;   
		buf_r[i5] = r1 - r2;
		r1 = buf_i[i1] + buf_i[i5];
		s5 = buf_i[i1] - buf_i[i5];
		r2 = buf_i[i2] + buf_i[i6];
		s6 = buf_i[i2] - buf_i[i6];
		s3 = buf_i[i3] + buf_i[i7];
		s7 = buf_i[i3] - buf_i[i7];
		r4 = buf_i[i4] + buf_i[i8];
		s8 = buf_i[i4] - buf_i[i8];
		t2 = r1 - s3;
		r1 = r1 + s3;
		s3 = r2 - r4;
		r2 = r2 + r4;
		buf_i[i1] = r1 + r2;
		buf_i[i5] = r1 - r2;
		buf_r[i3] = t1 + s3;
		buf_r[i7] = t1 - s3;
		buf_i[i3] = t2 - r3;
		buf_i[i7] = t2 + r3;
		r1 = (r6 - r8) * C81;
		r6 = (r6 + r8) * C81;
		r2 = (s6 - s8) * C81;
		s6 = (s6 + s8) * C81;
		t1 = r5 - r1;
		r5 = r5 + r1;
		r8 = r7 - r6;
		r7 = r7 + r6;
		t2 = s5 - r2;
		s5 = s5 + r2;
		s8 = s7 - s6;
		s7 = s7 + s6;
		buf_r[i2] = r5 + s7;
		buf_r[i8] = r5 - s7;
		buf_r[i6] = t1 + s8;
		buf_r[i4] = t1 - s8;
		buf_i[i2] = s5 - r7;
		buf_i[i8] = s5 + r7;
		buf_i[i6] = t2 - r8;
		buf_i[i4] = t2 + r8;

		if(n2 < 8) break; //finished

		ia1 = 0;
		j = 1;

		do
		{	
			id = ia1 + coeff_shift;
			ia1 = id;
			ia2 = ia1 + id;
			ia3 = ia2 + id;
			ia4 = ia3 + id;
			ia5 = ia4 + id;
			ia6 = ia5 + id;
			ia7 = ia6 + id;

			cos2 = exp_table[2 * ia1];
			cos3 = exp_table[2 * ia2];
			cos4 = exp_table[2 * ia3];
			cos5 = exp_table[2 * ia4];
			cos6 = exp_table[2 * ia5];
			cos7 = exp_table[2 * ia6];
			cos8 = exp_table[2 * ia7];
			sin2 = exp_table[2 * ia1 + 1];
			sin3 = exp_table[2 * ia2 + 1];
			sin4 = exp_table[2 * ia3 + 1];
			sin5 = exp_table[2 * ia4 + 1];
			sin6 = exp_table[2 * ia5 + 1];
			sin7 = exp_table[2 * ia6 + 1];
			sin8 = exp_table[2 * ia7 + 1];         

			i1 = j;

			i2 = i1 + n2;
			i3 = i2 + n2;
			i4 = i3 + n2;
			i5 = i4 + n2;
			i6 = i5 + n2;
			i7 = i6 + n2;
			i8 = i7 + n2;
			r1 = buf_r[i1] + buf_r[i5];
			r5 = buf_r[i1] - buf_r[i5];
			r2 = buf_r[i2] + buf_r[i6];
			r6 = buf_r[i2] - buf_r[i6];
			r3 = buf_r[i3] + buf_r[i7];
			r7 = buf_r[i3] - buf_r[i7];
			r4 = buf_r[i4] + buf_r[i8];
			r8 = buf_r[i4] - buf_r[i8];
			t1 = r1 - r3;
			r1 = r1 + r3;
			r3 = r2 - r4;
			r2 = r2 + r4;
			buf_r[i1] = r1 + r2;
			r2 = r1 - r2;
			s1 = buf_i[i1] + buf_i[i5];
			s5 = buf_i[i1] - buf_i[i5];
			s2 = buf_i[i2] + buf_i[i6];
			s6 = buf_i[i2] - buf_i[i6];
			s3 = buf_i[i3] + buf_i[i7];
			s7 = buf_i[i3] - buf_i[i7];
			s4 = buf_i[i4] + buf_i[i8];
			s8 = buf_i[i4] - buf_i[i8];
			t2 = s1 - s3;
			s1 = s1 + s3;
			s3 = s2 - s4;
			s2 = s2 + s4;
			r1 = t1 + s3;
			t1 = t1 - s3;
			buf_i[i1] = s1 + s2;
			s2 = s1 - s2;
			s1 = t2 - r3;
			t2 = t2 + r3;
			p1 = cos5 * r2;
			p2 = sin5 * s2;
			p3 = cos5 * s2;
			p4 = sin5 * r2;
			buf_r[i5] = p1 + p2;
			buf_i[i5] = p3 - p4;
			p1 = cos3 * r1;
			p2 = sin3 * s1;
			p3 = cos3 * s1;
			p4 = sin3 * r1;
			buf_r[i3] = p1 + p2;
			buf_i[i3] = p3 - p4;
			p1 = cos7 * t1;
			p2 = sin7 * t2;
			p3 = cos7 * t2;
			p4 = sin7 * t1;
			buf_r[i7] = p1 + p2;
			buf_i[i7] = p3 - p4;
			r1 = (r6 - r8) * C81;
			r6 = (r6 + r8) * C81;
			s1 = (s6 - s8) * C81;
			s6 = (s6 + s8) * C81;
			t1 = r5 - r1;
			r5 = r5 + r1;
			r8 = r7 - r6;
			r7 = r7 + r6;
			t2 = s5 - s1;
			s5 = s5 + s1;
			s8 = s7 - s6;
			s7 = s7 + s6;
			r1 = r5 + s7;
			r5 = r5 - s7;
			r6 = t1 + s8;
			t1 = t1 - s8;
			s1 = s5 - r7;
			s5 = s5 + r7;
			s6 = t2 - r8;
			t2 = t2 + r8;
			p1 = cos2 * r1;
			p2 = sin2 * s1;
			p3 = cos2 * s1;
			p4 = sin2 * r1;
			buf_r[i2] = p1 + p2;
			buf_i[i2] = p3 - p4;
			p1 = cos8 * r5;
			p2 = sin8 * s5;
			p3 = cos8 * s5;
			p4 = sin8 * r5;
			buf_r[i8] = p1 + p2;
			buf_i[i8] = p3 - p4;
			p1 = cos6 * r6;
			p2 = sin6 * s6;
			p3 = cos6 * s6;
			p4 = sin6 * r6;
			buf_r[i6] = p1 + p2;
			buf_i[i6] = p3 - p4;
			p1 = cos4 * t1;
			p2 = sin4 * t2;
			p3 = cos4 * t2;
			p4 = sin4 * t1;
			buf_r[i4] = p1 + p2;
			buf_i[i4] = p3 - p4;

			j++;
		} while(j < n2);
//		coeff_shift <<= 3;
	}   
}
