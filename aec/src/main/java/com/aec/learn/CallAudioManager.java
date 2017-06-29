package com.aec.learn;

import android.content.Context;
import android.media.AudioManager;
import android.os.Build;
import android.util.Log;

import java.io.IOException;
import java.net.SocketException;

public class CallAudioManager {

  private static final String TAG = "AEC-" + CallAudioManager.class.getSimpleName();

  static {
    System.loadLibrary("redphone-audio");
  }

  private final long handle;

  public CallAudioManager(String remoteHost, int remotePort, String session)
          throws IOException {
    Log.e(TAG, "CallAudioManager");
    this.handle = create(Build.VERSION.SDK_INT, remoteHost, remotePort, session);
  }

  public void setMute(boolean enabled) {
    setMute(handle, enabled);
  }

  public void start(Context context) throws IOException {
    Log.e(TAG, "start");
    ((AudioManager) context.getSystemService(Context.AUDIO_SERVICE)).setMode(
        AudioManager.MODE_IN_COMMUNICATION);
      start(handle);

  }

  public void terminate() {
    stop(handle);
    dispose(handle);
  }

  private native long create(int androidSdkVersion,  /** Android 版本*/
      String serverIpString, int serverPort, String session) throws IOException;

  private native void start(long handle) throws IOException;

  private native void setMute(long handle, boolean enabled);

  private native void stop(long handle);

  private native void dispose(long handle);
}
