package com.ultimaterobotics.uecgmonitor4_2;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.Log;

public class BPM_processor {
    public static final int chart_ID_BPM = 0;
    public static final int chart_ID_HRV = 1;
    public static final int chart_ID_GSR = 2;
    public static final int chart_ID_steps = 3;
    public static final int chart_ID_acc = 4;
    public static final int chart_cnt = 5;
    int active_charts = 5;

    class bpm_record
    {
        public int bpm;
        public float gsr;
        public float hrv_param;
        public float step_rate;
        public float ax;
        public float ay;
        public float az;
    };
    bpm_record[] records_fine = new bpm_record[10000];
    bpm_record[] records_coarse = new bpm_record[10000];
    int rec_cnt_fine = 5000;
    int rec_cnt_coarse = 5000;
    int rec_pos_fine = 0;
    int rec_pos_coarse = 0;
    int[] chart_states = new int[chart_cnt];

    int points_cnt = 0;
    int prev_steps = -1;
    float avg_steps = 0;

    float avg_bpm = 80;
    float avg_hrv = 0;
    float avg_gsr = -123456;
    float avg_ax = 0;
    float avg_ay = 0;
    float avg_az = 0;

    float vx, vy, vw, vh;

//    int active_chart = 0;

    int img_width;
    int img_height;

    long start_time = 0;
    long last_write_time_fine = 0;
    long last_write_time_coarse = 0;

    int draw_range = 300;

    public BPM_processor()
    {
        for(int x = 0; x < rec_cnt_fine; x++)
            records_fine[x] = new bpm_record();
        for(int x = 0; x < rec_cnt_coarse; x++)
            records_coarse[x] = new bpm_record();

        for(int x = 0; x < chart_cnt; x++)
            chart_states[x] = 1;
    };

    public int[] pack_int_array()
    {
        int data_start = 10;
        int[] arr = new int[data_start + rec_cnt_fine + rec_cnt_coarse];
        arr[0] = rec_cnt_fine;
        arr[1] = rec_cnt_coarse;
        arr[2] = rec_pos_fine;
        arr[3] = rec_pos_coarse;
        for(int x = 0; x < rec_cnt_fine; x++)
            arr[data_start + x] = records_fine[x].bpm;
        for(int x = 0; x < rec_cnt_coarse; x++)
            arr[data_start + rec_cnt_fine + x] = records_coarse[x].bpm;
        return  arr;
    };
    public float[] pack_float_array()
    {
        int var_cnt = 6;
        float[] arr = new float[(rec_cnt_fine + rec_cnt_coarse) * var_cnt];
        for(int x = 0; x < rec_cnt_fine; x++)
        {
            arr[var_cnt*x + 0] = records_fine[x].step_rate;
            arr[var_cnt*x + 1] = records_fine[x].gsr;
            arr[var_cnt*x + 2] = records_fine[x].hrv_param;
            arr[var_cnt*x + 3] = records_fine[x].ax;
            arr[var_cnt*x + 4] = records_fine[x].ay;
            arr[var_cnt*x + 5] = records_fine[x].az;
        }
        for(int x = 0; x < rec_cnt_coarse; x++)
        {
            arr[var_cnt*(rec_cnt_fine+x) + 0] = records_coarse[x].step_rate;
            arr[var_cnt*(rec_cnt_fine+x) + 1] = records_coarse[x].gsr;
            arr[var_cnt*(rec_cnt_fine+x) + 2] = records_coarse[x].hrv_param;
            arr[var_cnt*(rec_cnt_fine+x) + 3] = records_coarse[x].ax;
            arr[var_cnt*(rec_cnt_fine+x) + 4] = records_coarse[x].ay;
            arr[var_cnt*(rec_cnt_fine+x) + 5] = records_coarse[x].az;
        }
        return  arr;
    };
    public void restore_from_arrays(int[] arr_int, float[] arr_float)
    {
        Log.e("uECG", "array restore: " + arr_int + " " + arr_float);
        int data_start = 10;
        rec_cnt_fine = arr_int[0];
        rec_cnt_coarse = arr_int[1];
        rec_pos_fine = arr_int[2];
        rec_pos_coarse = arr_int[3];
        Log.e("uECG", "vars " + rec_cnt_fine + " " + rec_cnt_coarse + " " + rec_pos_fine + " " + rec_pos_coarse);
        for(int x = 0; x < rec_cnt_fine; x++)
            records_fine[x].bpm = arr_int[data_start + x];
        for(int x = 0; x < rec_cnt_coarse; x++)
            records_coarse[x].bpm = arr_int[data_start + rec_cnt_fine + x];
        int var_cnt = 6;
        for(int x = 0; x < rec_cnt_fine; x++)
        {
            records_fine[x].step_rate = arr_float[var_cnt*x + 0];
            records_fine[x].gsr = arr_float[var_cnt*x + 1];
            records_fine[x].hrv_param = arr_float[var_cnt*x + 2];
            records_fine[x].ax = arr_float[var_cnt*x + 3];
            records_fine[x].ay = arr_float[var_cnt*x + 4];
            records_fine[x].az = arr_float[var_cnt*x + 5];
        }
        for(int x = 0; x < rec_cnt_coarse; x++)
        {
            records_coarse[x].step_rate = arr_float[var_cnt*(rec_cnt_fine+x) + 0];
            records_coarse[x].gsr = arr_float[var_cnt*(rec_cnt_fine+x) + 1];
            records_coarse[x].hrv_param = arr_float[var_cnt*(rec_cnt_fine+x) + 2];
            records_coarse[x].ax = arr_float[var_cnt*(rec_cnt_fine+x) + 3];
            records_coarse[x].ay = arr_float[var_cnt*(rec_cnt_fine+x) + 4];
            records_coarse[x].az = arr_float[var_cnt*(rec_cnt_fine+x) + 5];
        }
    };

