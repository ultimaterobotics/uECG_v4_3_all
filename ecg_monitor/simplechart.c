#include "simplechart.h"

int strEq(const char *inputText1, const char *inputText2)
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

void sc_updateScaling(CSimpleChart *chart)
{
	if(chart->scalingType == 0)
	{
		if(chart->scale != 0) chart->mScale = 1.0 / chart->scale;
		float dmin = chart->data[0], dmax = dmin;
		for(int x = 0; x < chart->dataSize; ++x)
		{
			if(chart->data[x] > dmax) dmax = chart->data[x];
			if(chart->data[x] < dmin) dmin = chart->data[x];
		}
		if(dmin == dmax) {dmin -= 0.5; dmax += 0.5; };
		chart->curMin = dmin;
		chart->curMax = dmax;
	}
	if(chart->scalingType == 1)
	{
		float dmin = chart->data[0], dmax = dmin;
		for(int x = 0; x < chart->dataSize; ++x)
		{
			if(chart->data[x] > dmax) dmax = chart->data[x];
			if(chart->data[x] < dmin) dmin = chart->data[x];
		}
		if(dmax == dmin)
		{
			dmax = dmin+0.00001;
			dmin -= 0.00001;
		}
		chart->curMin = dmin;
		chart->curMax = dmax;
		if(dmin == dmax) {dmin -= 0.5; dmax += 0.5; };
		chart->zeroV = 0.5*(dmax+dmin);
		chart->scale = dmax - dmin;
		chart->mScale = 1.0 / chart->scale;
	}
	if(chart->scalingType == 2)
	{
		float dmin = chart->data[0], dmax = dmin;
		for(int x = 0; x < chart->dataSize; ++x)
		{
			if(chart->data[x] > dmax) dmax = chart->data[x];
			if(chart->data[x] < dmin) dmin = chart->data[x];
		}
		if(dmin == dmax) {dmin -= 0.5; dmax += 0.5; };
		chart->curMin = dmin;
		chart->curMax = dmax;
		chart->zeroV = 0.5*(dmax+dmin);
	}
}

float sc_normVal(CSimpleChart *chart, float normPos)
{
	float rpos = normPos*chart->dataSize;
	int i1 = rpos;
	int i2 = i1+1;
	float c2 = rpos - i1, c1 = i2-rpos;
	int ri1 = chart->dataPos - i1; if(ri1 < 0) ri1 += chart->dataSize;
	int ri2 = chart->dataPos - i2; if(ri2 < 0) ri2 += chart->dataSize;
	return (chart->data[ri1]*c1 + chart->data[ri2]*c2  - chart->zeroV) * chart->mScale;
}

void sc_create_simple_chart(CSimpleChart *chart, int length, int type)
{
	chart->dataSize = length;
	if(chart->dataSize < 1) chart->dataSize = 1;
	chart->data = (float*)malloc(chart->dataSize*sizeof(float));
	for(int x = 0; x < chart->dataSize; ++x) chart->data[x] = 0;
	if(type == 1)
	{
		chart->dataX = (float*)malloc(chart->dataSize*sizeof(float));
		for(int x = 0; x < chart->dataSize; ++x) chart->dataX[x] = 0;
	}
	chart->dataPos = 0;

	chart->scalingType = 0;
	chart->zeroV = 0;
	chart->scale = 1;
	chart->drawAxis = 1;
	chart->color = 0xFF;
	chart->axisColor = 0xFFFFFF;
}

