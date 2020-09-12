package com.ultimaterobotics.uecgmonitor4_2;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.provider.MediaStore;
import android.support.annotation.Nullable;
import android.support.v4.app.NotificationCompat;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.AlertDialog;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import static android.app.Activity.RESULT_OK;
import static android.support.v4.app.ActivityCompat.startActivityForResult;

public class ble_uecg_service extends Service {

    public static final int UECG_ALIVE_INDICATOR = 10;

    public static final int UECG_PING_REQUEST = 100;
    public static final int UECG_PING_RESPONSE = 101;

    public static final int UECG_CLOSE_REQUEST = 110;
    public static final int UECG_CLOSE_RESPONSE = 111;

    public static final int UECG_STATE_ARRAY_REQUEST = 120;
    public static final int UECG_STATE_ARRAY_RESPONSE = 121;

    public static final int UECG_SAVE_START_REQUEST = 130;
    public static final int UECG_SAVE_STOP_REQUEST = 131;

    private ble_uecg_service uecg_service;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothManager bluetoothManager;
    private BluetoothLeAdvertiser mBluetoothLeAdvertiser;
    int bt_inited = -1;
    int first_run = 1;
    int had_err = 0;
    int had_data = 0;
    private static final int REQUEST_ENABLE_BT = 1;

    private BluetoothLeScanner mLEScanner;
    private ScanSettings settings;
    private List<ScanFilter> filters;

    String dev_mac = "";
    private Timer timer;

    private boolean stop_requested = false;

    void stopScanning() {
        if(mLEScanner != null && mScanCallback != null)
            mLEScanner.stopScan(mScanCallback);
//        mScanCallback = null;
    }
    void init_bluetooth() {
        // Initializes Bluetooth adapter.
        bluetoothManager =
                (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = bluetoothManager.getAdapter();
//        mBluetoothAdapter.disable();
        // Code here executes on main thread after user presses button
        if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            if(bt_inited == -1) {
                bt_inited = 0;
                Log.e("uECG", "ble not enabled");
                if(first_run == 1) {
                    Intent btIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                    btIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    getApplicationContext().startActivity(btIntent);
                    first_run = 0;
                }
                else
                {
                    mBluetoothAdapter.enable();
                }
//                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
//                startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            }
        }
        else {

            mLEScanner = mBluetoothAdapter.getBluetoothLeScanner();
            bt_inited = 1;
            Log.e("uECG", "ble enabled");
        }
    };
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // Check which request we're responding to
        if (requestCode == REQUEST_ENABLE_BT) {
            // Make sure the request was successful
            if (resultCode == RESULT_OK) {
                mLEScanner = mBluetoothAdapter.getBluetoothLeScanner();
                bt_inited = 1;
            }
        }
    };
    ECG_processor ecg_processor = new ECG_processor();
    RR_processor rr_processor = new RR_processor();
    uecg_ble_parser uecg_parser = new uecg_ble_parser();
    BPM_processor bpm_processor = new BPM_processor();

    float[] save_values = new float[500];
    long[] save_uid = new long[500];
    int[] save_valid = new int[500];
    int max_save_len = 500;

    int prev_pack_id = 0;
    int batt = 0;
    int draw_skip = 0;
    int[] bpm_chart = new int[100];
    int bpm_length = 100;
    long last_bpm_add = 0;

    float lost_points_avg = 0;
    float total_points_avg = 0;

    int total_packs = 0;

    int dev_BPM = 0;
    int dev_SDNN = 0;
    int dev_RMSSD = 0;
    float calc_SDNN = 0;
    int dev_skin = 0;
    float dev_ax = 0;
    float dev_ay = 0;
    float dev_az = 0;
    int dev_steps = 0;
    int dev_gg = 0;
    int dev_lastRR_id = -1;

    int rec_uid = 0;
    int unsaved_cnt = 0;

    int version_id = -1;

    int[] pNN_short = new int[32];
    int bins_count = 1;

    File log_file = null;
    File log_file_ecg = null;
    FileOutputStream file_out_stream;
    FileOutputStream file_out_stream_ecg;

    int save_on = 0;
    long save_start_time = 0;

    int had_new_RR = 0;