    public void set_range(int range)
    {
        draw_range = range;
    };

    public void set_chart_state(int chart_id, int state)
    {
       chart_states[chart_id] = state;
       active_charts = 0;
       for(int x = 0; x < chart_cnt; x++)
           active_charts += chart_states[x];
    };

    public void set_viewport(float pos_x, float pos_y, float width, float height)
    {
        vx = pos_x;
        vy = pos_y;
        vw = width;
        vh = height;
    }

    public void push_data(int BPM, int steps, float gsr, float hrv_parameter, float ax, float ay, float az)
    {
        long tm = System.currentTimeMillis();
        if(tm - last_write_time_fine >= 250)
        {
            if(prev_steps <= 0)
                prev_steps = steps;
            float dst = steps - prev_steps;
            if(dst > 4) dst = 4; //can't be more than 4 steps in a small time, even if it's accumulated error
            if(dst < 0) dst = 0; //step counter overflows sometimes
            prev_steps = steps;
            avg_steps *= 0.85;
            avg_steps += 0.15*dst;

            if(avg_gsr < -123455)
                avg_gsr = gsr;

            float avg_mult = 0.9f;
            float avg_add = 1.0f - avg_mult;
            avg_bpm *= avg_mult;
            avg_gsr *= avg_mult;
            avg_hrv *= avg_mult;
            avg_ax *= avg_mult;
            avg_ay *= avg_mult;
            avg_az *= avg_mult;
            avg_bpm += avg_add * (float)BPM;
            avg_gsr += avg_add * gsr;
            avg_hrv += avg_add * hrv_parameter;
            avg_ax += avg_add*ax;
            avg_ay += avg_add*ay;
            avg_az += avg_add*az;

            records_fine[rec_pos_fine].bpm = BPM;
            records_fine[rec_pos_fine].gsr = gsr;
            records_fine[rec_pos_fine].hrv_param = hrv_parameter;
            records_fine[rec_pos_fine].ax = ax;
            records_fine[rec_pos_fine].ay = ay;
            records_fine[rec_pos_fine].az = az;
            records_fine[rec_pos_fine].step_rate = avg_steps;
            rec_pos_fine++;
            if(rec_pos_fine >= rec_cnt_fine)
                rec_pos_fine = 0;

            last_write_time_fine = tm;
        }

        if(tm - last_write_time_coarse > 10000)
        {
            records_coarse[rec_pos_coarse].bpm = (int)avg_bpm;
            records_coarse[rec_pos_coarse].gsr = avg_gsr;
            records_coarse[rec_pos_coarse].hrv_param = avg_hrv;
            records_coarse[rec_pos_coarse].ax = avg_ax;
            records_coarse[rec_pos_coarse].ay = avg_ay;
            records_coarse[rec_pos_coarse].az = avg_az;
            records_coarse[rec_pos_coarse].step_rate = avg_steps;
            rec_pos_coarse++;
            if(rec_pos_coarse >= rec_cnt_coarse)
                rec_pos_coarse = 0;

            last_write_time_coarse = tm;
        }
    }

