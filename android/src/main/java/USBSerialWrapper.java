/*
	Copyright (C) 2019-2021 Doug McLain

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

package DroidStar;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.app.PendingIntent;
import android.content.Context;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDeviceConnection;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManagerTest;

public class USBSerialWrapper implements SerialInputOutputManagerTest.Listener {
    public static int counter = 0;
    public int id;
    public Context m_context;
    public int m_baudrate = 115200;
	public int m_databits = 8;
	public int m_stopbits = 1;
	public int m_parity = 0;
	public int m_flowcontrol = 0;
	public int m_rts = 0;
	public String m_devicename;
	
    UsbSerialPort m_port;
    SerialInputOutputManagerTest usbIoManager;
    private static final String ACTION_USB_PERMISSION = "org.dudetronics.droidstar.USB_PERMISSION";
    IntentFilter m_filter = new IntentFilter(ACTION_USB_PERMISSION);
    
    UsbManager m_manager;
    UsbSerialDriver m_driver;
  
    public BroadcastReceiver m_usbReceiver = new BroadcastReceiver() {
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			System.out.println("BroadcastReceiver() onReceive() action == " + action);
			if (ACTION_USB_PERMISSION.equals(action)) {
				if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
					System.out.println("USB Permission Granted");
					if(open_device() > 0) device_open();
				}
				else {
					System.out.println("USB Permission Denied");
					device_denied();
				}
			}
			if ( (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) || (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) ) {
				devices_changed();
				System.out.println("USB devices changed");
			}
		}
	};

    public USBSerialWrapper() {
        this.id = counter;
        System.out.println("Created USBSerialWrapper object with id: " + this.id);
        counter++;
        m_filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
		m_filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
    }
    
    public void set_port_name(String d)
    {
		m_devicename = d;
		System.out.println("Java Devicename: " + m_devicename);
	}
    
    public void set_baud_rate(int br)
    {
		m_baudrate = br;
		System.out.println("Java Baudrate: " + br);
	}
	
	public void set_data_bits(int db)
    {
		m_databits = db;
		System.out.println("Java DataBits: " + db);
	}
	
	public void set_stop_bits(int sb)
    {
		m_stopbits = sb;
		System.out.println("Java StopBits: " + sb);
	}
	
	public void set_parity(int p)
    {
		m_parity = p;
		System.out.println("Java Parity: " + p);
	}
	
	public void set_flow_control(int fc)
    {
		m_flowcontrol = fc;
		System.out.println("Java FlowControl: " + fc);
	}
	
	public void set_rts(int rts)
    {
		m_rts = rts;
		System.out.println("Java RTS: " + rts);
	}
	
	public String[] discover_devices(Context c)
	{
		System.out.println("Java: discover_devices()");
		m_manager = (UsbManager) c.getSystemService(Context.USB_SERVICE);
		List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(m_manager);
		String[] devices = new String[availableDrivers.size()];
		
		if (availableDrivers.isEmpty()) {
			System.out.println("No drivers found");
		}
		else {
			System.out.println(availableDrivers.size() + " found");

			for(int i = 0; i < availableDrivers.size(); ++i){
				devices[i] = availableDrivers.get(i).getDevice().getProductName() + ":" + availableDrivers.get(i).getDevice().getDeviceName();
				System.out.println("USB device getDeviceName() == " + availableDrivers.get(i).getDevice().getDeviceName());
				System.out.println("USB device getProductName() == " + availableDrivers.get(i).getDevice().getProductName());
				System.out.println("USB device getProductId() == " + availableDrivers.get(i).getDevice().getProductId());
				System.out.println("USB device getVendorId() == " + availableDrivers.get(i).getDevice().getVendorId());
			}
		}
		c.registerReceiver(m_usbReceiver, m_filter, Context.RECEIVER_EXPORTED);
		return devices;
	}
	
    public String setup_serial(Context c)
    {
		m_context = c;
		int devicenum = 0;
		List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(m_manager);
		
		if (availableDrivers.isEmpty()) {
			System.out.println("No drivers found");
			return "returning...";
		}
		else {
			System.out.println(availableDrivers.size() + " found");
			for(int i = 0; i < availableDrivers.size(); ++i){
				System.out.println("USB device getDeviceName() == " + availableDrivers.get(i).getDevice().getDeviceName());
				System.out.println("USB device getProductName() == " + availableDrivers.get(i).getDevice().getProductName());
				System.out.println("USB device getProductId() == " + availableDrivers.get(i).getDevice().getProductId());
				System.out.println("USB device getVendorId() == " + availableDrivers.get(i).getDevice().getVendorId());
				
				if(m_devicename.equals(availableDrivers.get(i).getDevice().getDeviceName())){
					devicenum = i;
					System.out.println("Found device == " + devicenum);
					
				}
			}
		}

		m_driver = availableDrivers.get(devicenum);
		PendingIntent mPendingIntent = PendingIntent.getBroadcast(c, 0, new Intent(ACTION_USB_PERMISSION), PendingIntent.FLAG_MUTABLE | PendingIntent.FLAG_ALLOW_UNSAFE_IMPLICIT_INTENT);
		m_manager.requestPermission(m_driver.getDevice(), mPendingIntent);
		System.out.println("setup_serial() finished");
		return "Yipee!";
	}
	
	public int open_device()
	{
		UsbDeviceConnection connection = m_manager.openDevice(m_driver.getDevice());
		
		if (connection == null) {
			return 0;
		}

		m_port = m_driver.getPorts().get(0); // Most devices have just one port (port 0)
		
		try {
			m_port.open(connection);
			m_port.setParameters(m_baudrate, m_databits, m_stopbits, m_parity);
			m_port.setRTS(true);
			
		} catch (Exception e) {
			System.out.println("Exception: " + e.getMessage());
		}
		
		System.out.println("Created UsbManager");
		
		usbIoManager = new SerialInputOutputManagerTest(m_port, this);
		usbIoManager.start();
		return 1;
	}
	
	public void write(byte[] data)
	{
		try {
			m_port.write(data, 0);
		} catch (Exception e) {
			System.out.println("Exception: " + e.getMessage());
		}
	}
	
	public byte[] read()
	{
		byte[] buffer = new byte[1024];
		int r = 0;
		
		try {
			r = m_port.read(buffer, 0);
		} catch (Exception e) {
			System.out.println("Exception: " + e.getMessage());
		}
		
		byte[] returnbytes = Arrays.copyOf(buffer, r);
		return returnbytes;
	}
	
	public void close(int c)
	{
		if(usbIoManager != null) {
            usbIoManager.setListener(null);
            usbIoManager.stop();
        }
        usbIoManager = null;
        try {
            m_port.close();
        } catch (IOException ignored) {}
        m_port = null;
	}
	
	@Override
	public void onNewData(byte[] data) {
		data_received(data);
	}
	
	@Override
	public void onRunError(Exception e) {
		System.out.println("Exception: " + e.getMessage());
	}
	
	private static native void data_received(byte[] data);
	private static native void device_open();
	private static native void device_denied();
	private static native void devices_changed();
}
