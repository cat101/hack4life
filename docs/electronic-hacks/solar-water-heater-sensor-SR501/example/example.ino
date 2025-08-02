#include <Arduino.h>
//#define SENSOR_TYPE_SR500 2       //The value should be the pin to use (GHBO1)
#define SENSOR_TYPE_SR501 Serial3 //The value should be the serial port to use

extern volatile char tempWH,levelWH; // temp in Celsius and level goes from 0 to 4
void initWaterHeaterSensorSR500(); //Attaches the interrupt handler
void initWaterHeaterSensorSR501();
bool pollsensorSR501();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  #if defined(SENSOR_TYPE_SR500)
    initWaterHeaterSensorSR500(); // The sensor continuously sends serial readings which are captured by an ISR
  #elif defined(SENSOR_TYPE_SR501)
    initWaterHeaterSensorSR501(); // The sensor continuously sends serial readings which are captured by an ISR
  #endif   
  Serial.println("Starting");
  pinMode(13, OUTPUT);  //Enable LED pin
  digitalWrite(13, LOW); // Turn off LED
}

char lastTempWH=0,lastLevelWH=5; // temp in Celsius and level goes from 0 to 4
void loop() {
  #if defined(SENSOR_TYPE_SR501)
    // Try to get data from serial sensor
    if(pollsensorSR501()) {
      // Update global variables with serial sensor data
      Serial.print(tempWH,DEC);Serial.print("c - ");Serial.print(levelWH,DEC);Serial.println("%");
    }
  #elif defined(SENSOR_TYPE_SR500)
    if(lastTempWH!=tempWH || lastLevelWH!=levelWH){
      Serial.print(tempWH,DEC);Serial.print("c - L");Serial.println(levelWH,DEC);
      lastTempWH=tempWH;
      lastLevelWH=levelWH;
    }
    delay(1000);
  #endif   
}
