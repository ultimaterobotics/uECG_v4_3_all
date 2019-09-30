package com.ultimaterobotics.uecgmonitor;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.Bundle;
import android.os.Handler;
import android.os.ParcelUuid;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Intent;

import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.ScanSettings;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import android.content.Context;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    int cnt = 0;
    int getCnt() {return cnt;};

    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothManager bluetoothManager;
    private BluetoothLeAdvertiser mBluetoothLeAdvertiser;
    int bt_inited = 0;
    int adv_on = 0;
    private static final int REQUEST_ENABLE_BT = 1;

    private BluetoothLeScanner mLEScanner;
    private ScanSettings settings;
    private List<ScanFilter> filters;

    void stopScanning() {
        mLEScanner.stopScan(mScanCallback);
        mScanCallback = null;
    }
    void init_bluetooth() {
        // Initializes Bluetooth adapter.
        bluetoothManager =
                (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = bluetoothManager.getAdapter();
        // Code here executes on main thread after user presses button
        if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }

        mLEScanner = mBluetoothAdapter.getBluetoothLeScanner();
        bt_inited = 1;
    };
    Handler mHandler = new Handler();

    long prev_time = 0;
    float avg_dt = 0;
    float avg_dp = 0;
    double[] points_hist = new double[500];
    double[] points_dh = new double[500];
    long[] points_uid = new long[500];
    int[] points_valid = new int[500];
    int hist_depth = 400;
    int[] in_buf = new int[50];
    int points_count = 19;
    int[] cur_vals = new int[55];
    int points_pos = 0;
    int pack_id = 0;
    int prev_pack_id = 0;
    int batt = 0;
    int draw_skip = 0;
    double avg_dv = 0;
    double avg_dv_f = 0;
    double prev_peak = 0;
    int prev_peak_cnt = 0;
    long[] bpm_times = new long[20];
    int[] bpm_chart = new int[100];
    int bpm_length = 100;
    int bpm_interval = 10;
    long last_bpm_add = 0;

    float lost_points_avg = 0;
    float total_points_avg = 0;

    int total_packs = 0;

    int dev_BPM = 0;
    int dev_SDNN = 0;
    int dev_RMSSD = 0;
    int dev_skin = 0;
    float dev_ax = 0;
    float dev_ay = 0;
    float dev_az = 0;
    int dev_lastRR_id = -1;

    int rec_uid = 0;
    int unsaved_cnt = 0;

    int version_id = -1;

    int[] pNN_short = new int[32];

    File log_file = null;
    File log_file_ecg = null;
    FileOutputStream file_out_stream;
    FileOutputStream file_out_stream_ecg;

    String dev_mac = "";

    int save_on = 0;
    long save_start_time = 0;

    Timer timer = new Timer();

    int parse_result(byte[] scanRecord, int type, int rssi, String mac)
    {
        int param_batt_bpm = 0;
        int param_sdnn = 1;
        int param_skin_res = 2;
        int param_lastRR = 3;
        int param_imu_acc = 4;
        int param_pnn_bins = 5;
        int param_end = 6;
        int param_emg_spectrum = 7;

        if(true)
        {
            String name = "";
            for(int n = 0; n < 4; n++)
                name += (char)scanRecord[2 + n];

            if(name.contains("uECG") && mac.contains("BA:BE"))
            {
                if (rssi > -60 && dev_mac.length() < 2)
                    dev_mac = mac;
            }
            if(mac.equals(dev_mac))
            {

                for(int n = 6; n < 35; n++) {
                    in_buf[n-6] = scanRecord[n];
                    if(in_buf[n-6] < 0)
                        in_buf[n-6] = 256 + in_buf[n-6];
                }
                pack_id = in_buf[1];
                int param_id = in_buf[2] & 0x0F;
                int param_mod = (in_buf[2]>>4) & 0x0F;
                if(param_id == param_batt_bpm)
                {
                    batt = in_buf[3];
                    if(version_id < 0)
                    {
                        if(in_buf[4] >= 3 && in_buf[4] < 20)
                            version_id = in_buf[4];
                        else
                            version_id = 2;
                    }
                    dev_BPM = in_buf[5];
                }
                if(param_id == param_sdnn)
                {
                    int v1_h = in_buf[3]>>4;
                    int v2_h = in_buf[3]&0x0F;
                    int v1_l = in_buf[4];
                    int v2_l = in_buf[5];
                    int v1 = (v1_h<<8) | v1_l;
                    int v2 = (v2_h<<8) | v2_l;
                    dev_SDNN = v1;
                    dev_RMSSD = v2;
                }
                if(param_id == param_skin_res)
                {
                    dev_skin = in_buf[3] * 256 + in_buf[4];
                }
                if(param_id == param_imu_acc)
                {
                    dev_ax = (in_buf[3] - 128.0f) / 4.0f;
                    dev_ay = (in_buf[4] - 128.0f) / 4.0f;
                    dev_az = (in_buf[5] - 128.0f) / 4.0f;
                }
                if(param_id == param_pnn_bins) {
                    int pnn_bin = param_mod;
                    pNN_short[pnn_bin+0] = in_buf[3];
                    pNN_short[pnn_bin+1] = in_buf[4];
                    pNN_short[pnn_bin+2] = in_buf[5];
                }

                if(param_id == param_lastRR)
                {
                    int v1_h = in_buf[3]>>4;
                    int v2_h = in_buf[3]&0x0F;
                    int v1_l = in_buf[4];
                    int v2_l = in_buf[5];
                    int v1 = (v1_h<<8) | v1_l;
                    int v2 = (v2_h<<8) | v2_l;

                    int rr_id = param_mod;
                    if(rr_id != dev_lastRR_id)
                    {
                        dev_lastRR_id = rr_id;
                        if(save_on == 1)
                        {
                            String out_buf;
                            long tm = System.currentTimeMillis();
                            out_buf = tm + ";" + v1 + ";" + v2 + ";" + dev_skin + ";" + dev_ax + ";" + dev_ay + ";" + dev_az + "\n";
                            String ecg_buf = "";
                            int save_cnt = unsaved_cnt;
                            unsaved_cnt = 0;
                            if(save_cnt > hist_depth-2) save_cnt = hist_depth-2;

                            for(int n = 0; n < save_cnt; n++)
                            {
                                ecg_buf += points_uid[hist_depth - save_cnt + n] + ";" + points_hist[hist_depth - save_cnt + n] + ";" + points_valid[hist_depth - save_cnt + n] + "\n";
                            }
                            try {
//                                if(file_out_stream == null)
                                  file_out_stream = new FileOutputStream(log_file, true);
                                  file_out_stream_ecg = new FileOutputStream(log_file_ecg, true);
//                                file_stream_writer = new OutputStreamWriter(fileOutputStream);

                                try {
                                    file_out_stream.write(out_buf.getBytes());
                                    file_out_stream_ecg.write(ecg_buf.getBytes());
//                                    file_stream_writer.write(out_buf);
//                                    file_buf_writer = new BufferedWriter(file_stream_writer);
//                                    file_buf_writer.write(out_buf);
                                } finally {
                                    file_out_stream.close();
                                    file_out_stream_ecg.close();
//                                    file_stream_writer.close();
                                    ;
//                                    file_buf_writer.close();
                                }
                            }
                            catch(IOException ex)
                            {
                                ;
//                                Log.e(ex.getMessage());
                            }
                        }
                    }
                }
                int pack_data_count = 10;
                if(version_id == 2)
                {
                    for (int x = 0; x < pack_data_count; x++) {
                        cur_vals[x] = in_buf[5 + x * 2] * 256 + in_buf[6 + x * 2];
                        if (cur_vals[x] > 32767)
                            cur_vals[x] = -65536 + cur_vals[x];
                    }
                }
                if(version_id == 3)
                {
                    pack_data_count = 18;
                    byte check = 0;
                    int cur_v = in_buf[5] * 256 + in_buf[6];
                    if (cur_v > 32767)
                        cur_v = -65536 + cur_v;
                    int scale_v = in_buf[7];
                    if(scale_v > 100) scale_v = 100 + (scale_v - 100)*4;
                    cur_vals[0] = cur_v;
                    for (int x = 1; x < pack_data_count; x++) {
                        int dv = in_buf[7 + x];
                        if (dv > 127) dv = -256 + dv;
                        cur_v += dv * scale_v;
                        check += in_buf[7 + x];
                        cur_vals[x] = cur_v;
                    }
                    byte cmp_checksum = (byte)in_buf[7 + pack_data_count];
                    if(cmp_checksum != check)
                    {
                        ;//error
                    }
//                    for (int x = 0; x < pack_data_count; x++)
//                        cur_vals[x] = x;
                }

                if(version_id == 4)
                {
                    pack_data_count = 18;
                    byte check = 0;
                    int cur_v = in_buf[6] * 256 + in_buf[7];
                    if (cur_v > 32767)
                        cur_v = -65536 + cur_v;
                    int scale_v = in_buf[8];
                    if(scale_v > 100) scale_v = 100 + (scale_v - 100)*4;
                    cur_vals[0] = cur_v;
                    for (int x = 1; x < pack_data_count; x++) {
                        int dv = in_buf[8 + x];
                        if (dv > 127) dv = -256 + dv;
                        cur_v += dv * scale_v;
                        cur_vals[x] = cur_v;
                    }
                }

                int d_id = pack_id - prev_pack_id;
                prev_pack_id = pack_id;
                if(d_id < 0) d_id += 256;
                draw_skip++;
                if(d_id == 0)
                    return 1;
//                if(d_id > 100)
//                {
//                    d_id = 100;// pack_data_count;
//                }

                total_packs++;
                unsaved_cnt += d_id;
                for(int x = 0; x < hist_depth - d_id; x++)
                {
                    points_hist[x] = points_hist[x + d_id];
                    points_uid[x] = points_uid[x + d_id];
                    points_valid[x] = points_valid[x + d_id];
                }
                long tm = System.currentTimeMillis();
                for(int x = 0; x < pack_data_count; x++)
                {
                    points_hist[hist_depth - pack_data_count + x] = cur_vals[x];
                    points_uid[hist_depth - pack_data_count + x] = tm - save_start_time;
                    points_valid[hist_depth - pack_data_count + x] = 1;

                }

                return 1;
            }
        }
        return  0;
    };
    int init_file()
    {
        String state = Environment.getExternalStorageState();
        //Environment.DIRECTORY_DOCUMENTS;
        try
        {
        if (Environment.MEDIA_MOUNTED.equals(state)) {
            SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MMM-dd-HH-mm-ss");
            String ts = sdf.format(Calendar.getInstance().getTime());
            String log_name = "uECG/uecg_log_" + ts + ".csv";
            String log_name_ecg = "uECG/uecg_dat_" + ts + ".csv";
            File dir = new File(Environment.getExternalStoragePublicDirectory(
                    Environment.DIRECTORY_DOCUMENTS), "uECG");
            dir.mkdirs();

            log_file = new File(Environment.getExternalStoragePublicDirectory(
                    Environment.DIRECTORY_DOCUMENTS), log_name);
            log_file_ecg = new File(Environment.getExternalStoragePublicDirectory(
                    Environment.DIRECTORY_DOCUMENTS), log_name_ecg);

            save_start_time = System.currentTimeMillis();

            return 1;
        }
        }catch(Exception ex)
        {
            ;
//            Log.e(ex.getMessage());
        }
        return 0;
    };
    class drawUITask extends TimerTask {

        @Override
        public void run() {
            MainActivity.this.runOnUiThread(new Runnable() {

                @Override
                public void run() {
                    TextView var1 = (TextView)findViewById(R.id.textVar1);
//                    TextView var2 = (TextView)findViewById(R.id.textVar2);
//                    txt.setText(name + " rssi " + rssi);
                    int bat_v = 0;
                    if(batt > 147)
                        bat_v = 4200;
                    else bat_v = 4200 - (147 - batt)*35;
                    float lost_perc = 0;
                    if(total_points_avg > 1)
                        lost_perc = (float)(Math.round((float)(lost_points_avg / total_points_avg * (float)1000.0))/10.0);
                    var1.setText("Battery: " + bat_v + " mV" + " lost %: " + lost_perc + " total packs " + total_packs);
//                    var1.setText("Battery: " + bat_v + " mV");// + " d_id " + d_id);
//                    var1.setText("id " + pack_id + " bat " + batt);
//                    var2.setText("bat " + batt); //130 -> 3.6, 147 -> 4.2
                    points_pos = 0;
                    double min_v = points_hist[0];
                    double max_v = points_hist[0];
                    for(int x = 0; x < hist_depth-12; x++) {
                        if(points_hist[x] < min_v) min_v = points_hist[x];
                        if(points_hist[x] > max_v) max_v = points_hist[x];
                    }
                    min_v -= 1;
                    max_v += 1;
                    SurfaceView sv = (SurfaceView)findViewById(R.id.surfaceView);
                    Canvas img = sv.getHolder().getSurface().lockCanvas(null);
                    Paint pnt = new Paint();
                    int width = img.getWidth();
                    int height = img.getHeight();
                    pnt.setARGB(255, 0, 0, 0);
                    img.drawRect(0, 0, width, height, pnt);
                    pnt.setARGB(255, 50, 255, 70);
                    int prev_x = width;
                    int prev_y = height/2;
                    for(int p = 0; p < hist_depth-1; p++)
                    {
                        int cur_x = (p * width / hist_depth);
                        int cur_y = (int)(height*0.9 - 0.3*height * (points_hist[p] - min_v) / (max_v - min_v));

                        if(p > 0)
                        {
                            if(points_valid[p] == 0)
                                pnt.setARGB(255, 250, 155, 70);
                            else
                                pnt.setARGB(255, 50, 255, 70);

                            img.drawLine(prev_x, prev_y, cur_x, cur_y, pnt);
                        }
                        prev_x = cur_x;
                        prev_y = cur_y;
                    }

                    for(int b = 0; b < 15; b++)
                    {
                        int bw = (int)(width/15.0*0.6);
                        int cur_x = (int)(width*0.05) + b*(int)(bw+5);
                        int cur_ys = (int)(height*0.20 - 0.2*height * pNN_short[b] / 255.0);
                        pnt.setTextSize(15);
                        pnt.setARGB(255, 0, 200, 100);
                        img.drawRect(cur_x, cur_ys, cur_x+bw, (int)(height*0.2), pnt);
                        img.drawText("" + (int)(pNN_short[b]/2.55), cur_x, cur_ys-15, pnt);

//                        pnt.setARGB(255, 0, 100, 200);
//                        img.drawRect(cur_x, cur_yl, cur_x+bw, (int)(height*0.4), pnt);
//                        img.drawText("" + (int)(pNN_long[b]/2.55), cur_x, cur_yl-15, pnt);
                    }

/*                    if(prev_peak_cnt > 0)
                    {
                        pnt.setARGB(255, 0, 200, 100);
                        img.drawRect(width-20, height/2, width, height/2 - 100, pnt);
                    }*/
                    int bpm = dev_BPM;
                    long tm = System.currentTimeMillis();
                    if(tm - last_bpm_add > 10000)
                    {
                        last_bpm_add = tm;
                        for(int x = 0; x < bpm_length-1; x++)
                            bpm_chart[x] = bpm_chart[x+1];
                        bpm_chart[bpm_length-1] = bpm;
                    }
                    pnt.setARGB(255, 30, 200, 70);
                    pnt.setTextSize(35);
                    img.drawText("BPM: " + dev_BPM, (float)(width*0.1), (float)(height*0.45), pnt);
                    img.drawText("SDNN: " + dev_SDNN, (float)(width*0.35), (float)(height*0.45), pnt);
                    img.drawText("RMSSD: " + dev_RMSSD, (float)(width*0.65), (float)(height*0.45), pnt);

                    if(save_on == 1)
                    {
                        pnt.setTextSize(20);
                        pnt.setARGB(255, 130, 100, 70);
                        img.drawText("stop recording", (float) (width * 0.3), (float) (height * 0.9), pnt);
                    }
                    else {
                        pnt.setTextSize(20);
                        pnt.setARGB(255, 30, 200, 70);
                        img.drawText("start recording", (float) (width * 0.3), (float) (height * 0.9), pnt);
                    }
/*                    for(int p = 0; p < bpm_length; p++)
                    {
                        int cur_x = p * width / bpm_length;
                        int cur_y = (int)(height*0.5 - 0.3*height * ((float)bpm_chart[p] / 150.0));
//                        if(bpm_chart[p] < 70)
                        pnt.setARGB(255, 60, 155, 80);
                        if(p > 0)
                            img.drawLine(prev_x, prev_y, cur_x, cur_y, pnt);
                        prev_x = cur_x;
                        prev_y = cur_y;
                    }
                    int lvl_50 = (int)(height*0.5 - 0.3*height * (50.0 / 150.0));
                    int lvl_100 = (int)(height*0.5 - 0.3*height * (100.0 / 150.0));
                    int lvl_150 = (int)(height*0.5 - 0.3*height * (150.0 / 150.0));
                    pnt.setARGB(255, 0, 0, 150);
                    img.drawLine(0, lvl_50, width, lvl_50, pnt);
                    pnt.setARGB(255, 150, 150, 0);
                    img.drawLine(0, lvl_100, width, lvl_100, pnt);
                    pnt.setARGB(255, 150, 0, 0);
                    img.drawLine(0, lvl_150, width, lvl_150, pnt);*/
                    sv.getHolder().getSurface().unlockCanvasAndPost(img);

                }
            });
        }
    };
    private class MyScanCallback extends ScanCallback {
        Context ctx;
        void set_context(Context c)
        {
            ctx = c;
        };
        @Override
        public void onScanResult(int callbackType, ScanResult result) {

            try {
                if (parse_result(result.getScanRecord().getBytes(), result.getDevice().getType(), result.getRssi(), result.getDevice().getAddress()) > 0) {
                    ;
//                MyBluetoothGattCallback gatt_cb = new MyBluetoothGattCallback();
//                result.getDevice().connectGatt(ctx, false, gatt_cb);
                }
            }
            catch (Exception e) {
                e.getCause();
            }
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
/*            String str = "";
            for (ScanResult sr : results) {
                str += sr;
            }
            TextView txt = (TextView)findViewById(R.id.scanText);
            txt.setText(str);*/
        }

        @Override
        public void onScanFailed(int errorCode) {
//            TextView txt = (TextView)findViewById(R.id.scanText);
//            txt.setText("failed: " + errorCode);
        }
    };
    MyScanCallback mScanCallback = null;
    void scan_bluetooth() {
        if (!mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }

//        mBluetoothAdapter.startLeScan(mLeScanCallback);
        if(mScanCallback == null) {
            settings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                    .build();
            filters = new ArrayList<ScanFilter>();
//            filters = new ScanFilter.Builder().setServiceUuid(ParcelUuid.fromString("")).build();
            mScanCallback = new MyScanCallback();
            mScanCallback.set_context(this);
            mLEScanner.startScan(filters, settings, mScanCallback);
//            mLEScanner.startScan(mScanCallback);
            timer = new Timer();
            timer.schedule(new drawUITask(), 100,40);
        }
        else {
            stopScanning();
            timer.cancel();
            timer.purge();
        }
//        mBluetoothAdapter.getBluetoothLeScanner().startScan(null, ScanSettings.SCAN_MODE_LOW_LATENCY, mLeScanCallback);
    };

    public boolean touch_process(MotionEvent e){
        int tx = (int) Math.floor(e.getX());
        int ty = (int) Math.floor(e.getY());
        SurfaceView sv = (SurfaceView)findViewById(R.id.surfaceView);
        float width = sv.getWidth();
        float height = sv.getHeight();

        float x = (float)tx / width;
        float y = (float)ty / height;
        if(y > 0.85 && x > 0.3 && x < 0.6)
        {
            if(save_on == 1)
            {
                save_on = 0;
            }
            else
            {
                init_file();
                save_on = 1;
            }

        }
        return false;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        init_bluetooth();
        scan_bluetooth();
        SurfaceView sv = (SurfaceView)findViewById(R.id.surfaceView);
        sv.setOnTouchListener(new View.OnTouchListener() {
            public boolean onTouch(View v, MotionEvent event) {
                touch_process(event);
                return false;
            }
        });
/*        Button bt_start = (Button)findViewById(R.id.bt_start);
        bt_start.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                TextView txt = (TextView)findViewById(R.id.baseText);
                Button bt_start = (Button)findViewById(R.id.bt_start);
//                txt.setText("ok : " + cnt);
                cnt++;
                if(bt_inited < 1) {
                    init_bluetooth();
                    bt_start.setText("Scan");
                    return;
                }
                if(mScanCallback == null)
                    bt_start.setText("Stop");
                else
                    bt_start.setText("Scan");
                scan_bluetooth();
            }
        });*/

    }

}
