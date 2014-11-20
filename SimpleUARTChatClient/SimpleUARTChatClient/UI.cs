using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;


namespace SimpleUARTChatClient
{
    public partial class UI : Form
    {
        static string[] comPorts;
        int baudRate = 600;
        public UI()
        {
            InitializeComponent();
            updateWidgets(false);
            refreshComPorts();
            Sender.messageAvailable += messageAvailable;
        }


        public void appendMessage(String msg,bool incoming)
        {

            this.BeginInvoke(new EventHandler(delegate
            {
                this.richTextBox1.SelectionColor = incoming? Color.Blue:Color.OrangeRed;
                this.richTextBox1.SelectionAlignment = incoming? HorizontalAlignment.Right: HorizontalAlignment.Left;
                this.richTextBox1.AppendText(!incoming ? "> " : ""); 
                this.richTextBox1.AppendText(msg);
                this.richTextBox1.AppendText(incoming ? "< " :""); 
                this.richTextBox1.AppendText("\n");               
            }));

        }

        public void messageAvailable(String msg)
        {            
            appendMessage(msg, true);
        }

        private void refreshComPorts()
        {
            comPorts = SerialPort.GetPortNames();
            if (comPorts.Length != 0)
            {
                this.cmbBoxSerialPort.Items.Clear();
                this.cmbBoxSerialPort.Items.AddRange(comPorts);
                this.cmbBoxSerialPort.SelectedIndex = 0;
            }
            else
            {
                if (this.cmbBoxSerialPort.Items.Count == 0)
                {
                    this.cmbBoxSerialPort.Items.Add("<N/A>");
                }
            }

        }

        private void sendMsg()
        {
            String msg = textBox1.Text;
            if (msg == null || msg == String.Empty)
            {
                return;
            }
            Sender.Send(Encoding.ASCII.GetBytes(msg + "\n"));
            appendMessage(msg, false);
            textBox1.Text = "";
        }

        private void button1_Click(object sender, EventArgs e)
        {
            sendMsg();
        }

        private void cmbBoxSerialPort_DropDown(object sender, EventArgs e)
        {
            refreshComPorts();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (!Sender.Instance.serialPortIsOPen() && cmbBoxSerialPort.SelectedItem != null)
            {
                
               String PortName = this.cmbBoxSerialPort.SelectedItem.ToString();
               try
                    {
                        Sender.Instance.openSerialPort(PortName, baudRate);
                        updateWidgets(true);
                    }
                    catch (Exception e1)
                    {
                        MessageBox.Show("Unable to open port: " + PortName + "\n" + e1.Message);
                        updateWidgets(false);
                    }

                }
                else
                {
                    Sender.Instance.closeSerialPort();
                    updateWidgets(false);
                }
            
        }

        private void updateWidgets(bool isConnected)
        {
            this.button2.Text = isConnected ? "Disconnet" : "Connect";
            this.cmbBoxSerialPort.Enabled = !isConnected;
            this.textBox1.Enabled = isConnected;
            this.richTextBox1.Enabled = isConnected;

        }

        private void textBox1_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)13)
            {
                sendMsg();
            }
        } 


    }
}