void sc_setViewport(CSimpleChart *chart, int dx, int dy, int sizeX, int sizeY)
{
	chart->DX = dx;
	chart->DY = dy;
	chart->SX = sizeX;
	chart->SY = sizeY;
}
void sc_setParameter_cc(CSimpleChart *chart, const char *name, const char *value)
{
	if(strEq(name, "draw axis")) chart->drawAxis = strEq(value, "yes");
	if(strEq(name, "scaling"))
	{
		if(strEq(value, "manual")) chart->scalingType = 0; 
		if(strEq(value, "auto")) chart->scalingType = 1; 
		if(strEq(value, "follow center")) chart->scalingType = 2; 
	}
}
void sc_setParameter_cv(CSimpleChart *chart, const char *name, float value)
{
	if(strEq(name, "zero value")) chart->zeroV = value;
	if(strEq(name, "scale")) chart->scale = value;
}
void sc_setParameter_cl(CSimpleChart *chart, const char *name, int r, int g, int b)
{
	if(strEq(name, "axis color")) chart->axisColor = (r<<16) + (g<<8) + b;
	if(strEq(name, "color")) chart->color = (r<<16) + (g<<8) + b;
}
void sc_clear(CSimpleChart *chart)
{
	for(int x = 0; x < chart->dataSize; ++x) chart->data[x] = 0;
}
void sc_addV(CSimpleChart *chart, float v)
{
	if(chart->type == 1) return;
	chart->data[chart->dataPos++] = v;
	if(chart->dataPos >= chart->dataSize) chart->dataPos = 0;
}
void sc_addVXY(CSimpleChart *chart, float v, float vx)
{
	if(chart->type != 1) return;
	chart->data[chart->dataPos] = v;
	chart->dataX[chart->dataPos++] = vx;
	if(chart->dataPos >= chart->dataSize) chart->dataPos = 0;
}
float sc_getV(CSimpleChart *chart, int hist_depth)
{
	int p = chart->dataPos - hist_depth;
	while(p < 0) p += chart->dataSize;
	return chart->data[p];
}
float sc_getVX(CSimpleChart *chart, int hist_depth)
{
	if(chart->type != 1) return 0;
	int p = chart->dataPos - hist_depth;
	while(p < 0) p += chart->dataSize;
	return chart->dataX[p];
}
GdkPoint *points_buf;
int points_buf_ready = 0;
int points_buf_size = 0;

void sc_draw_ln(CSimpleChart *chart, GdkDrawable *drawable, GdkGC *gc, int w, int h)
{
	if(chart->type == 1)
	{
//		sc_drawXY(chart, drawable, gc, w, h);
		return;
	}
	sc_updateScaling(chart);
	if(!points_buf_ready)
	{
		points_buf_size = (200*chart->SX + 100);
		points_buf = (GdkPoint*)malloc(points_buf_size*sizeof(GdkPoint));
		points_buf_ready = 1;
	}
	float mSX = 1.0 / (float)(chart->SX-1);
	GdkColor clr;
	clr.pixel = chart->color;
	gdk_gc_set_foreground(gc, &clr);	
	int points_cnt = 0;
//	float spstep = (float)chart->dataSize / (float)chart->SX;
//	if(spstep > 0.1) spstep = 0.1;
//	if(spstep < 0.1) spstep = 0.1;
	for(int x = 0; x < chart->SX; ++x)
	{
		float rp = (float)x; rp *= mSX;
		float v = sc_normVal(chart, rp);
		if(x > chart->SX - 10)
		{
			; 
//			printf("%d -> %g %g\n", x, rp, v);
		}
		if(v > 1) v = 1;
		if(v < -1) v = -1;
		int xx = chart->DX + chart->SX-1 - x;
		int yy = chart->DY + chart->SY/2 - v*0.5*chart->SY;
		int idx = (yy*w + xx);
		if(xx >= 0 && xx < w && yy >= 0 && yy < h)
		{
			points_buf[points_cnt].x = xx;
			points_buf[points_cnt].y = yy;
			points_cnt++;
//				((unsigned int*)drawPix)[idx] = color;
		}
	}
//	gdk_draw_points(drawable, gc, points_buf, points_cnt);
	gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	if(points_cnt > 0)
		gdk_draw_lines(drawable, gc, points_buf, points_cnt);
	points_cnt = 0;
	clr.pixel = chart->axisColor;
	gdk_gc_set_foreground(gc, &clr);	
	if(chart->drawAxis)
	{
		int xx = chart->DX;
		int yy = chart->DY + chart->SY/2;
		points_buf[points_cnt].x = xx;
		points_buf[points_cnt].y = yy;
		points_cnt++;
		xx = chart->DX + chart->SX;
		points_buf[points_cnt].x = xx;
		points_buf[points_cnt].y = yy;
		points_cnt++;
		gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
		gdk_draw_lines(drawable, gc, points_buf, points_cnt);
	}
}

