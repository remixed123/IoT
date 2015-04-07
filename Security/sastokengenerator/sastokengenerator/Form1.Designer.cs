namespace sastokengenerator
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.sbconnstringtext = new System.Windows.Forms.TextBox();
            this.eventhubnametext = new System.Windows.Forms.TextBox();
            this.button1 = new System.Windows.Forms.Button();
            this.sastokentext = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 30);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(151, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Service Bus Connection String";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 141);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(89, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Event Hub Name";
            // 
            // sbconnstringtext
            // 
            this.sbconnstringtext.Location = new System.Drawing.Point(169, 30);
            this.sbconnstringtext.Name = "sbconnstringtext";
            this.sbconnstringtext.Size = new System.Drawing.Size(604, 20);
            this.sbconnstringtext.TabIndex = 2;
            // 
            // eventhubnametext
            // 
            this.eventhubnametext.Location = new System.Drawing.Point(169, 138);
            this.eventhubnametext.Name = "eventhubnametext";
            this.eventhubnametext.Size = new System.Drawing.Size(227, 20);
            this.eventhubnametext.TabIndex = 3;
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(624, 207);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(137, 23);
            this.button1.TabIndex = 4;
            this.button1.Text = "Generate SAS Token";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // sastokentext
            // 
            this.sastokentext.Location = new System.Drawing.Point(162, 297);
            this.sastokentext.Multiline = true;
            this.sastokentext.Name = "sastokentext";
            this.sastokentext.Size = new System.Drawing.Size(599, 85);
            this.sastokentext.TabIndex = 5;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Microsoft Tai Le", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(166, 53);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(543, 70);
            this.label3.TabIndex = 6;
            this.label3.Text = resources.GetString("label3.Text");
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(169, 165);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(227, 39);
            this.label4.TabIndex = 7;
            this.label4.Text = "This is the name of the Event Hub you created\r\nThe string name will look similar " +
    "to this:-\r\ncompanyname-eh";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(12, 300);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(115, 13);
            this.label5.TabIndex = 8;
            this.label5.Text = "Generated SAS Token";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(159, 385);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(612, 52);
            this.label6.TabIndex = 9;
            this.label6.Text = resources.GetString("label6.Text");
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(646, 233);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(96, 26);
            this.label7.TabIndex = 10;
            this.label7.Text = "Press To Generate\r\n     SAS Token\r\n";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(782, 458);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.sastokentext);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.eventhubnametext);
            this.Controls.Add(this.sbconnstringtext);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Name = "Form1";
            this.Text = "SAS Token Generator";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox sbconnstringtext;
        private System.Windows.Forms.TextBox eventhubnametext;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.TextBox sastokentext;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label7;
    }
}

