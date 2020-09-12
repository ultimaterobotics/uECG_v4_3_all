package com.ultimaterobotics.uecgmonitor4_2;

import android.graphics.Canvas;
import android.graphics.Paint;

public class RR_processor {
    float[] RR_cur_hist = new float[20000];
    float[] RR_prev_hist = new float[20000];
    long[] RR_uid = new long[20000];
    int buf_size = 20000;
    int buf_cur_pos = 0;

    float data_rate = 122; //uECG sends at 122 Hz and for now it's a constant for BLE version

    float vx, vy, vw, vh;

    int img_width;
    int img_height;

    float hrv_parameter = 1;

    float draw_time_xmin = 0.25f;
    float draw_time_xmax = 1.4f;
    float draw_time_ymin = 0.25f;
    float draw_time_ymax = 1.4f;

    long start_time = 0;

    public float get_hrv_parameter()
    {
        return  hrv_parameter;
    };

    public void set_viewport(float pos_x, float pos_y, float width, float height)
    {
        vx = pos_x;
        vy = pos_y;
        vw = width;
        vh = height;
    }

    public void push_data(int RR_cur, int RR_prev)
    {
        long tm = System.currentTimeMillis();
        if(start_time == 0) {
            start_time = tm;
            for(int x = 0; x < buf_size; x++)
            {
                RR_cur_hist[x] = 0;
                RR_prev_hist[x] = 0;
            }
        }
        float rr1 = RR_cur;
        float rr2 = RR_prev;
        float rel = rr1 / rr2;
        if(rr2 / rr1 > rel) rel = rr2 / rr1;

        boolean false_detect = false;
        if(rel > 1.7 && rel < 2.3)
        {
            false_detect = true;
        }
        if(rel > 2.7 && rel < 3.3)
        {
            false_detect = true;
        }
        if(rel > 3.8)
        {
            false_detect = true;
        }

        if(!false_detect)
        {
            hrv_parameter *= 0.95;
            hrv_parameter += 0.05*rel;

            RR_cur_hist[buf_cur_pos] = 0.001f * (float)RR_cur;
            RR_prev_hist[buf_cur_pos] = 0.001f * (float)RR_prev;
            RR_uid[buf_cur_pos] = tm - start_time;
            buf_cur_pos++;
            if(buf_cur_pos >= buf_size)
                    buf_cur_pos = 0;
        }
    }

    int time_to_x(float tim)
    {
        int left = (int)(vx  * img_width);
        float rel_x = (tim - draw_time_xmin) / (draw_time_xmax - draw_time_xmin);
        if(rel_x > 0.99) rel_x = 0.99f;
        if(rel_x < 0.01) rel_x = 0.01f;
        rel_x *= vw; //fit viewport
        return left + (int)(rel_x * img_width);
    }

    int time_to_y(float tim)
    {
        int top = (int)((vy + vh)  * img_height);
        float rel_y = (tim - draw_time_ymin) / (draw_time_ymax - draw_time_ymin);
        if(rel_y > 0.99) rel_y = 0.99f;
        if(rel_y < 0.01) rel_y = 0.01f;
        rel_y *= vh; //fit viewport
        return top - (int)(rel_y * img_height);
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

        int buf_pos = buf_cur_pos - 1;
        if(buf_pos < 0) buf_pos += buf_size;

        int num_points = buf_size;
        int ppos = buf_pos;

        //draw grid
        pnt.setARGB(255, 120, 80, 80);
        float tim_step = 0.1f;
        int text_size = 40;
        pnt.setTextSize(text_size);
        pnt.setStyle(Paint.Style.FILL_AND_STROKE);
        pnt.setARGB(255, 80, 180, 150);
        img.drawText("Previous vs next beat time (in seconds)", left, top - text_size, pnt);

        for(float tv = draw_time_xmin; tv < draw_time_xmax; tv += tim_step)
        {
            pnt.setARGB(255, 80, 120, 150);
            int x_pos = time_to_x(tv);
            img.drawLine(x_pos, bottom, x_pos, top, pnt);
            int rr = Math.round(tv*10);
            int pre_dot = 0;
            int past_dot = rr;
            if(rr >= 10)
            {
                pre_dot = rr/10;
                past_dot = rr%10;
            }
            img.drawText(pre_dot + "." + past_dot, time_to_x(tv), bottom, pnt);
        }
        for(float tv = draw_time_ymin; tv < draw_time_ymax; tv += tim_step)
        {
            pnt.setARGB(255, 80, 120, 150);
            int y_pos = time_to_y(tv);
            img.drawLine(left, y_pos, right, y_pos, pnt);

            int rr = Math.round(tv*10);
            int pre_dot = 0;
            int past_dot = rr;
            if(rr >= 10)
            {
                pre_dot = rr/10;
                past_dot = rr%10;
            }
            if(tv > draw_time_ymin + 0.001)
                img.drawText(pre_dot + "." + past_dot, left, time_to_y(tv), pnt);
        }

        for(int p = 0; p < num_points; p++)
        {
            ppos = buf_pos - p;
            if(ppos < 0) ppos += buf_size;
            if(RR_cur_hist[ppos] < 0.001 || RR_prev_hist[ppos] < 0.001) continue;
            int x_coord = time_to_x(RR_cur_hist[ppos]);
            int y_coord = time_to_y(RR_prev_hist[ppos]);
            int r_size = 3;
            pnt.setARGB(255, 50, 255, 70);
            img.drawRect(x_coord - r_size, y_coord - r_size, x_coord + r_size, y_coord + r_size, pnt);
        }

        int ident_x0 = time_to_x(draw_time_xmin);
        int ident_y0 = time_to_y(draw_time_ymin);
        int ident_x1 = time_to_x(draw_time_xmax);
        int ident_y1 = time_to_y(draw_time_ymax);
        pnt.setARGB(255, 110, 150, 180);
        img.drawLine(ident_x0, ident_y0, ident_x1, ident_y1, pnt);
    }
}
