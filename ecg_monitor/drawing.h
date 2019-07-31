
#include "widget_list.h"
#include <stdint.h>
#include "simplechart.h"
#include "spectrogram.h"

CSimpleChart acc_charts[3];
CSimpleChart gyro_charts[3];
CSimpleChart mag_charts[3];


CSimpleChart ecg_charts[8];
CSimpleChart stress_charts[8];
CSimpleChart stress_hist[8];

SSpectrogramView emg_c;
SSpectrogramView *emg_chart;

SSpectrogramView emg_charts[3];

int draw_spectr;
int draw_emg;

void draw_loop();