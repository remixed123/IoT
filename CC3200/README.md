**SendToEventHub Folder**
* Contains the CC3200 LaunchPad C source code, contained within a Code Composure Studio v6 Project
* Contains a binary that can be flashed to the CC3200 LaunchPad. Which works with the original site. You can see telemetry being added here - http://ssmlwf.azurewebsites.net/sqldatabase/processedtelemetry

**sastokengenerator Folder**
* Contains Windows Forms Application Project written in C# that will generate a SAS Token 
* Contains application installation which can be installed and used to generate a SAS Token

**azurecacert.cer** 
* The certificate authority used to communicate over SSL/TLS with Azure services. It is called the Baltimore CyberTrust Root