    int pos_to_x(int pos, int draw_cnt)
    {
        int left = (int)(vx  * img_width);
        float rel_x = ((float)draw_cnt - (float)pos) / (float)(draw_cnt + 1);
        if(rel_x > 0.99) rel_x = 0.99f;
        if(rel_x < 0.01) rel_x = 0.01f;
        rel_x *= vw; //fit viewport
        return left + (int)(rel_x * img_width);
    }

    int rel_pos_to_y(float rel_pos, int chart_num)
    {
        int top = (int)((vy + vh)  * img_height);
        if(active_charts == 0) active_charts = chart_cnt;
        float sz = 0.85f / active_charts;
        int on_screen_num = 0;
        for(int x = 0; x < chart_num; x++)
            if(chart_states[x] > 0) on_screen_num++;
        rel_pos *= vh * sz; //fit viewport
        return (int) (top * (0.08f + sz * (on_screen_num + 1)) - rel_pos * img_height);
/*
        if(chart_num == active_chart)
        {
            rel_pos *= vh * 0.35; //fit viewport
            return (int) (top * 0.4f - rel_pos * img_height);
        }
        else
        {
            if(chart_num == 0 || (chart_num == 1 && active_chart < 1))
            {
                rel_pos *= vh * 0.2; //fit viewport
                return (int) (top * 0.65f - rel_pos * img_height);
            }
            else if(chart_num == 1 || (chart_num == 2 && active_chart < 2))
            {
                rel_pos *= vh * 0.2; //fit viewport
                return (int) (top * 0.8f - rel_pos * img_height);
            }
            else if(chart_num == 2 || (chart_num == 3 && active_chart < 3))
            {
                rel_pos *= vh * 0.2; //fit viewport
                return (int) (top * 1.0f - rel_pos * img_height);
            }
        }
        return 0;*/
    }

    int bpm_to_y(int bpm)
    {
        int top = (int)((vy + vh)  * img_height);
        if(bpm < 40) bpm = 40;
        float r_bpm = bpm - 50;
        float rel_y = r_bpm / 110.0f;
        return rel_pos_to_y(rel_y, chart_ID_BPM);
    }
    int hrv_to_y(float hrv)
    {
        int top = (int)((vy + vh)  * img_height);
        if(hrv < 1) hrv = 1;
        float rel_y = (hrv-1.0f) / 0.2f;
        return rel_pos_to_y(rel_y, chart_ID_HRV);
    }
    int gsr_to_y(float gsr)
    {
        int top = (int)((vy + vh)  * img_height);
        float rel_y = 0.5f + gsr / 1000.0f;
        return rel_pos_to_y(rel_y, chart_ID_GSR);
    }

