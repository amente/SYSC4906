using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO.Ports;
using System.Diagnostics;
namespace SimpleUARTChatClient
{
    public delegate void messageAvailableHandler(String msg);

    class Sender
    {
        private static volatile Sender sender;
        private static object syncRoot = new Object();
        private static SerialPort serialPort;

        public static event messageAvailableHandler messageAvailable;
        private static StringBuilder msgBuffer;

        private static Stopwatch watchdog;
        //private static System.Threading.Timer MyTimer = new System.Threading.Timer(dataWatchDog, null, 100, 2000);

        private Sender() { }

        public static Sender Instance
        {
            get
            {
                // use a lock for additional thread safety
                lock (syncRoot)
                {
                    if (sender == null)
                    {
                        sender = new Sender();
                        serialPort = new SerialPort();
                        msgBuffer = new StringBuilder();
                        serialPort.DataReceived += new SerialDataReceivedEventHandler(DataReceivedHandler);
                    }
                }
                return sender;
            }
        }


        private static void DataReceivedHandler(object sender, SerialDataReceivedEventArgs e)
        {
            if (!serialPort.IsOpen)
            {
                return;
            }
            /*lock (syncRoot)
            {
                watchdog.Reset();
                watchdog.Start();
            }*/
            byte[] smallBuffer = new byte[serialPort.BytesToRead];
            serialPort.Read(smallBuffer, 0, smallBuffer.Length);
            msgBuffer.Append(Encoding.UTF8.GetString(smallBuffer, 0, smallBuffer.Length));            
            String s = msgBuffer.ToString();
            //Console.WriteLine(s);
            if (s.Contains('\n'))
            {
                Console.Write(s);
                OnMessageAvailable(s.Substring(0,s.IndexOf('\n')));
                msgBuffer.Clear();
                msgBuffer.Append(s.Substring(s.IndexOf('\n') + 1));
            }
            
            //Console.WriteLine(BitConverter.ToString(smallBuffer).Replace("-", string.Empty));
        }

        public void closeSerialPort()
        {
            if (serialPort != null && serialPort.IsOpen)
            {
                serialPort.Close();
            }
        }

        public void openSerialPort(String name, int baud)
        {
            if (!serialPort.IsOpen)
            {
                serialPort.PortName = name;
                serialPort.BaudRate = baud;
                serialPort.Open();
                Console.WriteLine(name + " Open Success!");
            }
            else
            {
                Console.WriteLine(name + " Open Failed!");
            }

        }

        private static void OnMessageAvailable(String msg)
        {
            if (messageAvailable != null)
            {
                messageAvailable(msg);
            }
        }

        public static void Send(byte[] cmd)
        {           
            try
            {
                serialPort.Write(cmd, 0, cmd.Length);
            }
            catch (InvalidOperationException e)
            {
                // Message Not sent comport is closed
            }           
        }

        public bool serialPortIsOPen()
        {
            return serialPort.IsOpen;
        }

        public static void dataWatchDog(object state)
        {
            lock (syncRoot)
            {
                if (watchdog.ElapsedMilliseconds > 10000)
                {
                    //No data recieved for more than 10 seconds
                }

            }
        }

    }
}