package com.ultimaterobotics.uecgmonitor4_2;

import android.graphics.Canvas;
import android.graphics.Paint;

public class ECG_processor {
    //processing buffer
    float[] points_hist = new float[500];
    long[] points_uid = new long[500];
    int[] points_valid = new int[500];
    int hist_depth = 500;

    //drawing buffer - when in pause mode, not updated with processing buffer
    float[] draw_points_buf = new float[20000];
    long[] draw_points_uid = new long[20000];
    int[] draw_points_valid = new int[20000];
    int draw_buf_size = 20000;
    int draw_buf_cur_pos = 0;

    int draw_grid = 1;

    long save_start_time = 0;

    float data_rate = 122; //uECG sends at 122 Hz and for now it's a constant for BLE version

    float screen_time = 2.5f; //time interval currently displayed on screen

    int paused = 0;

    float vx, vy, vw, vh;

    int img_width;
    int img_height;

    float draw_shift = 0;

    public void set_save_time(long value)
    {
        save_start_time = value;
    }

    public void set_pause_state(int pause_on)
    {
        paused = pause_on;
    }

    public void set_draw_shift(float dt)
    {
        draw_shift = dt;
    }

    public void set_screen_time(float val)
    {
        screen_time = val;
    }

    public void set_viewport(float pos_x, float pos_y, float width, float height)
    {
        vx = pos_x;
        vy = pos_y;
        vw = width;
        vh = height;
    }

    public void push_data(int d_id, int[] values, int values_count)
    {
        for(int x = 0; x < hist_depth - d_id; x++)
        {
            points_hist[x] = points_hist[x + d_id];
            points_uid[x] = points_uid[x + d_id];
            points_valid[x] = points_valid[x + d_id];
        }
        long tm = System.currentTimeMillis();
        for(int x = 0; x < d_id; x++)
        {
            points_hist[hist_depth - d_id + x] = values[0];
            points_uid[hist_depth - d_id + x] = tm - save_start_time;
            points_valid[hist_depth - d_id + x] = 0;
        }
        for(int x = 0; x < values_count; x++)
        {
            points_hist[hist_depth - values_count + x] = values[x];
            points_uid[hist_depth - values_count+ x] = tm - save_start_time;
            points_valid[hist_depth - values_count + x] = 1;
        }

        if(paused != 0) return;

        for(int x = 0; x < d_id; x++)
        {
            draw_points_buf[draw_buf_cur_pos] = points_hist[hist_depth - d_id + x];
            draw_points_uid[draw_buf_cur_pos] = points_uid[hist_depth - d_id + x];
            draw_points_valid[draw_buf_cur_pos] = points_valid[hist_depth - d_id + x];
            draw_buf_cur_pos++;
            if(draw_buf_cur_pos >= draw_buf_size)
                draw_buf_cur_pos = 0;
        }
    }

    public int get_last_data_count(int count, float[] data, long[] uid, int[] valid)
    {
        int cnt = count;
        if(count > hist_depth-2) cnt = hist_depth-2;
        for(int x = 0; x < cnt; x++)
        {
            data[x] = points_hist[hist_depth - cnt + x];
            uid[x] = points_uid[hist_depth - cnt + x];
            valid[x] = points_valid[hist_depth - cnt + x];
        }
        return cnt;
    }

    int time_to_x(float tim)
    {
        int right = (int)((vx + vw) * img_width);
        float scr_per_second = vw / screen_time;
        return right - (int)(scr_per_second * tim * img_width);
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
        int center = (top + bottom) / 2;
        int length = right - left;

        int buf_pos = draw_buf_cur_pos - 1;
        if(paused > 0 && draw_shift > 0)
        {
            buf_pos = draw_buf_cur_pos - (int)(data_rate * draw_shift);
        }
        if(buf_pos < 0) buf_pos += draw_buf_size;

        int prev_x = right;
        int prev_y = center;
        int num_points = (int)(data_rate * screen_time);
        int ppos = buf_pos;
        float min_v = draw_points_buf[ppos];
        float max_v = min_v;
        float avg_val = 0;
        float avg_z = 0.00001f;
        min_v -= 10;
        max_v += 10;

        for(int p = 0; p < num_points; p++)
        {
            ppos = buf_pos - p;
            if(ppos < 0) ppos += draw_buf_size;
            float p_val = draw_points_buf[ppos];
            if(p_val < min_v) min_v = p_val;
            if(p_val > max_v) max_v = p_val;
            avg_val += p_val;
            avg_z += 1.0f;
        }

        avg_val /= avg_z;
        if(draw_grid > 0)
        {
            pnt.setARGB(255, 120, 80, 80);
            float tim_step = 0.04f;
            int cnt = 0;
            for(float tv = 0; tv < screen_time; tv += tim_step)
            {
                if(cnt%5 == 0)
                    pnt.setARGB(255, 80, 120, 150);
                else
                    pnt.setARGB(120, 80, 120, 150);
                int x_pos = time_to_x(tv);
                if(screen_time < 3.0 || cnt%5 == 0)
                    img.drawLine(x_pos, bottom, x_pos, top, pnt);
                cnt++;
            }
//0.57220459 uV per 1 unit
            float min_uv = (avg_val - min_v) * 0.5722f; //in uV
            float max_uv = (max_v - avg_val) * 0.5722f; //in uV

            cnt = 0;
            for(float uv = 0; uv < max_uv; uv += 100)
            {
                if(cnt%5 == 0)
                    pnt.setARGB(255, 80, 120, 150);
                else
                    pnt.setARGB(120, 80, 120, 150);
                int y_pos = bottom + (int)((top - bottom) * (avg_val + uv / 0.5722f - min_v) / (max_v - min_v));
                img.drawLine(left, y_pos, right, y_pos, pnt);
                cnt++;
            }
            cnt = 0;
            for(float uv = 0; uv < min_uv; uv += 100)
            {
                if(cnt%5 == 0)
                    pnt.setARGB(255, 80, 120, 150);
                else
                    pnt.setARGB(120, 80, 120, 150);
                int y_pos = bottom + (int)((top - bottom) * (avg_val - uv / 0.5722 - min_v) / (max_v - min_v));
                img.drawLine(left, y_pos, right, y_pos, pnt);
                cnt++;
            }
        }

        pnt.setStrokeWidth(3.0f);
        for(int p = 0; p < num_points; p++)
        {
            ppos = buf_pos - p;
            if(ppos < 0) ppos += draw_buf_size;
            float p_val = draw_points_buf[ppos];
            int valid = draw_points_valid[ppos];
            int cur_x = right - (p * length / num_points);
            int cur_y = bottom + (int)((top - bottom) * (p_val - min_v) / (max_v - min_v));

            if(p > 0)
            {
                if(valid == 0)
                    pnt.setARGB(255, 250, 155, 70);
                else
                    pnt.setARGB(255, 50, 255, 70);

                img.drawLine(prev_x, prev_y, cur_x, cur_y, pnt);
            }
            prev_x = cur_x;
            prev_y = cur_y;
        }


    }


}
