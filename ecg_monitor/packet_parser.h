#include <stdint.h>

typedef struct uECG_data
{
	uint8_t mac[6]; //BLE mode only
	uint32_t ID;
	uint8_t protocol_version;
	uint8_t last_pack_id;
	int battery_mv;
	int bpm;
	int sdnn;
	int hrv_percent[32];
	int rr_id;
	int rr_current;
	int rr_prev;
	int skin_res;
	int steps;
	int acc_x;
	int acc_y;
	int acc_z;
	int RR_data[128];
}uECG_data;

enum param_sends
{
	param_batt_bpm = 0,
	param_sdnn,
	param_skin_res,
	param_lastRR,
	param_imu_acc,
	param_imu_steps,
	param_pnn_bins,
	param_end,
	param_emg_spectrum
};

float decode_acc(float acc);
int parse_ble_packet(uint8_t *pack, int maxlen, uECG_data *result);
