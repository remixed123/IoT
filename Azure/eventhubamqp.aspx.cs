using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.IO;
using System.Net.Http;
using System.Net.Http.Formatting;
using System.Net.Http.Headers;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Microsoft.ServiceBus;
using Microsoft.ServiceBus.Messaging;
using System.Runtime.Serialization;

namespace ssmlwf
{
    public partial class eventhubamqp : System.Web.UI.Page
    {
        

        protected void Page_Load(object sender, EventArgs e)
        {
           
        }

        protected void Button1_Click(object sender, EventArgs e)
        {
            int sendCount = Convert.ToInt32(SendCount.SelectedValue);

            EventHubConnectSend(sendCount).Wait();
        }

        public async Task EventHubConnectSend(int sendCount)
        {
            var cs = @"Endpoint=sb://yournamespace.servicebus.windows.net/;SharedAccessKeyName=YourPublisher;SharedAccessKey=12EHa5NNllnZBz6gksogvNfhketIokdTdjx4ZrTfu9U=";

            var builder = new ServiceBusConnectionStringBuilder(cs)
                        {
                            TransportType = TransportType.Amqp
                        };

            var client = EventHubClient.CreateFromConnectionString(builder.ToString(), "swiftsoftware-eh");

            int sentCount = sendCount;
            string infoText;

            for (int i = 0; i < sendCount; i++)
            {
                try
                {
                    if (MessageType.Text == "Sensor")
                    {
                        infoText = "Operating Within Expected Parameters";
                    }
                    else
                    {
                        infoText = "Temperature Sensor Requires Calibration";
                    }

                    Random rnd = new Random();
                    int intTemp = rnd.Next(15, 25);
                    int intHum = rnd.Next(50, 80);
                 
                    var e = new Event
                    {
                        MessageType = MessageType.Text,
                        Temp = intTemp,
                        Humidity = intHum,
                        Location = Location.Text,
                        Room = Room.Text,
                        Info = infoText
                    };

                    var serializedString = JsonConvert.SerializeObject(e);
                    var data = new EventData(Encoding.UTF8.GetBytes(serializedString))
                    {
                        PartitionKey = rnd.Next(6).ToString()
                    };

                    // Set user properties if needed
                    data.Properties.Add("Type", "Telemetry_" + DateTime.Now.ToLongTimeString());
                    
                    await client.SendAsync(data).ConfigureAwait(false);
                }
                catch (Exception exp)
                {
                    Console.WriteLine("Error on send: " + exp.Message);
                    sentCount--;
                }

                await Task.Delay(Convert.ToInt32(DelayTime.Text));
               
            }

            MessageResult.Text = " " + sentCount.ToString() + " " + MessageType.Text + " Messages were successfully sent to Event Hub";
            return;
        }
   
    }

    [DataContract]
    public class Event
    {
        [DataMember]
        public string MessageType { get; set; }
        [DataMember]
        public int Temp{ get; set; }
        [DataMember]
        public int Humidity { get; set; }
        [DataMember]
        public string Location { get; set; }
        [DataMember]
        public string Room { get; set; }
        [DataMember]
        public string Info { get; set; }
    }
}

