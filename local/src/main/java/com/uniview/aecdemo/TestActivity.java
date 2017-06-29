package com.uniview.aecdemo;

import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Process;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.AppCompatButton;
import android.support.v7.widget.AppCompatEditText;
import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;
import com.aec.learn.CallAudioManager;
import com.aec.learn.Utils;

import java.io.IOException;

public class TestActivity extends AppCompatActivity implements View.OnClickListener {

  private static final String KEY_HOST = "HOST";

  CallAudioManager cm;

  private String host;
  private Integer port;
  private String session;

  private AppCompatEditText etHost;
  private AppCompatEditText etPort;
  private AppCompatEditText etSession;

  private AppCompatButton btStart;
  private AppCompatButton btStop;

  @Override protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.test_abc);

    etHost = (AppCompatEditText) super.findViewById(R.id.host);
    etPort = (AppCompatEditText) super.findViewById(R.id.port);

    btStart = (AppCompatButton) super.findViewById(R.id.start);
    btStart.setOnClickListener(this);
    btStop = (AppCompatButton) super.findViewById(R.id.stop);
    btStop.setOnClickListener(this);

    String host = Utils.readStr(this, KEY_HOST);
    if (!TextUtils.isEmpty(host)) {
      etHost.setText(host);
    }

    setVolumeControlStream(AudioManager.STREAM_VOICE_CALL);
    ((AudioManager) this.getSystemService(Context.AUDIO_SERVICE)).setSpeakerphoneOn(true);

    new Thread(new Runnable() {
      @Override public void run() {
        String host = Utils.getLocalIpAddress();
        if (TextUtils.isEmpty(host)) {
          host = "无法获取 IP 地址";
        } else {
          host = "设备 IP :" + host;
        }
        final String finalHost = host;
        runOnUiThread(new Runnable() {
          @Override public void run() {
            ((TextView) findViewById(R.id.tv_host)).setText(finalHost);
          }
        });
      }
    }).start();
  }

  public void start() {

    btStart.setEnabled(false);

    host = etHost.getText().toString();
    Utils.saveStr(this, KEY_HOST, host);

    port = Integer.valueOf(etPort.getText().toString());

    new Thread(new Runnable() {
      @Override public void run() {
        try {
          Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
          cm = new CallAudioManager(host, port, session);
          cm.start(TestActivity.this);
        } catch (final IOException e) {
          e.printStackTrace();
          runOnUiThread(new Runnable() {
            @Override public void run() {
              failed(e.getMessage() == null ? e.toString() : e.getMessage());
            }
          });
        }
      }
    }).start();

    btStop.setEnabled(true);
  }

  public void failed(@NonNull String msg) {
    Toast.makeText(TestActivity.this, msg, Toast.LENGTH_LONG).show();
    btStart.setEnabled(false);
    btStop.setEnabled(false);
  }

  public void stop() {
    btStop.setEnabled(false);
    new Thread(new Runnable() {
      @Override public void run() {
        cm.terminate();
        cm = null;
      }
    }).start();
    btStart.setEnabled(true);
  }

  @Override public void onClick(View v) {
    if (v == btStart) {
      start();
    } else if (v == btStop) {
      stop();
    }
  }
}
