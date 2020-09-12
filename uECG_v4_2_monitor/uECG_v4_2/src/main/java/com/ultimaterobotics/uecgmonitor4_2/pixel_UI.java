package com.ultimaterobotics.uecgmonitor4_2;

import android.graphics.Canvas;
import android.graphics.Paint;

public class pixel_UI {

    public interface SimpleCallback {
        void callback(int action, float dx, float dy);
    }
    public enum GBType {
        touch,
        toggle,
        scroll,
        label,
        checkbox
    }
    public class GraphButton
    {
        public float x;
        public float y;
        public float w;
        public float h;
        public float chbox_size;
        public int text_size;
        public String text;
        public String toggled_text;
        public int draw_rect = 1;
        public int draw_text = 1;
        public int draw_cross = 0;
        public GBType type;
        public int pressed = 0;
        public int r, g, b;
        public int tr, tg, tb;
        public int draw_mask = 0xFFFFFFFF;
        public SimpleCallback touch_callback;

        int visible = 1;

        public void set_callback(SimpleCallback callback)
        {
            touch_callback = callback;
        }

        public void set_coords(float pos_x, float pos_y, float width, float height)
        {
            x = pos_x;
            y = pos_y;
            w = width;
            h = height;
        }
        public void set_color_normal(int rr, int gg, int bb)
        {
            r = rr; g = gg; b = bb;
        }
        public void set_color_toggled(int rr, int gg, int bb)
        {
            tr = rr; tg = gg; tb = bb;
        }

        public void draw(Canvas img, int draw_mask)
        {
            if((this.draw_mask & draw_mask) == 0)
            {
                visible = 0;
                return;
            }
            visible = 1;
            Paint pnt = new Paint();
            int width = img.getWidth();
            int height = img.getHeight();

            int left = (int)(x * width);
            int top = (int)(y * height);
            int right = (int)((x + w) * width);
            int bottom = (int)((y + h) * height);
            int check_size = (int)(chbox_size * width);
            int cr = r, cg = g, cb = b;
            String txt = text;
            if(pressed > 0)
            {
                cr = tr;
                cg = tg;
                cb = tb;
                if(toggled_text != null)
                    txt = toggled_text;
            }
            if(type == GBType.checkbox)
            {
                pnt.setARGB(255, cr, cg, cb);
                if(pressed > 0)
                    pnt.setStyle(Paint.Style.FILL_AND_STROKE);
                else
                    pnt.setStyle(Paint.Style.STROKE);
                img.drawRect(left, (top+bottom-check_size)/2, left+check_size, (top+bottom+check_size)/2, pnt);
            }
            if(draw_rect > 0)
            {
                pnt.setARGB(255, cr, cg, cb);
                pnt.setStyle(Paint.Style.STROKE);
                img.drawRect(left, top, right, bottom, pnt);
            }
            if(draw_cross > 0)
            {
                pnt.setARGB(255, cr, cg, cb);
                pnt.setStyle(Paint.Style.STROKE);
                img.drawLine(left, top, right, bottom, pnt);
                img.drawLine(right, top, left, bottom, pnt);
            }
            if(draw_text > 0)
            {
                pnt.setTextSize(text_size);
                pnt.setStyle(Paint.Style.FILL_AND_STROKE);
                pnt.setARGB(255, cr, cg, cb);
                if(type == GBType.checkbox)
                    img.drawText(txt, left + check_size + text_size, (top + bottom + text_size/2)/2, pnt);
                else
                    img.drawText(txt, left + text_size, (top + bottom + text_size/2)/2, pnt);
            }
        }

        public void process_touch(int touch_type, float tx, float ty, float sx, float sy)
        {
            if(visible == 0) return;
            if(tx < x || tx > x + w || ty < y || ty > y + h) return;
            if((type == GBType.toggle || type == GBType.checkbox) && touch_type == 1)
            {
                if(pressed == 0) pressed = 1;
                else pressed = 0;
                touch_callback.callback(1,0, 0);
            }
            if(type == GBType.touch && touch_type == 1)
            {
                touch_callback.callback(1, 0, 0);
            }
            if(type == GBType.scroll)
            {
                if(touch_type == 3) //zoom
                    touch_callback.callback(3, sx, sy);
                if(touch_type == 2)
                    touch_callback.callback(2,tx-sx, ty-sy);
                if(touch_type == 1)
                    touch_callback.callback(1,tx-sx, ty-sy);
            }
        }
    }

}
