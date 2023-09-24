// Note most of the important pieces of code were acquired from RandomNerdTutorials and other online sources.
// However, the algorithms were implemented by me, Abu Bakr Siddique. Please find my following details below:

// Author: Abu Bakr Siddique, Email: ecsesiddique.297@gmail.com
// My latest qualification: Bachelors(Honors) in ECSE(Electrical and Computer Systems Engineering) from Monash University Malaysia in 2022 December

// Last Modified Date: 24th September, 2023 - Sunday
// Performed Project at SRP - industrial level
// Sensor used was an NPN diffuse-reflective photosensor from Omron

// Note there are a total of 5 Edits that you need to make for this code to be fully functional.

#include <WiFi.h>
#include "time.h"
#include <Arduino.h>
#include <ESP_Mail_Client.h>

#define LED 2

// ----------------------------------- Edit: 1 Please Edit the SMTP variables, Email_ID and App_Passwords here ------------------------

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "" // enter the SMTP of of the host (ESP32) email ID
#define SMTP_PORT  // enter a number or integer here e.g. 123
/* The sign in credentials */
#define AUTHOR_EMAIL ""     //  enter the Email id, e.g.: sendmemail@gmail.com or givememail@outlook.com
#define AUTHOR_PASSWORD ""  //  enter the APP password for your email. This needs to be turned on and generated - look up on the randomnerdtutorials for this or the internet on how to acquire this

// ----------------------------------- Edit: 1 Please Edit the email and App passwords above -----------------------

#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

// ---------------------------- Edit: 2 Please input the email of all the people who you wish to email --------------

// ---------- Note: To add or remove receiving the email you need to add or remove the following line inside the sendMsg function: message.addRecipient(RECIPIENT_NAMES[0], RECIPIENT_EMAILS[0]);
/* Mailing List*/
const String RECIPIENT_EMAILS[]= {""};  //  Enter the email id of your recipients, e.g. "goodguy@outlook.com"
const String RECIPIENT_NAMES[] = {""};  //  Enter the name of the Participants, e.g. "John Wick"

// ---------------------------- Edit: 2 Enter the Recipient Email(s) above -----------------------------------

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

/* Declare the global Session_Config for user defined session credentials */
Session_Config config;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

/* Callback function to get the date in the correct format */
String dateFormat (int yearVal, int monthVal, int dayVal);

/* Callback function to get the time in the correct format */
String timeFormat (int hrVal, int minVal, int secVal);

/* Callback function to send the email */
void sendMsg(String dateVal, String timeStart, String timeStop, int units, int totalUnits);
size_t msgNo;

const int sensorPin = 25;
// Electrical Generator Room WiFi SSID: "UB-Wifi" "POCO", "SRF-GUEST", "BFM-Wifi"
// Electrical Generator Room WiFi password: "Ar@#Sbd7896" "Saad123456", "RS6S7LQH" (expires on Sunday - 24th September, 2023), "Singer@#bfm"

// -------------------- Edit: 3 Enter the WiFi credentials below --------------

// Note: Only use 2.4GHz networks/bands. Do not use 5GHz band(s)/Network(s)
const char* ssid = "";      //  WiFi network/band name, e.g.: "ThisIsMyWiFi", "Virus", "DoNotTouch"
const char* password = "";  //  WiFi password, e.g.: "Lonasb@459!njas"

// -------------------- Edit: 3 Enter the WiFi credentials above --------------

const char* ntpServer = "pool.ntp.org";

// -------------------- Edit: 4 Enter the offset from the UTC Time + Add any daylight saving if your country observes below------------

const long  gmtOffset_sec = 3600*6; //  Since I am in Bangladesh, which is 6 hours ahead of UTC time, I have used (6*3600) - 1 hr has 3600 seconds.
const int   daylightOffset_sec = 0; //  Since we don't observe daylightOffset in Bangladesh, it is 0. If your country observes any e.g. Belgium should be 3600 (1 hr)

// -------------------- Edit: 4 Enter the offset from the UTC Time + Add any daylight saving if your country observes above------------

// Date Variables
int todayDateCheck = -1;
int nowYear;
int nowMonth;
int nowDay;
String dateString;

// Time Variables
int lastHrVar = -1;
int nowHr;
int nowMin;
int nowSec;
String todayTimeStart;
String bodyRecTimeStart;
String bodyRecTimeStop;

const int interval = 3500; // 3.5 seconds

struct tm timeinfo;

// Body Count variables
int hourlyBodyCtr, totalBodyCtr = 0, prevTotalBodyCt  = 0;
// Debugging Purposes
int tempHourlyBodyCtr = 0;

// The Message:
String htmlMsg =
"<div>"
  "<style>"
    "table {"
      "font-family: arial, sans-serif;"
      "border-collapse: collapse;"
      "width: 100%;"
    "}"

    "td, th {"
      "border: 1px solid #dddddd;"
      "text-align: left;"
      "padding: 8px;"
    "}"

    "tr:nth-child(even) {"
      "background-color: #dddddd;"
    "}"
  "</style>"
  "<p> [TODAY DATE]: Total bodies foamed upto now = 0 units</p>"
  "<table style=\"width:100%\">"
    "<tr>"
      "<th>Date</th>"
      "<th>Start from</th>"
      "<th>to</th>"
      "<th>units</th>"
    "</tr>"
  "</table>"
