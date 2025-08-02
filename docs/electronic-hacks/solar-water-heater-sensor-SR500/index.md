---
layout: default
title: "Reversing a Solar Water Heater Sensor - GHBO1/SHBO1"
parent: Electronic Hacks & Reverse Engineering
nav_order: 301
date: 2021
---

# Reversing a Solar Water Heater Sensor - GHBO1/SHBO1 (2021)

The GHBO1/SHBO1 is a temperature and level sensor for solar water heaters. It is manufactured by Sunhome Technology Co.,Ltd & Green Homebuilders Inc. The manufacturer does not provide information on how to use it. They only sell it as a replacement part for solar water heater controllers like the [SR500](http://www.chinasunpark.com/DIYtrade/SR500-EN-201000929.pdf).

## Exploring the sensor

First I opened the case and noticed a couple of things

![Solar water heater level sensor 01](images/Sensor%20de%20nivel%20para%20termotanque%2001.jpg){: width="50%"}

- It has a microcontroler so it was most likely using some sort of comm protocol
- It has voltage regulator and the red wire is labeled 12V (i.e. the black & red wires are power). Using a bench power supply I tested that it can work with 5V as well (not with 3.3V)
- On the left there are 7 wires going in. Two are labeled T1 and the other five L0 to L4. This led me to think that the level sensor was discrete (5 steps) and there was a single temp sensor
- I also noted the ICs labels 62AJ64M HC04 and the model on the PCB SR1061B3 but none yielded any results

## Analyzing the comm protocol

I first measured with a multimeter the white wire to check if the output was TTL. I found out that the output voltage matches the supply voltage (i.e. if VCC=5V then it behaves like TTL, but if VCC=12 volts then the data voltage is 12V). I then hooked the sensor to a logic analyzer I made using an Arduino mega ([https://github.com/gillham/logic_analyzer](https://github.com/gillham/logic_analyzer)) and an open source logic sniffer ([http://www.lxtreme.nl/ols/](http://www.lxtreme.nl/ols/)). After fiddling with different sample rates I realized that the sensor was sending 70ms packets every 1 seconds (940ms). Each packet looks like this (shown at 50Khz sampling)

![25c - 4-4](images/25c%20-%204-4.png){: width="75%"}

I realized that it looked a lot like a the packets used on IR controls. In fact I used [https://github.com/cyborg5/IRLib](https://github.com/cyborg5/IRLib) to decode it and manage to do it with these parameters *decodeGeneric(0,3800,0,1400,400,575,0)*. More generally what I found is that the protocol has a header bit followed by 40 bits encoded with varying. This is the exact timing in usec

- MARK_HEADER 7000
- SPACE_HEADER 3000
- MARK 500
- SPACE_ONE 1500
- SPACE_ZERO 600

Finally I put together a simple arduino sketch (see at the bottom of this page) to read the package and try to make sense of them. A typical packet looks like AA1A0328BB. After experimenting with bottles filled with water at different levels & temperatures I found this

- AA → Constant. It never changed on my experiments
- **1A → Temperature in Celsius. In this case 26c**
  - It seems that the temperature sensor is located towards the top of the stick
- **03 → Water level as a discrete value from 0 to 4**
  - Level 0 spans from empty to a fifth of the stick
- 28 → Constant. It never changed on my experiments
- BB → Some sort of checksum that I did not bother to decode

## Additional Resources
For those interested in diving deeper into the reverse engineering process, you can browse the **[Exploration directory]({{ site.github.repository_url }}/tree/{{ site.github.source.branch | default: 'master' }}/docs/{{ page.path | remove: '.md' | remove: '/index' }}/Exploration)** on GitHub, which contains Arduino sketches, logic analyzer tools, raw recordings, and statistical analysis from the original research.

## Arduino routine for reading the sensor

```cpp
// Extracted from http://hack4life.pbworks.com/Arduino%20Solar%20Water%20Heater%20Sensor

const int inPin = 22;

void setup() {
  pinMode(inPin, INPUT);
  Serial.begin(9600);
}

#define TIMEOUT 50000
#define MARK_HEADER 7000
#define SPACE_HEADER 3000
#define MARK 500
#define SPACE_ONE 1500
#define SPACE_ZERO 600

int expectPulse(int val){
  unsigned long t=micros();
  while(digitalRead(inPin)==val){
    if( (micros()-t)>TIMEOUT ) return 0;
  }
  return micros()-t;
}

// temp in celsious and level goes from 0 to 3
bool readTempNLevelSensor(char inPin, char &temp, char &level){
   byte data[5]={0,0,0,0,0};
   unsigned long val1;
   unsigned long st=micros();
   val1 = expectPulse(HIGH);
   if(val1>MARK_HEADER){
     val1 = expectPulse(LOW);
     if(val1>SPACE_HEADER){
       int c=39;
       for(;c>-1;c--){
         val1 = expectPulse(HIGH);
         if(val1<MARK){
          val1 = expectPulse(LOW);
          if(val1==0){
            //Serial.println("Mark error 0");
            break; 
          }
          if(val1<SPACE_ZERO){
            //0
            //data[c>>3]|=(1<<(c & B111));
          }else if(val1<SPACE_ONE){
            //1
            data[c>>3]|=(1<<(c & B111));
          }else{
           //Serial.print(val1);Serial.println(" Space error");
           break; 
          }
         }else{
          //Serial.println("Mark error");
          break; 
         }  
       }
       // Each reading should not take more than 70ms (use time to detect errors)
       if(micros()-st<70000){
         temp=data[3];
         level=data[2];
        //Serial.print(data[3]);Serial.print(" ");Serial.println((data[2]));
        //Serial.print(data[4],HEX);Serial.print(" ");Serial.print(data[3],HEX);Serial.print(" ");Serial.print(data[2],HEX);Serial.print(" ");Serial.print(data[1],HEX);Serial.print(" ");Serial.println(data[0],HEX);
         return true;
       }
     }
   }  
   return false;
}

void loop() {
  char temp,level;
  if(readTempNLevelSensor(inPin, temp, level)){
    Serial.print(temp,DEC);Serial.print("c ");Serial.println(level,DEC);
  }
}
```

## Wemos D1 mini code for reading the sensor

This is hdelaroza's code to integrate the sensor via MQTT with Home Assistant

```cpp
#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
//////////////////////////////////////////
#include <ESP8266WebServer.h>
 
//USER CONFIGURED SECTION START//
const char* ssid = "MY_WIFI_NETWORK";
const char* password = "MY_WIFI_PASSWORD";
const char* host = "termotanque_sensor";
const char* mensaje = "Sensor termotanque solar: ";
const char* mqtt_server = "MY_MQTT_SERVER";
const int mqtt_port = 1883;
const char *mqtt_user = "MY_MQTT_USERNAME";
const char *mqtt_pass = "MY_MQTT_PASSWOR";
const char *mqtt_client_name = "termotanque_sensor"; // Client connections can't have the same connection name
 
 
const int inPin = 15; //D8, pin where the sensor is connected (has pulldown resistor)
//long lastPub=0;
char temp,level;
char temp2=-1,level2=-1;
String mensaje_mqtt;
 
#define TIMEOUT 50000  // in microseconds
#define MARK_HEADER 7000
#define SPACE_HEADER 3000
#define MARK 520
#define SPACE_ONE 1530
#define SPACE_ZERO 600
//USER CONFIGURED SECTION END//
 
ESP8266WebServer server(80);
 
WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
 
bool boot = true;
 
//Functions
 
void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA); //SACO EL FaryLink
  WiFi.hostname(host);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
 
void reconnect() 
{
  // Loop until we're reconnected
  int retries = 0;
  while (!client.connected()) {
    if(retries < 15)
    {
        Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) 
      {
        Serial.println("connected");
        // Once connected, publish an announcement...
        if(boot == true)
        {
          client.publish("checkIn/termo_solar","Rebooted");
          boot = false;
        }
        if(boot == false)
        {
          client.publish("checkIn/termo_soalar","Reconnected"); 
        }
        // ... and resubscribe
        client.subscribe("doorbell/commands");
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if(retries > 14)
    {
    ESP.restart();
    }
  }
}
 
void checkIn()
{
  client.publish("checkIn/TermoSolar","OK");
}
 
//TERMO SOLAR
int expectPulse(int val){
  unsigned long t=micros();
  while(digitalRead(inPin)==val){
    if( (micros()-t)>TIMEOUT ) return 0;
  }
  return micros()-t;
}
 
bool readTempNLevelSensor(char inPin, char &temp, char &level){
   byte data[5]={0,0,0,0,0};
   unsigned long val1;
   unsigned long st=micros();
   val1 = expectPulse(HIGH);
   if(val1>MARK_HEADER){
     val1 = expectPulse(LOW);
     if(val1>SPACE_HEADER){
       int c=39;
       for(;c>-1;c--){
         val1 = expectPulse(HIGH);
         if(val1<MARK){
          val1 = expectPulse(LOW);
          if(val1==0){
            Serial.println("Mark error 0");
            break; 
          }
          if(val1<SPACE_ZERO){
            //0
            //data[c>>3]|=(1<<(c & B111));
          }else if(val1<SPACE_ONE){
            //1
            data[c>>3]|=(1<<(c & B111));
          }else{
           Serial.print(val1);Serial.println(" Space error");
           break; 
          }
         }else{
          Serial.print(val1);Serial.println(" Mark error");
          break; 
         }  
       }
       // Each reading should not take more than 70ms (use time to detect errors)
       if(micros()-st<70000){
         temp=data[3];
         level=data[2];
         //Serial.print(data[3]);Serial.print(" ");Serial.println((data[2]));
         //Serial.print(data[4],HEX);Serial.print(" ");Serial.print(data[3],HEX);Serial.print(" ");Serial.print(data[2],HEX);Serial.print(" ");Serial.print(data[1],HEX);Serial.print(" ");Serial.println(data[0],HEX);
         return true;
       }
    }
   }  
   return false;
}
 
//Run once setup
void setup() {
  pinMode(inPin, INPUT);
  Serial.begin(115200);
 
  setup_wifi();
 
  client.setServer(mqtt_server, mqtt_port);
//  client.setCallback(callback);
 
  ArduinoOTA.setHostname("termo_solar");
  ArduinoOTA.begin(); 
 
  timer.setInterval(120000, checkIn);
//  timer.setInterval(500, getFrontState);
//  timer.setInterval(200, getTermoSolar);
//////////////////////////////////////////
  server.on("/other", []() {   //Define the handling function for the path
     server.send(200, "text / plain", "Other URL");
  });
 
  server.on("/", handleRootPath);    //Associate the handler function to the path
  server.begin();                    //Start the server
  Serial.println("Server listening");
//////////////////////////////////////////  
}
 
void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
////////////////////////////////////////////////////
  server.handleClient();         //Handling of incoming requests
///////////////////////////////////////////////////
  client.loop();
  ArduinoOTA.handle();
  if(readTempNLevelSensor(inPin, temp, level)){
//    Serial.print(temp,DEC);Serial.print("c ");Serial.println(level,DEC);
//    mensaje_mqtt = String(int (temp))+String(int (level));
  }
  if ((temp==temp2)|| (level==level2)){
    mensaje_mqtt = "{\"Temperature\":\"" +String(int (temp))+"\",\"Level\":\""+String(int (level))+"\"}";
 
    Serial.println(mensaje_mqtt);
 
    char charBuf[mensaje_mqtt.length() + 1];
    mensaje_mqtt.toCharArray(charBuf,mensaje_mqtt.length() + 1);
    client.publish("TermoSolar", charBuf);
  }
  temp2=temp;
  level2=level;
  timer.run();
}
 
///////////////////////////////////////////////////////////////
void handleRootPath() {            //Handler for the rooth path
  server.send(200, "text/plain", mensaje_mqtt);
}
///////////////////////////////////////////////////////////////
```