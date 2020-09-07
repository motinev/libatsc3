package org.ngbp.libatsc3.android;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.MalformedURLException;

public class VBoxConnect extends Thread {

    private static Context mContext = null;
    private static final String TAG = "VBoxConnect";

    public static void toast(Context context, final String text) {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            public void run() {
                if (mContext != null)
                    Toast.makeText(mContext, text, Toast.LENGTH_LONG).show();
            }
        });
    }

    /* Start a thread to send http request to web server use HttpURLConnection object. */
    public static void startSendHttpRequestThread(final String reqUrl, final Context context) {
        mContext = context;

        Thread sendHttpRequestThread = new Thread() {
            @Override
            public void run() {
                // Maintain http url connection.
                HttpURLConnection httpConn = null;
                // Read text input stream.
                InputStreamReader isReader = null;
                // Read text into buffer.
                BufferedReader bufReader = null;
                // Save server response text.
                StringBuffer readTextBuf = new StringBuffer();

                try {
                    // Create a URL object use page url.
                    URL url = new URL(reqUrl);
                    // Open http connection to web server.
                    httpConn = (HttpURLConnection) url.openConnection();
                    // Set http request method to get.
                    httpConn.setRequestMethod("GET");
                    // Set connection timeout and read timeout value.
                    httpConn.setConnectTimeout(4000);
                    httpConn.setReadTimeout(4000);

                    int resCode = httpConn.getResponseCode();
                    if (resCode == 200)
                        toast(mContext, "Request was sent successfully to the VBox device");

                    // Get input stream from web url connection.
                    InputStream inputStream = httpConn.getInputStream();
                    // Create input stream reader based on url connection input stream.
                    isReader = new InputStreamReader(inputStream);
                    // Create buffered reader.
                    bufReader = new BufferedReader(isReader);
                    // Read line of text from server response.
                    String line = bufReader.readLine();

                    // Loop while return line is not null.
                    while (line != null) {
                        // Append the text to string buffer.
                        readTextBuf.append(line);
                        // Continue to read text line.
                        line = bufReader.readLine();
                    }
                    Log.d(TAG, "Replay to: " + httpConn.getURL().getQuery().toString());
                    Log.d(TAG, "Replay Body: " +readTextBuf.toString());

                } catch (MalformedURLException ex) {
                    Log.e(TAG, "MalformedURLException");
                    toast(mContext, "Failed to send Request: IP format Malformed");
                    //Log.e(TAG_HTTP_URL_CONNECTION, ex.getMessage(), ex);
                } catch (java.net.SocketTimeoutException ex) {
                    Log.e(TAG, "SocketTimeoutException");
                    toast(mContext, "Failed to send Request: Timeout - Can't connect to Vbox device (check IP)");
                    //Log.e(TAG_HTTP_URL_CONNECTION, ex.getMessage(), ex);
                } catch (IOException ex) {
                    Log.e(TAG, "IOException");
                    toast(mContext, "Failed to send Request: Error - IOException");
                    //Log.e(TAG_HTTP_URL_CONNECTION, ex.getMessage(), ex);
                } finally {
                    try {
                        if (bufReader != null) {
                            bufReader.close();
                            bufReader = null;
                        }
                        if (isReader != null) {
                            isReader.close();
                            isReader = null;
                        }
                        if (httpConn != null) {
                            httpConn.disconnect();
                            httpConn = null;
                        }
                    } catch (IOException ex) {
                        Log.e(TAG, ex.getMessage(), ex);
                    }
                }
            }
        };
        // Start the child thread to request web page.
        sendHttpRequestThread.start();
    }

    /* Send Tune http request */
    public static void sendVBoxTuneRequest(final String ip, final int freqMHz, final int plp, final Context context) {
        String MethodPath = "/cgi-bin/HttpControl/HttpControlApp?OPTION=1&Method=SetFrequency&TunerID=1&TunerType=ATSC&Bandwidth=6MHZ&AtscType=ATSC3";
        String reqUrl = "http://" + ip + MethodPath + "&Frequency=" + freqMHz * 1000 + "&Atsc3PlpId=" + plp;
        Log.d(TAG, reqUrl);
        // Send the Http request
        startSendHttpRequestThread(reqUrl, context);
    }

    /* Send Open Multicast Stream http request */
    public static void OpenVBoxMulticastStream(final String ip, final String DstIP, final int DstPort, final Context context) {
        String MethodPath = "/cgi-bin/HttpControl/HttpControlApp?OPTION=1&Method=OpenAtsc3MulticastStream&TunerID=1&Loopback=YES";
        String reqUrl = "http://" + ip + MethodPath + "&DstIP=" + DstIP + "&DstPort=" + DstPort;
        Log.d(TAG, reqUrl);
        // Send the Http request
        startSendHttpRequestThread(reqUrl, context);
    }

    /* Send Open All Multicast Stream http request - no filtering is allowed */
    public static void OpenVBoxAllMulticastStream(final String ip, final String DstIP, final int DstPort, final Context context) {
        String MethodPath = "/cgi-bin/HttpControl/HttpControlApp?OPTION=1&Method=OpenAtsc3MulticastStream&TunerID=1";
        String reqUrl = "http://" + ip + MethodPath + "&DstIP=All";
        Log.d(TAG, reqUrl);
        // Send the Http request
        startSendHttpRequestThread(reqUrl, context);
    }

    /* Send Close Multicast Stream http request */
    public static void CloseVBoxMulticastStream(final String ip, final Context context) {
        String MethodPath = "/cgi-bin/HttpControl/HttpControlApp?OPTION=1&Method=CloseAtsc3MulticastStream&TunerID=1";
        String reqUrl = "http://" + ip + MethodPath;
        Log.d(TAG, reqUrl);
        // Send the Http request
        startSendHttpRequestThread(reqUrl, context);
    }

    public static void AddVBoxMulticastStream(final String ip, final String DstIP, final int DstPort, final Context context) {
        String MethodPath = "/cgi-bin/HttpControl/HttpControlApp?OPTION=1&Method=AddAtsc3MulticastFilter&TunerID=1";
        String reqUrl = "http://" + ip + MethodPath + "&DstIP=" + DstIP + "&DstPort=" + DstPort;
        Log.d(TAG, reqUrl);
        // Send the Http request
        startSendHttpRequestThread(reqUrl, context);
    }

    public static void RemoveVBoxMulticastStream(final String ip, final String DstIP, final int DstPort, final Context context) {
        String MethodPath = "/cgi-bin/HttpControl/HttpControlApp?OPTION=1&Method=RemoveAtsc3MulticastFilter&TunerID=1";
        String reqUrl = "http://" + ip + MethodPath + "&DstIP=" + DstIP + "&DstPort=" + DstPort;
        Log.d(TAG, reqUrl);
        // Send the Http request
        startSendHttpRequestThread(reqUrl, context);
    }

}