"</div>";

void setup()
{
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);
  pinMode(LED, OUTPUT);
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED,LOW);
    delay(500);
    Serial.println("Connecting to WiFi..");    
  }
  Serial.printf("Connected to %s\n", ssid);
  digitalWrite(LED, HIGH);
  // Print local IP address and start web server
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); //show ip address when connected on serial monitor.
  
   /*  Set the network reconnection option */
  MailClient.networkReconnect(true);

  /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
   */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Set the session config */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 6;
  config.time.day_light_offset = 0;

  // init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time. Please resolve it. Suggestion: Check the internet availability");
    return;
  }
  nowYear = timeinfo.tm_year;
  nowMonth = timeinfo.tm_mon+1;
  nowDay = timeinfo.tm_mday;
  todayDateCheck = nowDay;

  nowHr = timeinfo.tm_hour;
  nowMin = timeinfo.tm_min;
  nowSec = timeinfo.tm_sec;
  lastHrVar = nowHr;
  // lastHrVar = nowMin;
  todayTimeStart = timeFormat (nowHr, nowMin, nowSec);
  bodyRecTimeStop = todayTimeStart;
  dateString = dateFormat (nowYear, nowMonth, nowDay);
}
 
void loop()
{
  // gets the local time - necessary to iterate through in every instance of the loop
  if(!getLocalTime(&timeinfo))  
  {
    Serial.println("Failed to obtain time. Please resolve it. Suggestion: Check the internet availability");
    return;
  }else{
    bodyRecTimeStart = bodyRecTimeStop;
    ; // NULL Statement
    // Sensor detection code/function with body counting goes here
    if (digitalRead(sensorPin)==HIGH){
      // Serial.println("Entered the Sensor == HIGH condition");
      // Serial.printf("Sensor value inside the checking function is %d\n",digitalRead(sensorPin));
      unsigned long prevMillis = millis();
      while(digitalRead(sensorPin)==HIGH)
      {
        // Serial.printf("Sensor value is: %d\n",digitalRead(sensorPin));
        // digitalWrite(LED,HIGH);
        // Serial.println("Entered the Sensor == HIGH loop");
      }
      unsigned long currentMillis = millis();
      // Serial.printf("Current Millis is %d\n", currentMillis);
      if ((currentMillis - prevMillis)!=0){
        Serial.printf("Difference between Millis is %d\n", (currentMillis - prevMillis));
      }
      if ((digitalRead(sensorPin)==LOW)){
        // Serial.println("Entered the Sensor == LOW condition");
        if ((currentMillis - prevMillis >= interval)){
          Serial.println("Body was detected");
          // Serial.printf("Sensor value is: %d\n",digitalRead(sensorPin));
          ++hourlyBodyCtr;
          ++totalBodyCtr;
          // digitalWrite(LED,LOW);
        }else{
          Serial.printf("Sensor was ON for %d ms, less than %d ms. Something other than body passed by sensor\n",(currentMillis-prevMillis),interval);
          // digitalWrite(LED,LOW);
        }
      }
    }
      // Get Current Time
      nowHr = timeinfo.tm_hour;
      nowMin = timeinfo.tm_min;
      nowSec = timeinfo.tm_sec;
      
      // updating Current Date if we are recording longer than a day
      // Capture Current Date
      nowYear = timeinfo.tm_year;
      nowMonth = timeinfo.tm_mon+1;
      nowDay = timeinfo.tm_mday;
      
      if (todayDateCheck != nowDay){
        // Refresh the Date
        dateString = dateFormat (nowYear, nowMonth, nowDay);
        // Refresh the total Body Counter
        totalBodyCtr = 0;
        prevTotalBodyCt = 0;
        tempHourlyBodyCtr = 0;
        //Refresh Message Number
        msgNo = 0;
        // Refresh the HTML message list
        htmlMsg =
        "<div>"
          "<style>"
            "table {"
              "font-family: arial, sans-serif;"
              "border-collapse: collapse;"
              "width: 100%;"
            "}"

            "td, th {"
              "border: 1px solid #dddddd;"
              "text-align: left;"
              "padding: 8px;"
            "}"

            "tr:nth-child(even) {"
              "background-color: #dddddd;"
            "}"
          "</style>"
          "<table style=\"width:100%\">"
            "<tr>"
              "<th>Date</th>"
              "<th>Start from</th>"
              "<th>to</th>"
              "<th>units</th>"
            "</tr>"
          "</table>"
        "</div>";
      }

      // Email invoking frequency is set to 1 per hour

      // Serial Print only if a body is detected
      if (tempHourlyBodyCtr != hourlyBodyCtr){
        Serial.printf("Hourly Body Count outside email: %d\n",hourlyBodyCtr);
        Serial.printf("Total Body Count outside email: %d\n",totalBodyCtr);
      }
      // Send Email if 1 hour has passed or if it is 4.30 pm
      if ((lastHrVar!= nowHr)||((nowHr==16)&&((nowMin>=29)&&(nowMin<=59)))){
        Serial.printf("------------------------------ SENDING EMAIL NOW -----------------------------\n");
        lastHrVar = nowHr;
        // lastHrVar = nowMin;
        bodyRecTimeStop = timeFormat(nowHr, nowMin, nowSec);
        // ------------------------ Invoking Email Reporting Function -------
        sendMsg(dateString, bodyRecTimeStart, bodyRecTimeStop, hourlyBodyCtr, totalBodyCtr, prevTotalBodyCt);
        // ------------------------ End of Email Reporting Function ---------
        prevTotalBodyCt = totalBodyCtr;
        Serial.println("An hour has passed");
        // Printing time, body counts/hr and total body counts
        Serial.printf("%02d:%02d:%02d\n",nowHr,nowMin,nowSec);
        Serial.printf("Hourly Body Count is %d\n", hourlyBodyCtr);
        Serial.printf("Total.... Body Count is %d\n", totalBodyCtr);
        // reset the hourlyBodyCtr variable to 0
        hourlyBodyCtr = 0;
      }
      tempHourlyBodyCtr = hourlyBodyCtr;
    }
}

