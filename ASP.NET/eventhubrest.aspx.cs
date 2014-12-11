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
using System.Security.Cryptography;
using System.Globalization;
using System.Configuration;
using Microsoft.ServiceBus;
using System.Threading;
using Microsoft.ServiceBus.Messaging;

namespace ssmlwf
{
    public partial class eventhubrest : System.Web.UI.Page
    {
        public const string EventHubName = "YourEventHubName";
        protected void Page_Load(object sender, EventArgs e)
        {

        }

        protected void Button1_Click(object sender, EventArgs e)
        {
            int sendCount = Convert.ToInt32(SendCount.SelectedValue);

            EventHubConnectSend(sendCount);
        }

        public void EventHubConnectSend(int sendCount)
        {
            string serviceBusConnectionString = ConfigurationManager.AppSettings["Microsoft.ServiceBus.ConnectionString"];

            if (string.IsNullOrWhiteSpace(serviceBusConnectionString))
            {
                Console.WriteLine("Please provide ServiceBus Connection string in App.Config.");
            }

            ServiceBusConnectionStringBuilder connectionString = new ServiceBusConnectionStringBuilder(serviceBusConnectionString);

            string ServiceBusNamespace = connectionString.Endpoints.First().Host;
            string namespaceKeyName = connectionString.SharedAccessKeyName;
            string namespaceKey = connectionString.SharedAccessKey;
            string baseAddressHttp = "http://" + ServiceBusNamespace + "/";
            string eventHubAddress = baseAddressHttp + EventHubName;

            var sas = CreateSasToken(ServiceBusNamespace, namespaceKeyName, namespaceKey);

            // Create client.
            var httpClient = new HttpClient();
            httpClient.BaseAddress = new Uri(String.Format("https://{0}/", ServiceBusNamespace));
            httpClient.DefaultRequestHeaders.TryAddWithoutValidation("Authorization", sas);

            int sentCount = sendCount;
            string infoText;

            // Keep sending.
            for (int i = 0; i < sendCount; i++)
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

                Event e = new Event
                {
                    MessageType = MessageType.Text,
                    Temp = intTemp,
                    Humidity = intHum,
                    Location = Location.Text,
                    Room = Room.Text,
                    Info = infoText
                };


                //var serializedString = JsonConvert.SerializeObject(e);
                //var data = new EventData(Encoding.UTF8.GetBytes(serializedString))
                //{
                //    PartitionKey = rnd.Next(6).ToString()
                //};

                //// Set user properties if needed
                //data.Properties.Add("Type", "Telemetry_" + DateTime.Now.ToLongTimeString());

                var postResult = httpClient.PostAsJsonAsync(String.Format("{0}/messages", EventHubName), e).Result;

                if (postResult.StatusCode.ToString() != "Created")
                {
                    sentCount--;
                }

                Thread.Sleep(new Random().Next(1000, 5000));
            }

            MessageResult.Text = " " + sentCount.ToString() + " " + MessageType.Text + " Messages were successfully sent to Event Hub";
            return;
        }

        // Create a SAS token for a specified scope. SAS tokens are described in http://msdn.microsoft.com/en-us/library/windowsazure/dn170477.aspx.
        static string CreateSasToken(string uri, string keyName, string key)
        {
            // Set token lifetime to 20 minutes. When supplying a device with a token, you might want to use a longer expiration time.
            DateTime origin = new DateTime(1970, 1, 1, 0, 0, 0, 0);
            TimeSpan diff = DateTime.Now.ToUniversalTime() - origin;
            uint tokenExpirationTime = Convert.ToUInt32(diff.TotalSeconds) + 20 * 60;

            string stringToSign = HttpUtility.UrlEncode(uri) + "\n" + tokenExpirationTime;
            HMACSHA256 hmac = new HMACSHA256(Encoding.UTF8.GetBytes(key));

            string signature = Convert.ToBase64String(hmac.ComputeHash(Encoding.UTF8.GetBytes(stringToSign)));
            string token = String.Format(CultureInfo.InvariantCulture, "SharedAccessSignature sr={0}&sig={1}&se={2}&skn={3}",
                HttpUtility.UrlEncode(uri), HttpUtility.UrlEncode(signature), tokenExpirationTime, keyName);
            return token;
        }

    }

}
