#include <ArduinoJson.h>

#include <Firebase.h>
#include <FirebaseArduino.h>
#include <FirebaseCloudMessaging.h>
#include <FirebaseError.h>
#include <FirebaseHttpClient.h>
#include <FirebaseObject.h>


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <WiFiClientSecure.h>
#include <ESP_Mail_Client.h>


#define WIFI_SSID "Advait"
#define WIFI_PASSWORD "78677867"
#define SMTP_HOST "smtp.gmail.com"

#define SMTP_PORT 465

#define AUTHOR_EMAIL "smartdrainagesystem12@gmail.com"
#define AUTHOR_PASSWORD "smartdrainagesystem"

SMTPSession smtp;

ESP_Mail_Session session;

SMTP_Message message;




#define FIREBASE_HOST "drag-6d70d-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "p3IdPprDnbBvcwlwj6sjfTmDWlKfy4UhcrZNzLrj"

bool trigger_Send = true;
float p_Threshold_above = 85.0;
float p_Threshold_below = 70.0;
const int trigPin = 12;
const int echoPin = 14;

int led1 = D0;
int led2 = D1;
int led3 = D2;

#define SOUND_VELOCITY 0.034

#define CM_TO_INCH 0.393701

long duration;

float distanceCm;

float distanceInch;

int percentage;

String textMsg;
String SensorID = "SensorID = 1";

#define ON_Board_LED 2  
ESPTimeHelper ETH;

void setup() {

  Firebase.begin (FIREBASE_HOST, FIREBASE_AUTH);
  Serial.begin(115200);
  Serial.println();
  pinMode(led1, OUTPUT);

  pinMode(led2, OUTPUT);

  pinMode(led3, OUTPUT);

  digitalWrite(led1, LOW);

  digitalWrite(led2, LOW);

  digitalWrite(led3, LOW);


  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output

  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  delay(1000);

  pinMode(ON_Board_LED,OUTPUT);
  digitalWrite(ON_Board_LED, HIGH);

  Serial.print("Connecting to AP");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    digitalWrite(ON_Board_LED, LOW);
    delay(100);
    digitalWrite(ON_Board_LED, HIGH);
    delay(100);
  }

  digitalWrite(ON_Board_LED, HIGH);

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  ETH.TZ = 5.30;
 
  ETH.DST_MN = 0;
  Serial.println("Getting time data from server. Please wait...");
  if (!ETH.setClock(ETH.TZ, ETH.DST_MN)){
    Serial.println("Can't set clock...");
    return;
  }
  Serial.println("Successfully Get time data.");

  smtp.debug(1);

  smtp.callback(smtpCallback);

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;

  message.sender.name = "Smart Drainage System";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = " sensor data report";
  message.addRecipient("NodeMCUESP8266", "smartdrainagesystem12@gmail.com");

  message.text.charSet = "us-ascii";

  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_high;

  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

}

void loop() {

  digitalWrite(trigPin, LOW);

  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);

  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  distanceCm = duration * SOUND_VELOCITY/2;

  percentage = map(distanceCm, 47,2,0,100);

  String distance2 = String(distanceCm) ;
  String distance1 = String(distanceCm) + " Cm";
  String firebasedistance = String (distance2)  ;
  String percentage1 = "percentage : " + String(percentage) + " %";
  String firebasepercentage = String (percentage);
  String firebasesensorid = String (SensorID);
 
  Firebase.setString("SensorID",firebasesensorid);
  Firebase.setString("Percentage",firebasepercentage);

  if (isnan(distanceCm) || isnan(percentage)) {
    Serial.println("Failed to read data from  sensor!");
    delay(500);
    return;
  }



  Serial.printf("Date/Time: %02d/%02d/%d %02d:%02d:%02d", ETH.getDay(), ETH.getMonth(), ETH.getYear(), ETH.getHour(), ETH.getMin(), ETH.getSec());
  Serial.print(F(" | distance: "));
  Serial.print(distanceCm);
  Serial.print(F(" cm"));
  Serial.print(F(" | percentage: "));
  Serial.print(percentage);
  Serial.println(" %");

  if(percentage > p_Threshold_above && trigger_Send == true) {
    textMsg = textMsg + "percentage above threshold value : "  + String(p_Threshold_above) + " %" + "\n" + SensorID + "\n";
    Serial.print(textMsg);
    Serial.println("Send  sensor data via email");
    setTextMsg();
    sendTextMsg();
    textMsg = "";
    trigger_Send = false;
    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);
    digitalWrite(led3, HIGH);
  }

  if(percentage < p_Threshold_below && trigger_Send == false) {
    textMsg = textMsg + "percentage below threshold value of  : "   + String(p_Threshold_below) + " %" + "\n" + SensorID + "\n" ;
    Serial.print(textMsg);
    Serial.println("Send  sensor data via email");
    setTextMsg();
    sendTextMsg();
    textMsg = "";
    trigger_Send = true;
    digitalWrite(led1, LOW);

    digitalWrite(led2, HIGH);

    digitalWrite(led3, LOW);
  }

  if(  (percentage >= 0) && (percentage <= 25)   )

{

  digitalWrite(led1, HIGH);

  digitalWrite(led2, LOW);

  digitalWrite(led3, LOW);
 
digitalWrite(D4, LOW);

}
 
  delay(3000);
}

void setTextMsg() {
  textMsg = textMsg + "distance : " + String(distanceCm) + " °C"  + "\n" + "percentage : " + String(percentage) + " %";
  message.text.content = textMsg.c_str();

}

void sendTextMsg() {

  message.addHeader("Message-ID: <dht11.send@gmail.com>");

  if (!smtp.connect(&session)) return;



  if (!MailClient.sendMail(&smtp, &message)) Serial.println("Error sending Email, " + smtp.errorReason());

}


void smtpCallback(SMTP_Status status) {

  Serial.println(status.info());


  if (status.success()) {
    Serial.println("----------------");
    Serial.printf("Message sent success: %d\n", status.completedCount());
    Serial.printf("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      struct tm *localtime_r( const time_t *timer, struct tm *buf );

      Serial.printf("Message No: %d\n", i + 1);
      Serial.printf("Status: %s\n", result.completed ? "success" : "failed");
      Serial.printf("Date/Time: %02d/%02d/%d %02d:%02d:%02d\n", dt.tm_mday, dt.tm_mon + 1, dt.tm_year + 1900, dt.tm_hour, dt.tm_min, dt.tm_sec);
      Serial.printf("Recipient: %s\n", result.recipients);
      Serial.printf("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }

}
