package org.dudetronics.droidstar;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;
import android.util.Log;

public class NotificationClient extends Service
{
    private static final String TAG = "DroidStarService";
    private static final String CHANNEL_ID = "droidstar_audio";
    private static final String CHANNEL_NAME = "DroidStar connection";
    private static final String EXTRA_MESSAGE =
            "org.dudetronics.droidstar.extra.MESSAGE";
    private static final int NOTIFICATION_ID = 1001;

    private PowerManager.WakeLock wakeLock;

    public static void startDroidStarService(Context context, String message)
    {
        Context appContext = context.getApplicationContext();
        Intent intent = new Intent(appContext, NotificationClient.class);
        intent.putExtra(EXTRA_MESSAGE, message);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            appContext.startForegroundService(intent);
        } else {
            appContext.startService(intent);
        }
    }

    public static void updateNotification(Context context, String message)
    {
        Context appContext = context.getApplicationContext();
        createNotificationChannel(appContext);

        NotificationManager manager = (NotificationManager)
                appContext.getSystemService(Context.NOTIFICATION_SERVICE);
        if (manager != null) {
            manager.notify(
                    NOTIFICATION_ID,
                    buildNotification(appContext, message));
        }
    }

    public static void stopDroidStarService(Context context)
    {
        Context appContext = context.getApplicationContext();
        appContext.stopService(new Intent(appContext, NotificationClient.class));
    }

    @Override
    public void onCreate()
    {
        super.onCreate();
        createNotificationChannel(this);
        acquireWakeLock();
        Log.i(TAG, "Foreground audio service created");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        String message = "DroidStar is connected";
        if (intent != null) {
            String requestedMessage = intent.getStringExtra(EXTRA_MESSAGE);
            if (requestedMessage != null && !requestedMessage.isEmpty()) {
                message = requestedMessage;
            }
        }

        enterForeground(buildNotification(this, message));
        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy()
    {
        releaseWakeLock();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            stopForeground(STOP_FOREGROUND_REMOVE);
        } else {
            stopForeground(true);
        }

        Log.i(TAG, "Foreground audio service destroyed");
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent)
    {
        return null;
    }

    private void enterForeground(Notification notification)
    {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                startForeground(
                        NOTIFICATION_ID,
                        notification,
                        ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK
                                | ServiceInfo.FOREGROUND_SERVICE_TYPE_MICROPHONE);
            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                startForeground(
                        NOTIFICATION_ID,
                        notification,
                        ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
            } else {
                startForeground(NOTIFICATION_ID, notification);
            }
        } catch (SecurityException microphonePermissionError) {
            // Receiving audio can still work when microphone access was denied.
            // Fall back to a playback-only foreground service rather than
            // allowing the service start to fail completely.
            Log.w(TAG,
                    "Microphone foreground-service type unavailable; "
                            + "falling back to media playback only",
                    microphonePermissionError);

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                startForeground(
                        NOTIFICATION_ID,
                        notification,
                        ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
            } else {
                startForeground(NOTIFICATION_ID, notification);
            }
        }
    }

    private void acquireWakeLock()
    {
        PowerManager powerManager =
                (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (powerManager == null) {
            Log.w(TAG, "PowerManager unavailable; no partial wake lock");
            return;
        }

        wakeLock = powerManager.newWakeLock(
                PowerManager.PARTIAL_WAKE_LOCK,
                "DroidStar:AudioService");
        wakeLock.setReferenceCounted(false);
        wakeLock.acquire();
    }

    private void releaseWakeLock()
    {
        if (wakeLock != null && wakeLock.isHeld()) {
            wakeLock.release();
        }
        wakeLock = null;
    }

    private static void createNotificationChannel(Context context)
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            return;
        }

        NotificationManager manager = (NotificationManager)
                context.getSystemService(Context.NOTIFICATION_SERVICE);
        if (manager == null) {
            return;
        }

        NotificationChannel channel = new NotificationChannel(
                CHANNEL_ID,
                CHANNEL_NAME,
                NotificationManager.IMPORTANCE_LOW);
        channel.setDescription(
                "Keeps DroidStar audio and network processing active");
        channel.setSound(null, null);
        channel.enableVibration(false);
        channel.setShowBadge(false);
        manager.createNotificationChannel(channel);
    }

    private static Notification buildNotification(
            Context context,
            String message)
    {
        Notification.Builder builder;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder = new Notification.Builder(context, CHANNEL_ID);
        } else {
            builder = new Notification.Builder(context);
            builder.setPriority(Notification.PRIORITY_LOW);
        }

        Intent launchIntent =
                context.getPackageManager()
                        .getLaunchIntentForPackage(context.getPackageName());
        if (launchIntent != null) {
            launchIntent.addFlags(
                    Intent.FLAG_ACTIVITY_CLEAR_TOP
                            | Intent.FLAG_ACTIVITY_SINGLE_TOP);
            PendingIntent contentIntent = PendingIntent.getActivity(
                    context,
                    0,
                    launchIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT
                            | PendingIntent.FLAG_IMMUTABLE);
            builder.setContentIntent(contentIntent);
        }

        Bitmap largeIcon = BitmapFactory.decodeResource(
                context.getResources(),
                R.drawable.icon);

        builder.setSmallIcon(R.drawable.icon)
                .setLargeIcon(largeIcon)
                .setContentTitle("DroidStar")
                .setContentText(message)
                .setColor(Color.GREEN)
                .setCategory(Notification.CATEGORY_SERVICE)
                .setVisibility(Notification.VISIBILITY_PUBLIC)
                .setOnlyAlertOnce(true)
                .setOngoing(true)
                .setAutoCancel(false)
                .setShowWhen(false);

        return builder.build();
    }
}
