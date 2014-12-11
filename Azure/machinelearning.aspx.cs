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

namespace ssmlwf.machinelearning
{
    public partial class test : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {
           
        }

        protected void Button1_Click(object sender, EventArgs e)
        {
            InvokeRequestResponseService().Wait();
        }

        public async Task InvokeRequestResponseService()
        {
            if (Age.Text == "")
                Age.Text = "0";
            if (EducationYears.Text == "")
                EducationYears.Text = "0";
            if (CapitalGains.Text == "")
                CapitalGains.Text = "0";
            if (CapitalLoss.Text == "")
                CapitalLoss.Text = "0";
            if (WorkHours.Text == "")
                WorkHours.Text = "0";

            using (var client = new HttpClient())
            {
                ScoreData scoreData = new ScoreData()
                {
                    FeatureVector = new Dictionary<string, string>() 
                    {
                        { "age", Age.Text },
                        { "education", Education.Text },
                        { "education-num", EducationYears.Text },
                        { "marital-status", MaritalStatus.Text },
                        { "relationship", Relationship.Text },
                        { "race", Race.Text },
                        { "sex", Sex.Text  },
                        { "capital-gain", CapitalGains.Text },
                        { "capital-loss", CapitalLoss.Text },
                        { "hours-per-week", WorkHours.Text },
                        { "native-country", NativeCountry.Text },
                    },
                    GlobalParameters =
                        new Dictionary<string, string>()
                        {
                        }
                };

                ScoreRequest scoreRequest = new ScoreRequest()
                {
                    Id = "score00001",
                    Instance = scoreData
                };

                const string apiKey = "dg/pwCd7zMPc57hgdusJqxP8nbtKGV7//7GI0STeNxSLaChdhjhj3O8WhfpEjQgwigvdVl/7VWjqe/ixOA=="; // Replace this with the API key for the web service
                client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", apiKey);

                client.BaseAddress = new Uri("https://ussouthcentral.services.azureml.net/workspaces/a932e11a04434bffa29ahshshkke9ed4/services/e8796c4382fb42ajsityud2b3ddac357/score");

                HttpResponseMessage response = await client.PostAsJsonAsync("", scoreRequest).ConfigureAwait(false);

                if (response.IsSuccessStatusCode)
                {
                    string result = await response.Content.ReadAsStringAsync();
                    Console.WriteLine("Result: {0}", result);

                    string jsonResults = "{results:" + result + "}";
                    object objectResults = JsonConvert.DeserializeObject<JsonResults>(jsonResults);

                    IndividualResult.Text = ((JsonResults)(objectResults)).results[11].ToString(); 
                    Results.Text = result;
                }
                else
                {
                    Console.WriteLine("Failed with status code: {0}", response.StatusCode);
                }
            }
        }

    }

    public class ScoreData
    {
        public Dictionary<string, string> FeatureVector { get; set; }
        public Dictionary<string, string> GlobalParameters { get; set; }
    }

    public class ScoreRequest
    {
        public string Id { get; set; }
        public ScoreData Instance { get; set; }
    }

    public class JsonResults
    {
        public string[] results { get; set; }
    }
}
