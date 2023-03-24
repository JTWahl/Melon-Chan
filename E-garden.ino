/**********Code By - RR**********/

#include <WiFi.h>
#include <HTTPClient.h>
#include "ESP32_MailClient.h"
#include <Alfredo_NoU2.h>

#define emailSenderAccount    "ROBOT EMAIL ADDRESS"
#define emailSenderPassword   "ROBOT EMAIL PASS"
#define emailRecipient        "YOUR EMAIL HERE"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define uS_TO_S_FACTOR        1000000             //conversion from microseconds to seconds
#define TIME_TO_SLEEP         18000               //seconds to sleep
#define TIME_TO_PUMP          30000               //ms water pump is active

const char* ssid      = "YOUR WIFI SSID HERE";
const char* password  = "YOUR WIFI PASS HERE";
const char* host      = "api.thingspeak.com";

const int moistureSensorPin = 35;
const int lightSensorPin = 34;
const int waterSensorPin = 32;

const int httpPort = 80;

RTC_DATA_ATTR boolean initializingEmail = 0;

int moistureSensorValue = 0;
int lightSensorValue = 0;
int waterSensorValue = 0;

NoU_Motor pump(1);
SMTPData smtpData;
void sendCallback(SendStatus info);

void setup() 
{ 
  //start serial comms
  Serial.begin(9600); 

  //configure sleep timer
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  //start WiFi connection
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  //uncomment if activation email is required
  if (!initializingEmail) 
  {
     sendEmail("Connection Established!", "Hello Friend! Melon-chan has connected to the WiFi!");
  }
  delay(1000);
}

void loop() 
{   
  boolean hasWater = false;
  
  //read in sensor data
  moistureSensorValue = analogRead(moistureSensorPin);   
  lightSensorValue = analogRead(lightSensorPin);
  waterSensorValue = analogRead(waterSensorPin);

  Serial.println("Moisture: " + moistureSensorValue);
  Serial.println("Light Level: " + lightSensorValue);
  Serial.println("Water Level: " + waterSensorPin);

  //put to sleep if solar cell is receiving significant charge
/*  if (lightSensorValue <= 500) 
  {
    //sleep for 5 hours
    deactivatePump();
    Serial.flush();
    esp_deep_sleep_start();
  }
*/

  if (waterSensorValue < 300) {
    sendEmail("Water Level Low!", "Hello Friend! Melon-chan wanted to let you know that her water tank is running low!");
    hasWater = false;  
  }
  else {
    hasWater = true;
  }
  
  //activate pump if necessary
  //0 is min wetness, 4095 is max wetness
  if (moistureSensorValue <= 2100 && hasWater) 
  {
    activatePump();
  }
  else 
  {
    deactivatePump();

    //sleep for 5 hours
    Serial.flush();
    esp_deep_sleep_start();
  }
  
  delay(500); 
}



//turns off water pump
void deactivatePump() 
{
  pump.set(0);
}

//turns on water pump
void activatePump() 
{
  String emailOutput;

  emailOutput = "Hello Friend! Melon-chan is just letting you know that they are watering melons now! (Data out of 4095) --- Soil Moisture: " 
  + String(moistureSensorValue) + ", Light Sensor: " + String(lightSensorValue);
  
  sendEmail("Watering Melons Now!", emailOutput);
  
  Serial.println("Pump State: ACTIVE");
  Serial.println();
  pump.set(1);
  delay(TIME_TO_PUMP);
}

// Callback function to get the Email sending status
void sendCallback(SendStatus msg) 
{
  // Print the current status
  //Serial.println(msg.info());

  // Do something when complete
  if (msg.success()) 
  {
    Serial.println("MESSAGE SUCCESSFULLY SENT");
  }
}

void sendEmail(String emailSubject, String emailMessage)
{
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
  smtpData.setSender("MelonBot", emailSenderAccount);
  smtpData.setPriority("High");
  smtpData.setSubject(emailSubject);
  smtpData.setMessage("<div style=\"color:#2f4468;\"><h1>" + emailMessage + "</h1><p>- Sent from ESP32 board</p></div>", true);
  smtpData.addRecipient(emailRecipient);
  smtpData.setSendCallback(sendCallback);

  if(!MailClient.sendMail(smtpData)) 
  {
    Serial.println("Error Sending Email, " + MailClient.smtpErrorReason());
  }

  smtpData.empty();  
}