    int parse_result(byte[] scanRecord, int type, int rssi, String mac)
    {
        if(mac.contains("BA:BE"))
        {
            if (rssi > -70 && dev_mac.length() < 2) {
                dev_mac = mac;
                restart_scan_bluetooth();
                return 0;
            }
        }
        if(mac.equals(dev_mac))
            if(uecg_parser.parse_record(scanRecord) > 0)
            {
                if(uecg_parser.parsed_state.has_batt)
                    batt = uecg_parser.parsed_state.batt;
                if(uecg_parser.parsed_state.has_BPM)
                    dev_BPM = uecg_parser.parsed_state.BPM;
                if(uecg_parser.parsed_state.has_SDNN)
                    dev_SDNN = uecg_parser.parsed_state.SDNN;
                if(uecg_parser.parsed_state.has_RMSSD)
                    dev_RMSSD = uecg_parser.parsed_state.RMSSD;
                if(uecg_parser.parsed_state.has_GSR)
                    dev_skin = uecg_parser.parsed_state.GSR;
                if(uecg_parser.parsed_state.has_accel) {
                    dev_ax = uecg_parser.parsed_state.ax;
                    dev_ay = uecg_parser.parsed_state.ay;
                    dev_az = uecg_parser.parsed_state.az;
                }
                if(uecg_parser.parsed_state.has_steps)
                    dev_steps = uecg_parser.parsed_state.steps;
                if(uecg_parser.parsed_state.pNN_bin1_id >= 0) {
                    pNN_short[uecg_parser.parsed_state.pNN_bin1_id] = uecg_parser.parsed_state.pNN_bin1;
                    pNN_short[uecg_parser.parsed_state.pNN_bin2_id] = uecg_parser.parsed_state.pNN_bin2;
                    pNN_short[uecg_parser.parsed_state.pNN_bin3_id] = uecg_parser.parsed_state.pNN_bin3;
                    if(uecg_parser.parsed_state.pNN_bin1_id > bins_count)
                        bins_count = uecg_parser.parsed_state.pNN_bin1_id;
                    if(uecg_parser.parsed_state.pNN_bin2_id > bins_count)
                        bins_count = uecg_parser.parsed_state.pNN_bin2_id;
                    if(uecg_parser.parsed_state.pNN_bin3_id > bins_count)
                        bins_count = uecg_parser.parsed_state.pNN_bin3_id;
                }
                had_new_RR = 0;
                if(uecg_parser.parsed_state.has_RR) {
                    if(uecg_parser.parsed_state.RR_id != dev_lastRR_id)
                    {
                        had_new_RR = 1;
                        rr_processor.push_data(uecg_parser.parsed_state.RR_cur, uecg_parser.parsed_state.RR_prev);
                        dev_lastRR_id = uecg_parser.parsed_state.RR_id;
                        float d_rr = uecg_parser.parsed_state.RR_cur - uecg_parser.parsed_state.RR_prev;
                        calc_SDNN *= 0.99;
                        calc_SDNN += 0.01 * d_rr * d_rr;
                        if(save_on == 1)
                        {
                            String out_buf;
                            long tm = System.currentTimeMillis();
                            out_buf = tm + ";" + uecg_parser.parsed_state.RR_cur + ";" + uecg_parser.parsed_state.RR_prev + ";" + dev_skin + ";" + dev_BPM + ";" + dev_steps + ";" + dev_gg + ";" + dev_ax + ";" + dev_ay + ";" + dev_az + "\n";
                            String ecg_buf = "";
                            int save_cnt = unsaved_cnt;
                            unsaved_cnt = 0;
                            if(save_cnt > max_save_len-2) save_cnt = max_save_len-2;
                            save_cnt = ecg_processor.get_last_data_count(save_cnt, save_values, save_uid, save_valid);

                            for(int n = 0; n < save_cnt; n++)
                            {
                                ecg_buf += save_uid[n] + ";" + save_values[n] + ";" + save_valid[n] + "\n";
                            }
                            try {
                                file_out_stream = new FileOutputStream(log_file, true);
                                file_out_stream_ecg = new FileOutputStream(log_file_ecg, true);

                                try {
                                    file_out_stream.write(out_buf.getBytes());
                                    file_out_stream_ecg.write(ecg_buf.getBytes());
                                } finally {
                                    file_out_stream.close();
                                    file_out_stream_ecg.close();
                                }
                            }
                            catch(IOException ex)
                            {
                                Log.e("uECG service", "file writing exception: " + ex.getMessage());
//                                ;
//                                Log.e(ex.getMessage());
                            }
                        }
                    }

                }

                int d_id = uecg_parser.parsed_state.pack_id - prev_pack_id;
                prev_pack_id = uecg_parser.parsed_state.pack_id;
                if(d_id < 0) d_id += 256;
                draw_skip++;
                if(d_id == 0)
                    return 1;

                total_packs++;
                unsaved_cnt += d_id;

                ecg_processor.push_data(d_id, uecg_parser.parsed_state.ECG_values, uecg_parser.parsed_state.ECG_values_count);

                bpm_processor.push_data(dev_BPM, dev_steps, dev_skin, rr_processor.get_hrv_parameter(), dev_ax, dev_ay, dev_az);

                return 1;
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

                Context ctx = getApplicationContext();

                if (android.os.Build.VERSION.SDK_INT > Build.VERSION_CODES.P){
                    File dir = new File(ctx.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS), "uECG");
                    boolean res = dir.mkdirs();

                    log_file = new File(ctx.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS), log_name);
                    log_file_ecg = new File(ctx.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS), log_name_ecg);

