package com.ultimaterobotics.uecgmonitor4_2;


public class uecg_ble_parser {

    public class uecg_parsed_state {
        public int pack_id;
        public int BPM;
        public int batt;
        public int SDNN;
        public int RMSSD;
        public int GSR;
        public float ax;
        public float ay;
        public float az;
        public int steps;
        public int g_value;
        public int RR_cur;
        public int RR_prev;
        public int RR_id;

        public int pNN_bin1;
        public int pNN_bin2;
        public int pNN_bin3;

        public int[] ECG_values = new int[32];
        public int ECG_values_count = 0;

        public boolean has_BPM = false;
        public boolean has_batt = false;
        public boolean has_SDNN = false;
        public boolean has_RMSSD = false;
        public boolean has_GSR = false;
        public boolean has_accel = false;
        public boolean has_steps = false;
        public boolean has_g = false;
        public boolean has_RR = false;
        public int pNN_bin1_id = -1;
        public int pNN_bin2_id = -1;
        public int pNN_bin3_id = -1;

        void clear_vals() {
            has_BPM = false;
            has_batt = false;
            has_SDNN = false;
            has_RMSSD = false;
            has_GSR = false;
            has_accel = false;
            has_steps = false;
            has_g = false;
            has_RR = false;
            pNN_bin1_id = -1;
            pNN_bin2_id = -1;
            pNN_bin3_id = -1;
        }
    }

    int[] in_buf = new int[50];
    public int version_id = -1;

    public uecg_parsed_state parsed_state = new uecg_parsed_state();

    public int parse_record(byte[] scanRecord)
    {
        int param_batt_bpm = 0;
        int param_sdnn = 1;
        int param_skin_res = 2;
        int param_lastRR = 3;
        int param_imu_acc = 4;
        int param_imu_steps = 5;
        int param_pnn_bins = 6;
        int param_end = 7;
        int param_emg_spectrum = 8;

        int field_id = scanRecord[1];
        if(field_id < 0)
            field_id = 256 + field_id;
        if(field_id == 255 || field_id == 8)
        {
            parsed_state.clear_vals();

            if(field_id == 255) {
                for (int n = 1; n < 35; n++) {
                    in_buf[n - 1] = scanRecord[n];
                    if (in_buf[n - 1] < 0)
                        in_buf[n - 1] = 256 + in_buf[n - 1];
                }
            }
            else {
                for (int n = 6; n < 35; n++) {
                    in_buf[n - 6] = scanRecord[n];
                    if (in_buf[n - 6] < 0)
                        in_buf[n - 6] = 256 + in_buf[n - 6];
                }
            }
            parsed_state.pack_id = in_buf[1];
            int param_id = in_buf[2] & 0x0F;
            int param_mod = (in_buf[2]>>4) & 0x0F;
            int param_b1 = in_buf[3];
            int param_b2 = in_buf[4];
            int param_b3 = in_buf[5];
            if(param_id == param_batt_bpm)
            {
                parsed_state.batt = param_b1;
                parsed_state.has_batt = true;
                if(version_id < 0)
                    {
                        if(param_b2 >= 3 && param_b2 < 20)
                            version_id = param_b2;
                        else
                            version_id = 2;
                    }
                parsed_state.BPM = param_b3;
                parsed_state.has_BPM = true;
                //fix of wrong timing on the device side
                if(version_id < 5) parsed_state.BPM = (int)( (double)(parsed_state.BPM) / 1.27);
            }
            if(param_id == param_sdnn)
            {
                int v1_h = param_b1>>4;
                int v2_h = param_b1&0x0F;
                int v1_l = param_b2;
                int v2_l = param_b3;
                int v1 = (v1_h<<8) | v1_l;
                int v2 = (v2_h<<8) | v2_l;
                parsed_state.SDNN = v1;
                parsed_state.RMSSD = v2;
                parsed_state.has_SDNN = true;
                parsed_state.has_RMSSD = true;
            }
            if(param_id == param_skin_res)
            {
                parsed_state.GSR = param_b1 * 256 + param_b2;
                if(parsed_state.GSR > 32767)
                    parsed_state.GSR = -65536 + parsed_state.GSR;
                parsed_state.has_GSR = true;
            }
            if(param_id == param_imu_acc)
            {
                parsed_state.ax = (param_b1 - 128.0f) / 4.0f;
                parsed_state.ay = (param_b2 - 128.0f) / 4.0f;
                parsed_state.az = (param_b3 - 128.0f) / 4.0f;
                parsed_state.has_accel = true;
            }
            if(param_id == param_imu_steps)
            {
                int steps = param_b1*256 + param_b2;
                parsed_state.steps = steps; //need to add overflow logic here
                parsed_state.g_value = param_b3;
                parsed_state.has_steps = true;
                parsed_state.has_g = true;
            }
            if(param_id == param_pnn_bins) {
                int pnn_bin = param_mod;
                parsed_state.pNN_bin1 = param_b1;
                parsed_state.pNN_bin1_id = pnn_bin + 0;
                parsed_state.pNN_bin2 = param_b2;
                parsed_state.pNN_bin2_id = pnn_bin + 1;
                parsed_state.pNN_bin3 = param_b3;
                parsed_state.pNN_bin3_id = pnn_bin + 2;
            }

            if(param_id == param_lastRR)
            {
                int v1_h = param_b1>>4;
                int v2_h = param_b1&0x0F;
                int v1_l = param_b2;
                int v2_l = param_b3;
                int v1 = (v1_h<<8) | v1_l;
                int v2 = (v2_h<<8) | v2_l;

                if(version_id < 5) { //fix of wrong timing on the device side
                    v1 = (int)( (double)(v1) / 1.27);
                    v2 = (int)( (double)(v1) / 1.27);
                }

                parsed_state.RR_cur = v1;
                parsed_state.RR_prev = v2;
                parsed_state.RR_id = param_mod;
                parsed_state.has_RR = true;
            }
            int pack_data_count = 17;

            if(version_id == 4 || version_id == 5)
            {
                pack_data_count = 17;
                if(field_id == 255) pack_data_count = 23;
                parsed_state.ECG_values_count = pack_data_count;
                byte check = 0;
                int cur_v = in_buf[6] * 256 + in_buf[7];
                if (cur_v > 32767)
                    cur_v = -65536 + cur_v;
                int scale_v = in_buf[8];
                if(scale_v > 100) scale_v = 100 + (scale_v - 100)*4;
                parsed_state.ECG_values[0] = cur_v;
                for (int x = 1; x < pack_data_count; x++) {
                    int dv = in_buf[8 + x];
                    if (dv > 127) dv = -256 + dv;
                    cur_v += dv * scale_v;
                    parsed_state.ECG_values[x] = cur_v;
                }
            }
            return  1; //parsed successfully
        }
        return  0; //wrong input data
    };
}
