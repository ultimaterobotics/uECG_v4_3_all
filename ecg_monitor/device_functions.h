#include <stdint.h>

void device_parse_response(uint8_t *buf, int len);

float device_get_rssi();
float device_get_battery();

int device_get_bpm();
int device_get_skin_res();

int device_get_mode();
int get_ble_draw_mode();

void device_toggle_ble_mode();
void device_toggle_stress_mode();
void device_toggle_stress_hist();
void device_toggle_file_save();
void device_close_log_file();

void device_draw_time();
void device_draw_phase();
void device_draw_centered();


float device_get_ax();
float device_get_ay();
float device_get_az();
int device_get_steps();