void sc_draw(CSimpleChart *chart, GdkDrawable *drawable, GdkGC *gc, int w, int h)
{
	sc_updateScaling(chart);
	if(!points_buf_ready)
	{
		points_buf = (GdkPoint*)malloc((13*chart->SX + 100)*sizeof(GdkPoint));
		points_buf_ready = 1;
	}
	float mSX = 1.0 / (float)(chart->SX-1);
	GdkColor clr;
	clr.pixel = chart->color;
	gdk_gc_set_foreground(gc, &clr);	
	int points_cnt = 0;
	float spstep = (float)chart->dataSize / (float)chart->SX;
	if(spstep > 0.1) spstep = 0.1;
	if(spstep < 0.1) spstep = 0.1;
	for(int x = 0; x < chart->SX; ++x)
	{
		for(float sp = 0; sp < 1.0; sp += spstep)
		{
			float rp = (float)x + sp; rp *= mSX;
			float v = sc_normVal(chart, rp);
			if(v > 1) v = 1;
			if(v < -1) v = -1;
			int xx = chart->DX + chart->SX-1 - x;
			int yy = chart->DY + chart->SY/2 - v*0.5*chart->SY;
			int idx = (yy*w + xx);
			if(xx >= 0 && xx < w && yy >= 0 && yy < h)
			{
				points_buf[points_cnt].x = xx;
				points_buf[points_cnt].y = yy;
				points_cnt++;
//				((unsigned int*)drawPix)[idx] = color;
			}
		}
	}
	if(points_cnt > 0)
		gdk_draw_points(drawable, gc, points_buf, points_cnt);
	points_cnt = 0;
	clr.pixel = chart->axisColor;
	gdk_gc_set_foreground(gc, &clr);	
	if(chart->drawAxis)
	{
		for(int x = 0; x < chart->SX; ++x)
		{
			int xx = chart->DX + x;
			int yy = chart->DY + chart->SY/2;
			int idx = (yy*w + xx);
			if(xx >= 0 && xx < w && yy >= 0 && yy < h)
			{
				points_buf[points_cnt].x = xx;
				points_buf[points_cnt].y = yy;
				points_cnt++;
	//			gdk_draw_point(drawable, gc, xx, yy);
	//			((unsigned int*)drawPix)[idx] = axisColor;
	//				drawPix[idx] = axisColor;
			}
		}
		if(points_cnt > 0)
			gdk_draw_points(drawable, gc, points_buf, points_cnt);
	}
}
float sc_getMin(CSimpleChart *chart) {return chart->curMin;}
float sc_getMax(CSimpleChart *chart) {return chart->curMax;}
int sc_getX(CSimpleChart *chart) {return chart->DX;}
int sc_getY(CSimpleChart *chart) {return chart->DY;}
int sc_getSizeX(CSimpleChart *chart) {return chart->SX;}
int sc_getSizeY(CSimpleChart *chart) {return chart->SY;}
int sc_getColor(CSimpleChart *chart) {return chart->color;}
int sc_getDataSize(CSimpleChart *chart) {return chart->dataSize;}
float sc_getMean(CSimpleChart *chart)
{
	float res = 0;
	for(int x = 0; x < chart->dataSize; ++x) res += chart->data[x];
	float z = chart->dataSize;
	z += (chart->dataSize == 0);
	return res/z;
}
float sc_getSDV(CSimpleChart *chart)
{
	float mean = sc_getMean(chart);
	float res = 0;
	for(int x = 0; x < chart->dataSize; ++x)
	{
		float dd = (chart->data[x] - mean);
		res += dd*dd;
	}
	float z = chart->dataSize;
	z += (chart->dataSize == 0);
	return sqrt(res/z);
}