// ------------------------------------ Callback functions -----------------------

void sendMsg(String dateVal, String timeStart, String timeStop, int units, int totalUnits, int pastHrUnits){
  /* Declare the message class */
  SMTP_Message message;

  //  --------------------------- Edit 5: Edit the Subject and Sender name, add or Remove the email recipients below ---------------------
  
  /* Set the message headers */
  message.sender.name = F("");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("");
  message.addRecipient(RECIPIENT_NAMES[0], RECIPIENT_EMAILS[0]);
  
  // This is how we add more recipients. We can also erase of comment the lines to remove the people from the mailing list
  // message.addRecipient(RECIPIENT_NAMES[1], RECIPIENT_EMAILS[1]);
  // message.addRecipient(RECIPIENT_NAMES[2], RECIPIENT_EMAILS[2]);
  // message.addRecipient(RECIPIENT_NAMES[3], RECIPIENT_EMAILS[3]);

  //  --------------------------- Edit 5: Edit the Subject and Sender name, add or Remove the email recipients above ---------------------

  // ------------------------------------- Modified Code -------------------------------------
  /*Modify HTML message*/
  htmlMsg.replace("</table>", String(""));
  htmlMsg.replace("</div>", String(""));
  htmlMsg +=
      "<tr>"
        "<td>[TABLE DATE]</td>"
        "<td>[START TIME]</td>"
        "<td>[END TIME]</td>"
        "<td>[UNITS]</td>"
      "</tr>"
    "</table>"
  "</div>";
  // ------------------------------------- Modified Code -------------------------------------
  // ------------------------------------- Added Code -------------------------------------
  // Replacing the contents inside the [] with actual values
  htmlMsg.replace("[TABLE DATE]", dateVal);
  htmlMsg.replace("[START TIME]", timeStart);
  htmlMsg.replace("[END TIME]", timeStop);
  htmlMsg.replace("[UNITS]", String(units));
  htmlMsg.replace("[TODAY DATE]", dateVal);
  htmlMsg.replace((String(pastHrUnits)+" units"), (String(totalUnits) + " units"));
  // ------------------------------------- Added Code -------------------------------------
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;


  /* Connect to the server */
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (msgNo = 0; msgNo < smtp.sendingResult.size(); msgNo++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(msgNo);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
      
      ESP_MAIL_PRINTF("Message No: %d\n", msgNo + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
    // --------------------------- No Changes to the above function smtpCallback ----------------------
  }
}

String dateFormat (int yearVal, int monthVal, int dayVal){
  String dateVal;
  if((dayVal<10)||(monthVal<10)){
    if(dayVal<10){
      dateVal = String("0") + String(dayVal);
    }else{
      dateVal = String(dayVal);
    }
    
    if(monthVal<10){
      dateVal += "." + String("0") + String(monthVal) + "." + String(yearVal);
    } else {
      dateVal += "." + String(monthVal) + "." + String(yearVal);
    }
  }else{
    dateVal = String(dayVal) + "." + String(monthVal) + "." + String(yearVal);
  }
  return dateVal;
}

String timeFormat (int hrVal, int minVal, int secVal){
    String timeVal;
    if(hrVal<10||minVal<10||secVal<10){
    if(hrVal<10){
      timeVal = String("0") + String(hrVal);
    }else{
      timeVal = String(hrVal) ;
    }

    if(minVal<10){
      timeVal +=  ":" + String("0") + String(minVal);
    }else{
      timeVal += ":" + String(minVal);
    }

    if(secVal<10){
      timeVal += ":" + String("0") + String(secVal);  
    }else{
      timeVal += ":" + String(secVal);
    }
  }else{
    timeVal = String(hrVal) + ":" + String(minVal) + ":" +String(secVal);
  }
  return timeVal;
}