    int steps_to_y(float steps_per_min)
    {
        int top = (int)((vy + vh)  * img_height);
        float r_step = steps_per_min;
        float rel_y = r_step / 2.0f;
        return rel_pos_to_y(rel_y, chart_ID_steps);
    }

    int acc_to_y(float acc)
    {
        int top = (int)((vy + vh)  * img_height);
        float rel_y = 0.5f + acc / 60.0f;
        return rel_pos_to_y(rel_y, chart_ID_acc);
    }

    public void draw(Canvas img)
    {
        Paint pnt = new Paint();
        int width = img.getWidth();
        int height = img.getHeight();
        img_width = width;
        img_height = height;

        int left = (int)(vx * width);
        int top = (int)(vy * height);
        int right = (int)((vx + vw) * width);
        int bottom = (int)((vy + vh) * height);

        float line_width = 4.0f;

        int text_size = 40;
        pnt.setARGB(255, 110, 150, 180);
        pnt.setStrokeWidth(line_width);
        if(chart_states[chart_ID_BPM] > 0) {
            img.drawLine(left, bpm_to_y(50), right, bpm_to_y(50), pnt);
            img.drawLine(left, bpm_to_y(75), right, bpm_to_y(75), pnt);
            img.drawLine(left, bpm_to_y(100), right, bpm_to_y(100), pnt);
            img.drawLine(left, bpm_to_y(125), right, bpm_to_y(125), pnt);
            img.drawLine(left, bpm_to_y(150), right, bpm_to_y(150), pnt);
            img.drawLine(left, bpm_to_y(40), left, bpm_to_y(170), pnt);
            pnt.setTextSize(text_size);
            pnt.setStyle(Paint.Style.FILL_AND_STROKE);
            pnt.setARGB(255, 110, 210, 100);
            pnt.setStrokeWidth(1.0f);
            img.drawText("BPM", left + text_size, bpm_to_y(160), pnt);
            img.drawText("50", left + text_size, bpm_to_y(48), pnt);
            img.drawText("75", left + text_size, bpm_to_y(73), pnt);
            img.drawText("100", left + text_size, bpm_to_y(98), pnt);
            img.drawText("125", left + text_size, bpm_to_y(123), pnt);
            img.drawText("150", left + text_size, bpm_to_y(148), pnt);
        }

        if(chart_states[chart_ID_HRV] > 0) {
            pnt.setARGB(255, 110, 150, 180);
            pnt.setStrokeWidth(line_width);
            img.drawLine(left, hrv_to_y(1), right, hrv_to_y(1), pnt);
            img.drawLine(left, hrv_to_y(1.05f), right, hrv_to_y(1.05f), pnt);
            img.drawLine(left, hrv_to_y(1.1f), right, hrv_to_y(1.1f), pnt);
            img.drawLine(left, hrv_to_y(1), left, hrv_to_y(1.18f), pnt);
            pnt.setTextSize(text_size);
            pnt.setStyle(Paint.Style.FILL_AND_STROKE);
            pnt.setARGB(255, 110, 210, 100);
            pnt.setStrokeWidth(1.0f);
            img.drawText("HRV parameter", left + text_size, hrv_to_y(1.12f), pnt);
        }

        if(chart_states[chart_ID_GSR] > 0) {
            pnt.setARGB(255, 110, 150, 180);
            pnt.setStrokeWidth(line_width);
            img.drawLine(left, gsr_to_y(0), right, gsr_to_y(0), pnt);
            img.drawLine(left, gsr_to_y(-250), right, gsr_to_y(-250), pnt);
            img.drawLine(left, gsr_to_y(250), right, gsr_to_y(250), pnt);
            img.drawLine(left, hrv_to_y(-260), left, hrv_to_y(260), pnt);
            pnt.setTextSize(text_size);
            pnt.setStyle(Paint.Style.FILL_AND_STROKE);
            pnt.setARGB(255, 110, 210, 100);
            pnt.setStrokeWidth(1.0f);
            img.drawText("GSR", left + text_size, gsr_to_y(200), pnt);
        }

        if(chart_states[chart_ID_steps] > 0) {
            pnt.setARGB(255, 110, 150, 180);
            pnt.setStrokeWidth(line_width);
            img.drawLine(left, steps_to_y(0), right, steps_to_y(0), pnt);
            img.drawLine(left, steps_to_y(0.5f), right, steps_to_y(0.5f), pnt);
            img.drawLine(left, steps_to_y(1.0f), right, steps_to_y(1.0f), pnt);
            img.drawLine(left, steps_to_y(0), left, steps_to_y(1.0f), pnt);
            pnt.setTextSize(text_size);
            pnt.setStyle(Paint.Style.FILL_AND_STROKE);
            pnt.setARGB(255, 110, 210, 100);
            pnt.setStrokeWidth(1.0f);
            img.drawText("steps/min", left + text_size, steps_to_y(1.0f), pnt);
        }

        if(chart_states[chart_ID_acc] > 0) {
            pnt.setARGB(255, 110, 150, 180);
            pnt.setStrokeWidth(line_width);
            img.drawLine(left, acc_to_y(0), right, acc_to_y(0), pnt);
            img.drawLine(left, acc_to_y(10), right, acc_to_y(10), pnt);
            img.drawLine(left, acc_to_y(-10), right, acc_to_y(-10), pnt);
            img.drawLine(left, acc_to_y(-10), left, acc_to_y(10), pnt);
            pnt.setTextSize(text_size);
            pnt.setStyle(Paint.Style.FILL_AND_STROKE);
            pnt.setARGB(255, 110, 210, 100);
            pnt.setStrokeWidth(1.0f);
            img.drawText("Accel (xyz)", left + text_size, acc_to_y(10), pnt);
        }

        pnt.setStrokeWidth(line_width);

        int prev_x = 0;
        int prev_bpm = 0;
        int prev_gsr = 0;
        int prev_steps = 0;
        int prev_ax = 0;
        int prev_ay = 0;
        int prev_az = 0;
        int prev_hrv = 0;

        boolean use_fine = false;
        if(draw_range*4 < rec_cnt_fine) use_fine = true;

        int max_cnt = draw_range / 10;
        if(max_cnt > rec_cnt_coarse) max_cnt = rec_cnt_coarse;
        if(use_fine)
        {
            max_cnt = draw_range * 4;
        }

        float gsr_min = 123456;
        float gsr_max = -123456;
        for(int p = 0; p < max_cnt; p++) {
            int ppos;
            float v_bpm, v_gsr, v_steps, v_ax, v_ay, v_az, v_hrv;
            if (use_fine) {
                ppos = rec_pos_fine - 1 - p;
                if (ppos < 0) ppos += rec_cnt_fine;
                v_bpm = records_fine[ppos].bpm;
                v_gsr = records_fine[ppos].gsr;
                v_steps = records_fine[ppos].step_rate;
                v_ax = records_fine[ppos].ax;
                v_ay = records_fine[ppos].ay;
                v_az = records_fine[ppos].az;
                v_hrv = records_fine[ppos].hrv_param;
            } else {
                ppos = rec_pos_coarse - 1 - p;
                if (ppos < 0) ppos += rec_cnt_coarse;
                v_bpm = records_coarse[ppos].bpm;
                v_gsr = records_coarse[ppos].gsr;
                v_steps = records_coarse[ppos].step_rate;
                v_ax = records_coarse[ppos].ax;
                v_ay = records_coarse[ppos].ay;
                v_az = records_coarse[ppos].az;
                v_hrv = records_coarse[ppos].hrv_param;
            }
            if (v_gsr < gsr_min && (v_gsr < -0.1 || v_gsr > 0.1) ) gsr_min = v_gsr;
            if (v_gsr > gsr_max && (v_gsr < -0.1 || v_gsr > 0.1) ) gsr_max = v_gsr;
        }

        for(int p = 0; p < max_cnt; p++)
        {
            int ppos;

//            if(bpm_cur_hist[ppos] < 1) continue;
            int x_coord = pos_to_x(p, max_cnt);

            int y_bpm, y_gsr, y_steps, y_ax, y_ay, y_az, y_hrv;
            if(use_fine)
            {
                ppos = rec_pos_fine - 1 - p;
                if(ppos < 0) ppos += rec_cnt_fine;
                y_bpm = bpm_to_y(records_fine[ppos].bpm);
//                y_gsr = gsr_to_y(records_fine[ppos].gsr);
                float r_gsr = records_fine[ppos].gsr;
                if(r_gsr > -0.1 && r_gsr < 0.1) r_gsr = gsr_min;
                y_gsr = gsr_to_y((r_gsr - gsr_min) * 250.0f / (gsr_max - gsr_min));
                y_steps = steps_to_y(records_fine[ppos].step_rate);
                y_ax = acc_to_y(records_fine[ppos].ax);
                y_ay = acc_to_y(records_fine[ppos].ay);
                y_az = acc_to_y(records_fine[ppos].az);
                y_hrv = hrv_to_y(records_fine[ppos].hrv_param);
            }
            else
            {
                ppos = rec_pos_coarse - 1 - p;
                if(ppos < 0) ppos += rec_cnt_coarse;
                y_bpm = bpm_to_y(records_coarse[ppos].bpm);
//                y_gsr = gsr_to_y(records_coarse[ppos].gsr);
                y_gsr = gsr_to_y((records_coarse[ppos].gsr - gsr_min) * 250.0f / (gsr_max - gsr_min));
                y_steps = steps_to_y(records_coarse[ppos].step_rate);
                y_ax = acc_to_y(records_coarse[ppos].ax);
                y_ay = acc_to_y(records_coarse[ppos].ay);
                y_az = acc_to_y(records_coarse[ppos].az);
                y_hrv = hrv_to_y(records_coarse[ppos].hrv_param);
            }

            if(p == 0)
            {
                prev_x = x_coord;
                prev_bpm = y_bpm;
                prev_gsr = y_gsr;
                prev_steps = y_steps;
                prev_ax = y_ax;
                prev_ay = y_ay;
                prev_az = y_az;
                prev_hrv = y_hrv;
                continue;
            }
            if(chart_states[chart_ID_BPM] > 0) {
                pnt.setARGB(255, 50, 255, 70);
                img.drawLine(x_coord, y_bpm, prev_x, prev_bpm, pnt);
            }
            if(chart_states[chart_ID_GSR] > 0) {
                pnt.setARGB(255, 150, 255, 70);
                img.drawLine(x_coord, y_gsr, prev_x, prev_gsr, pnt);
            }
            if(chart_states[chart_ID_HRV] > 0) {
                pnt.setARGB(255, 50, 255, 170);
                img.drawLine(x_coord, y_hrv, prev_x, prev_hrv, pnt);
            }
            if(chart_states[chart_ID_steps] > 0) {
                pnt.setARGB(255, 50, 255, 170);
                img.drawLine(x_coord, y_steps, prev_x, prev_steps, pnt);
            }
            if(chart_states[chart_ID_acc] > 0) {
                pnt.setARGB(255, 150, 55, 70);
                img.drawLine(x_coord, y_ax, prev_x, prev_ax, pnt);
                pnt.setARGB(255, 150, 155, 70);
                img.drawLine(x_coord, y_ay, prev_x, prev_ay, pnt);
                pnt.setARGB(255, 50, 155, 170);
                img.drawLine(x_coord, y_az, prev_x, prev_az, pnt);
            }
            prev_x = x_coord;
            prev_bpm = y_bpm;
            prev_gsr = y_gsr;
            prev_hrv = y_hrv;
            prev_steps = y_steps;
            prev_ax = y_ax;
            prev_ay = y_ay;
            prev_az = y_az;
        }

    }
}
