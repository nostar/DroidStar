package org.dudetronics.droidstar;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.BitmapFactory;
import android.app.NotificationChannel;

public class NotificationClient
{
    public static void notify(Context context, String message) {
        try {
            NotificationManager m_notificationManager = (NotificationManager)
                    context.getSystemService(Context.NOTIFICATION_SERVICE);

            Notification.Builder m_builder;
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
                int importance = NotificationManager.IMPORTANCE_DEFAULT;
                NotificationChannel notificationChannel;
                notificationChannel = new NotificationChannel("Qt", "Qt Notifier", importance);
                m_notificationManager.createNotificationChannel(notificationChannel);
                m_builder = new Notification.Builder(context, notificationChannel.getId());
            } else {
                m_builder = new Notification.Builder(context);
            }

            Bitmap icon = BitmapFactory.decodeResource(context.getResources(), R.drawable.icon);
            m_builder.setSmallIcon(R.drawable.icon)
                    .setLargeIcon(icon)
                    .setContentTitle("DroidStar")
                    .setContentText(message)
                    .setDefaults(Notification.DEFAULT_SOUND)
                    .setColor(Color.GREEN)
                    .setAutoCancel(true);

                Notification n = m_builder.getNotification();
                n.flags |= Notification.FLAG_NO_CLEAR | Notification.FLAG_ONGOING_EVENT;
                m_notificationManager.notify(0, m_builder.build());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void denotify(Context context) {
        try {
            NotificationManager m_notificationManager = (NotificationManager)
                    context.getSystemService(Context.NOTIFICATION_SERVICE);

            m_notificationManager.cancelAll();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
