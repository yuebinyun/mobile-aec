package com.aec.learn;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.Locale;

public class Utils {

  private Utils() {
  }

  public static void e(Object o) {
    if (null == o) {
      Log.e(getFileLineMethod(), "null");
    } else {
      Log.e(getFileLineMethod(), o.toString());
    }
  }

  private static String getFileLineMethod() {
    StackTraceElement te = ((new Exception()).getStackTrace())[2];
    return String.format(Locale.CHINA, "(%s:%d)(Func:%s)", te.getFileName(), te.getLineNumber(),
        te.getMethodName());
  }

  public static void saveStr(Context context, String key, String value) {
    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(context).edit();
    e.putString(key, value);
    e.apply();
  }

  public static String readStr(Context context, String key) {
    return PreferenceManager.getDefaultSharedPreferences(context).getString(key, "");
  }

  public static String getLocalIpAddress() {
    try {
      for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces();
          en.hasMoreElements(); ) {
        NetworkInterface intf = en.nextElement();
        for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses();
            enumIpAddr.hasMoreElements(); ) {
          InetAddress inetAddress = enumIpAddr.nextElement();
          if (!inetAddress.isLoopbackAddress() && inetAddress instanceof Inet4Address) {
            return inetAddress.getHostAddress();
          }
        }
      }
    } catch (SocketException ex) {
      ex.printStackTrace();
    }
    return null;
  }
}