                    log_file.createNewFile();
                    log_file_ecg.createNewFile();
                } else{
                    File dir = new File(Environment.getExternalStoragePublicDirectory(
                            Environment.DIRECTORY_DOCUMENTS), "uECG");
                    log_file = new File(Environment.getExternalStoragePublicDirectory(
                            Environment.DIRECTORY_DOCUMENTS), log_name);
                    log_file_ecg = new File(Environment.getExternalStoragePublicDirectory(
                            Environment.DIRECTORY_DOCUMENTS), log_name_ecg);
                }

                save_start_time = System.currentTimeMillis();
                ecg_processor.set_save_time(save_start_time);

                return 1;
            }
            else
            {
                Log.e("uECG service", "path: " + Environment.DIRECTORY_DOCUMENTS + ", media state: " + state);
            }
        }catch(Exception ex)
        {
            Log.e("uECG service", "File save: got exception:" + ex.toString() + " when trying to access " + Environment.DIRECTORY_DOCUMENTS + ", media state: " + state);
//            Log.e(ex.getMessage());
        }
        return 0;
    };

    private class MyScanCallback extends ScanCallback {
        Context ctx;
        void set_context(Context c)
        {
            ctx = c;
        };
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            had_data = 1;
            try {
//                if(result.getDevice().getAddress().contains("BA:BE"))
//                {
//                    if (result.getRssi() > -70 && dev_mac.length() < 2) {
//                        dev_mac = result.getDevice().getAddress();
//                        restart_scan_bluetooth();
//                    }
//                }
                no_scan_data_cnt = 0;
                parse_result(result.getScanRecord().getBytes(), result.getDevice().getType(), result.getRssi(), result.getDevice().getAddress());
                Intent intent = new Intent("uECG_DATA");
                // You can also include some extra data.
//                intent.putExtra("Status", msg);
                Bundle b = new Bundle();
                b.putInt("batt", batt);
                b.putInt("dev_BPM", dev_BPM);
                b.putInt("dev_SDNN", dev_SDNN);
                b.putInt("dev_RMSSD", dev_RMSSD);
                b.putInt("dev_skin", dev_skin);
                b.putFloat("dev_ax", dev_ax);
                b.putFloat("dev_ay", dev_ay);
                b.putFloat("dev_az", dev_az);
                b.putInt("dev_steps", dev_steps);
                b.putInt("dev_BPM", dev_BPM);
                b.putInt("dev_pNN_bins", bins_count);
                b.putIntArray("pNNs", pNN_short);

                b.putInt("has_RR", had_new_RR);
                if(had_new_RR > 0)
                {
                    b.putInt("RR_cur", uecg_parser.parsed_state.RR_cur);
                    b.putInt("RR_prev", uecg_parser.parsed_state.RR_prev);
                }

                b.putInt("pack_id", uecg_parser.parsed_state.pack_id);
                b.putInt("RR_count", uecg_parser.parsed_state.ECG_values_count);
                b.putIntArray("RR", uecg_parser.parsed_state.ECG_values);

                intent.putExtra("BLE_data", b);

                intent.addCategory(Intent.CATEGORY_DEFAULT);
                sendBroadcast(intent);

                last_scan_restart = System.currentTimeMillis();
            }
            catch (Exception e) {
                e.getCause();
            }

        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            Log.e("uECG service", "scan result list");
        }

        @Override
        public void onScanFailed(int errorCode) {
            had_err++;
            Log.e("uECG service", "scan failed " + errorCode);
        }
    };
    MyScanCallback mScanCallback = null;
    long last_scan_restart = 0;
    void restart_scan_bluetooth()
    {
        Log.e("uECG", "scan restart");
        last_scan_restart = System.currentTimeMillis();
        if(mLEScanner == null)
        {
            mBluetoothAdapter.disable();
            bt_inited = -1;
            return;
        }
        mLEScanner.stopScan(mScanCallback);
        ScanSettings settings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .setCallbackType(ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
                .setMatchMode(ScanSettings.MATCH_MODE_AGGRESSIVE)
                .setNumOfMatches(ScanSettings.MATCH_NUM_MAX_ADVERTISEMENT)
//                    .setPhy(ScanSettings.PHY_LE_ALL_SUPPORTED)
                .setReportDelay(0L)
                .build();
        filters = new ArrayList<ScanFilter>();
        ScanFilter uecg_name = new ScanFilter.Builder().setDeviceName("uECG").build();
        if(dev_mac.length() > 2) {
            ScanFilter uecg_mac = new ScanFilter.Builder().setDeviceAddress(dev_mac).build();
            filters.add(uecg_mac);
        }
        else
            filters.add(uecg_name);
        mLEScanner.startScan(filters, settings, mScanCallback);
    }
    void scan_bluetooth() {
        Log.e("uECG", "scan start, ble addr: " + mBluetoothAdapter.getAddress());
        mBluetoothAdapter.enable();
        if(mLEScanner == null)
        {
            mLEScanner = mBluetoothAdapter.getBluetoothLeScanner();
            if(mLEScanner == null) {
                Log.e("uECG", "mLEScanner still null");
                return;
            }
        }
        if(mScanCallback == null) {
            Log.e("uECG", "no scan callback");
            ScanSettings settings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                    .setCallbackType(ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
                    .setMatchMode(ScanSettings.MATCH_MODE_AGGRESSIVE)
                    .setNumOfMatches(ScanSettings.MATCH_NUM_MAX_ADVERTISEMENT)
//                    .setPhy(ScanSettings.PHY_LE_ALL_SUPPORTED)
                    .setReportDelay(0L)
                    .build();
            filters = new ArrayList<ScanFilter>();
            ScanFilter uecg_name = new ScanFilter.Builder().setDeviceName("uECG").build();
            if(dev_mac.length() > 2) {
                ScanFilter uecg_mac = new ScanFilter.Builder().setDeviceAddress(dev_mac).build();
                filters.add(uecg_mac);
            }
            else
                filters.add(uecg_name);
            mScanCallback = new ble_uecg_service.MyScanCallback();
            mScanCallback.set_context(getApplicationContext());
            Log.e("uECG", "starting scan");
            mLEScanner.startScan(filters, settings, mScanCallback);
            bt_inited = 10;
        }
        else {
            Log.e("uECG", "scan callback exists");
            if(bt_inited >= 1 && mScanCallback != null && mLEScanner != null) {
                mLEScanner.stopScan(mScanCallback);
                bt_inited = 1;
            }
            mScanCallback = null;
//            stopScanning();
//            timer.cancel();
//            timer.purge();
        }
//        mBluetoothAdapter.getBluetoothLeScanner().startScan(null, ScanSettings.SCAN_MODE_LOW_LATENCY, mLeScanCallback);
    };
    int no_scan_data_cnt = 0;
    class BLE_Checker extends TimerTask {

        @Override
        public void run() {
            Intent intent = new Intent("uECG_SIGNAL");
            intent.putExtra("signal", ble_uecg_service.UECG_ALIVE_INDICATOR);
            intent.addCategory(Intent.CATEGORY_DEFAULT);
            sendBroadcast(intent);

//            Log.e("uECG service", "timer event");
            if(stop_requested) {
                timer.cancel();
                stopScanning();
                stop_requested = false;
                Log.e("uECG service", "stop processed");
//                uecg_service.stopForeground(true);
                stopForeground(true);
                return;
            }
            if (bt_inited < 1) {
                init_bluetooth();
            } else if (bt_inited < 10)
                scan_bluetooth();

            long ms = System.currentTimeMillis();
            if(ms - last_scan_restart > 2000 && bt_inited == 10) {
                no_scan_data_cnt++;
                if(no_scan_data_cnt > 4)
                {
                    stopScanning();
                    bt_inited = -1;
                    no_scan_data_cnt = 0;
                    return;
                }
                restart_scan_bluetooth();
            }

            if(had_err >= 3 && had_data == 0) {
                mBluetoothAdapter.disable();
                bt_inited = -1;
            }
            if(had_data > 0)
                had_err = 0;
            had_data = 0;
        };

    };


    public static final String CHANNEL_ID = "uECGServiceChannel";
    @Override
    public void onCreate() {
        Log.e("uECG service", "trying to create");
        super.onCreate();
        uecg_service = this;
        Log.e("uECG service", "created");
    }
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.e("uECG service", "on start");
        String input = intent.getStringExtra("inputExtra");
        createNotificationChannel();
        Intent notificationIntent = new Intent(getApplicationContext(), MainActivity.class);
        notificationIntent.setAction("main_action");
        notificationIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(this,
                0, notificationIntent, 0);
        Notification notification = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle("uECG receiver")
                .setContentText(input)
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pendingIntent)
                .build();
        startForeground(1, notification);

        BroadcastReceiver ecg_service_info = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                int sig = intent.getIntExtra ("signal", 0);
                if(sig == UECG_CLOSE_REQUEST)
                {
                    Intent intent_out = new Intent("uECG_SIGNAL");
                    intent_out.putExtra("signal", UECG_CLOSE_RESPONSE);
                    intent_out.addCategory(Intent.CATEGORY_DEFAULT);
                    sendBroadcast(intent_out);
                    Log.e("uECG service", "stop request");

                    stop_requested = true;
                }

                if(sig == UECG_PING_REQUEST)
                {
                    Intent intent_out = new Intent("uECG_SIGNAL");
                    intent_out.putExtra("signal", UECG_PING_RESPONSE);
                    intent_out.addCategory(Intent.CATEGORY_DEFAULT);
                    sendBroadcast(intent_out);
                }

                if(sig == UECG_SAVE_START_REQUEST)
                {
                    init_file();
                    save_on = 1;
                }
                if(sig == UECG_SAVE_STOP_REQUEST)
                {
                    save_on = 0;
                }

                if(sig == UECG_STATE_ARRAY_REQUEST)
                {
                    Log.e("uECG", "state array request");
                    Intent intent_out = new Intent("uECG_SIGNAL");
                    intent_out.putExtra("signal", UECG_STATE_ARRAY_RESPONSE);
                    intent_out.addCategory(Intent.CATEGORY_DEFAULT);
                    Bundle b = new Bundle();

                    b.putInt("file_save_state", save_on);
                    b.putInt("RR_count", rr_processor.buf_size);
                    b.putInt("RR_bufpos", rr_processor.buf_cur_pos);
                    int st_high = (int)(rr_processor.start_time>>24);
                    int st_low = (int)(rr_processor.start_time&0xFFFFFF);
                    b.putInt("RR_start_time_h", st_high);
                    b.putInt("RR_start_time_l", st_low);
                    b.putFloatArray("RR_cur", rr_processor.RR_cur_hist);
                    b.putFloatArray("RR_prev", rr_processor.RR_prev_hist);
                    b.putInt("ECG_count", ecg_processor.hist_depth);
                    b.putFloatArray("ECG_data", ecg_processor.points_hist);
                    b.putIntArray("ECG valid", ecg_processor.points_valid);
                    int[] uids_high = new int[ecg_processor.hist_depth];
                    int[] uids_low = new int[ecg_processor.hist_depth];
                    for(int x = 0; x < ecg_processor.hist_depth; x++) {
                        uids_high[x] = (int)(ecg_processor.points_uid[x] >> 24);
                        uids_low[x] = (int)(ecg_processor.points_uid[x]&0xFFFFFF);
                    }
                    b.putIntArray("ECG uid_h", uids_high);
                    b.putIntArray("ECG uid_l", uids_low);

                    b.putFloat("HRV_parameter", rr_processor.get_hrv_parameter());

                    b.putIntArray("bpm_processor_int", bpm_processor.pack_int_array());
                    b.putFloatArray("bpm_processor_float", bpm_processor.pack_float_array());
                    intent_out.putExtra("uecg_data", b);
                    sendBroadcast(intent_out);
                }

            }
        };

        IntentFilter filter = new IntentFilter();
        filter.addAction("uECG_SERVICE_SIGNAL");
        filter.addCategory(Intent.CATEGORY_DEFAULT);
        registerReceiver(ecg_service_info, filter);

        timer = new Timer();
        timer.schedule(new ble_uecg_service.BLE_Checker(), 100, 500);

        return START_NOT_STICKY;
    }
    @Override
    public void onDestroy() {
        timer.cancel();
        stopScanning();

        super.onDestroy();
    }
    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel serviceChannel = new NotificationChannel(
                    CHANNEL_ID,
                    "uECG Service Channel",
                    NotificationManager.IMPORTANCE_DEFAULT
            );
            NotificationManager manager = getSystemService(NotificationManager.class);
            manager.createNotificationChannel(serviceChannel);
        }
    }
}