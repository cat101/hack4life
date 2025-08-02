/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.0  January 2013
 * Copyright 2013 by Chris Young http://cyborg5.com
 * Based on original example sketch for IRremote library 
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */
/*
 * IRLib: IRrecvDump - dump details of IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 */

#include <IRLib.h>
#include "IRLibMatch.h"


int RECV_PIN = 22;

IRrecv My_Receiver(RECV_PIN);

unsigned int Buffer[RAWBUF];

class TempNLevelSensor: public virtual IRdecodeBase 
{
public:
  virtual bool decode(void);
};

bool TempNLevelSensor::decode(void) {
//  ATTEMPT_MESSAGE(F("Temp"));  
//  if(!decodeGeneric(42,500*16,500*4,500,500*3,500,500)) return false;
  //Serial.println("Trying");
//  if(!decodeGeneric(0,500*16,500*4,500,500,500*3,500)) return false;
//  if(!decodeGeneric(0,3800,600,0,500,500*3,500)) return false;
  // Marks are LOWs on the OLS!!!
  // the first 8ms are considered gap
//  if(!decodeGeneric(0,3800,0,500*3,500,500,0)) return false;
  rawlen=82;
  if(!decodeGeneric(0,3800,0,1400,400,575,0)) return false;
//  bits = (offset - 1) / 2;
//  value = data;
  decode_type = NECX;
  return true;
}

//  bool TempNLevelSensor::decode(void) {
//    // Marks are LOWs on the OLS!!!
//  int Raw_Count=0;
//  int Head_Mark=3800;
//  int Mark=600;int Space_One=1375;int Space_Zero=600;
//  // If raw samples count or head mark are zero then don't perform these tests.
//// Some protocols need to do custom header work.
//  long data = 0;  int Max; int offset;
//  //if (Raw_Count) {if (rawlen != Raw_Count) Serial.println("RAW_COUNT_ERROR");return RAW_COUNT_ERROR;}
//  if (Head_Mark) {if (!MATCH_MARK(rawbuf[1],Head_Mark))    {Serial.println("HEADER_MARK_ERROR");return false;}}
//    Max=rawlen; //ignore stop bit
//    offset=2;//skip initial gap and plus two header items
//    while (offset < Max) {
//      if (!MATCH_MARK (rawbuf[offset],Mark)) {Serial.println("DATA_MARK_ERROR");return false;}
//      offset++;
//      if (MATCH_SPACE(rawbuf[offset],Space_One)) {
//        data = (data << 1) | 1;
//      } 
//      else if (MATCH_SPACE (rawbuf[offset],Space_Zero)) {
//        data <<= 1;
//      } 
//      else {Serial.println(offset);Serial.println("DATA_SPACE_ERROR");return false;}
//      offset++;
//    }
//    bits = (offset - 1) / 2;
//    // Success
//  value = data;
//  return true;
//  }

TempNLevelSensor My_Decoder;

void setup()
{
  Serial.begin(9600);
  My_Receiver.enableIRIn(); // Start the receiver
  My_Decoder.UseExtnBuf(Buffer);
}

void loop() {
  if (My_Receiver.GetResults(&My_Decoder)) {
    //Restart the receiver so it can be capturing another code
    //while we are working on decoding this one.
    My_Receiver.resume(); 
    My_Decoder.decode();
    My_Decoder.DumpResults();
  }
}

