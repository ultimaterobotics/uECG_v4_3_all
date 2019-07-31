#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

//#ifndef SIMPLECHART__H
//#define SIMPLECHART__H

#include <stddef.h>
#include <stdint.h>
#include <math.h>

int strEq(const char *inputText1, const char *inputText2);

typedef struct CSimpleChart
{
//private:
	int type; // 0 - normal, 1 - xy
	int dataSize;
	float *data;
	float *dataX; //for xy chart
	int dataPos;
	float scale;
	float mScale;
	float zeroV;
	float curMin;
	float curMax;
	int scalingType;
	int drawAxis;
	int color;
	int axisColor;
	int DX, DY;
	int SX, SY;
}CSimpleChart;
void sc_updateScaling(CSimpleChart *chart);
float sc_normVal(CSimpleChart *chart, float normPos);
//public:
void sc_create_simple_chart(CSimpleChart *chart, int length, int type);
//	CSimpleChart(int length);
//	~CSimpleChart();
void sc_setViewport(CSimpleChart *chart, int dx, int dy, int sizeX, int sizeY);
void sc_setParameter_cc(CSimpleChart *chart, const char *name, const char *value);
void sc_setParameter_cv(CSimpleChart *chart, const char *name, float value);
void sc_setParameter_cl(CSimpleChart *chart, const char *name, int r, int g, int b);
void sc_clear(CSimpleChart *chart);
void sc_addV(CSimpleChart *chart, float v);
float sc_getV(CSimpleChart *chart, int hist_depth);

void sc_draw(CSimpleChart *chart, GdkDrawable *drawable, GdkGC *gc, int w, int h);
void sc_draw_ln(CSimpleChart *chart, GdkDrawable *drawable, GdkGC *gc, int w, int h);
float sc_getMin(CSimpleChart *chart);
float sc_getMax(CSimpleChart *chart);
int sc_getX(CSimpleChart *chart);
int sc_getY(CSimpleChart *chart);
int sc_getSizeX(CSimpleChart *chart);
int sc_getSizeY(CSimpleChart *chart);
int sc_getColor(CSimpleChart *chart);
int sc_getDataSize(CSimpleChart *chart);
float sc_getMean(CSimpleChart *chart);
float sc_getSDV(CSimpleChart *chart);

//#endif