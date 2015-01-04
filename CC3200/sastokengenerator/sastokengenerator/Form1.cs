///////////////////////////////////////////////////////////////////////////////
//
// SAS Token Generator
//
// Developer: Glenn Vassallo
// Email: contact@swiftsoftware.com.au
//
// Contains c# code which can be used to create a shared access token for a specific scope, which is required to POST to the EVENT HUB REST API. 
// SAS tokens are described here http://msdn.microsoft.com/en-us/library/windowsazure/dn170477.aspx.
// 
///////////////////////////////////////////////////////////////////////////////

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Web;
using System.Globalization;
using System.Security.Cryptography;
using Microsoft.ServiceBus;


namespace sastokengenerator
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {

            string EventHubName = eventhubnametext.Text;
            string serviceBusConnectionString = sbconnstringtext.Text;

            ServiceBusConnectionStringBuilder connectionString = new ServiceBusConnectionStringBuilder(serviceBusConnectionString);

            string ServiceBusNamespace = connectionString.Endpoints.First().Host;
            string namespaceKeyName = connectionString.SharedAccessKeyName;
            string namespaceKey = connectionString.SharedAccessKey;
            string baseAddressHttp = "http://" + ServiceBusNamespace + "/";
            string eventHubAddress = baseAddressHttp + EventHubName;

            var sas = CreateSasToken(ServiceBusNamespace, namespaceKeyName, namespaceKey);

            sastokentext.Text = sas.ToString();

        }
        
        static string CreateSasToken(string uri, string keyName, string key)
        {
            // Set token lifetime to 20 minutes. When supplying a device with a token, you might want to use a longer expiration time (see below)
            DateTime origin = new DateTime(1970, 1, 1, 0, 0, 0, 0);
            TimeSpan diff = DateTime.Now.ToUniversalTime() - origin;
            //uint tokenExpirationTime = Convert.ToUInt32(diff.TotalSeconds) + 20 * 60;
            uint tokenExpirationTime = Convert.ToUInt32(diff.TotalSeconds) + 60 * 60 * 24 * 365 * 10; // 10 year expiry, use when creating SAS Token for device

            string stringToSign = HttpUtility.UrlEncode(uri) + "\n" + tokenExpirationTime;
            HMACSHA256 hmac = new HMACSHA256(Encoding.UTF8.GetBytes(key));

            string signature = Convert.ToBase64String(hmac.ComputeHash(Encoding.UTF8.GetBytes(stringToSign)));
            string token = String.Format(CultureInfo.InvariantCulture, "SharedAccessSignature sr={0}&sig={1}&se={2}&skn={3}",
                HttpUtility.UrlEncode(uri), HttpUtility.UrlEncode(signature), tokenExpirationTime, keyName);
            return token;
        }
    }
}
