// --- Run in example.ino ---
#include "Arduino.h"
//#define SENSOR_TYPE_SR500 2       //The value should be the pin to use (GHBO1)
#define SENSOR_TYPE_SR501 Serial3 //The value should be the serial port to use

// Run in Techo/config.cpp
//#include "config.h"
//#include "debug.h"

//volatile unsigned int count = 0;
volatile char tempWH=1; // temp in Celsius
volatile unsigned char levelWH=1; // level goes from 0 to 100


#ifdef SENSOR_TYPE_SR500
// The input needs to generate I/O interrupts (https://www.arduino.cc/en/Reference/AttachInterrupt)
const int inPin = SENSOR_TYPE_SR500; //This is hard coded for now...it should really be WATER_TANK_SENSOR
const int inInt = digitalPinToInterrupt(inPin); 
byte wh_data[5];

void handleSensorInterrupt();

void initWaterHeaterSensorSR500() {
  attachInterrupt(inInt, handleSensorInterrupt, CHANGE);
}


void handleSensorInterrupt() {
  /* This routine has been optimized to just capture the part of the packet 
     that has the temp & level
     On averages it uses 550 us per second
  */
  static unsigned int duration;
  static unsigned long lastTime;
  static char c=15; //Current bit
  static bool expectMark;
  unsigned long time = micros();
  duration = time - lastTime;
  if(5000>duration && duration>3000){
    //Match SPACE_HEADER typically 4000us (new packet)
    c=39;
    // memset(data, 0, sizeof(data));
    wh_data[2]=0;
    wh_data[3]=0;
    expectMark=true;
  // }else if(c>0){ // Get the whole packet
  }else if(c>=16){ 
    if(expectMark){
      // Skip MARK
      expectMark=false;
    }else{
      // Decode SPACE
      if(1700>duration && duration>1300){
        // #define SPACE_ONE 1500
        wh_data[c>>3]|=(1<<(c & B111));
        c--;
      }else if(800>duration && duration>400){
        // #define SPACE_ZERO 500
        c--;
      }
      expectMark=true;
      if(c==15){
        // Got all the bits
        tempWH=wh_data[3];
        levelWH=(wh_data[2] & 0x0F)*25U; // Convert to percentage (0-100)
      }
    }
  }
  lastTime = time;  
//  count+=micros()-time;
}

#endif

#ifdef SENSOR_TYPE_SR501
// Initialize Serial for sensor communication
void initWaterHeaterSensorSR501() {
  SENSOR_TYPE_SR501.begin(4800, SERIAL_8N1);

}

// --- Integer-Based Temperature Calculation for SR501/B03 Sensor ---
// Corrected version using two separate arrays and only 16-bit integer math.

// This special library is needed to read from Flash memory (PROGMEM).
#include <avr/pgmspace.h>

// --- Lookup Table Definition ---
// The table is stored in PROGMEM (flash memory) to save RAM.
// It maps 12-bit ADC values to temperatures in tenths of a degree Celsius (e.g., 250 = 25.0 C).
const int NUM_TABLE_ENTRIES = 11;

// Calibration array for the 12-bit ADC measurement points
const uint16_t adc_table[NUM_TABLE_ENTRIES] PROGMEM = {
  961, 1249, 1562, 1895, 2237, 2577, 2902, 3202, 3471, 3706, 3905
};

// Calibration array for the corresponding temperature points (in tenths of a degree)
const int16_t temp_table[NUM_TABLE_ENTRIES] PROGMEM = {
  0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000
};

// --- Main Calculation Function ---
/**
 * @brief Converts a raw 12-bit ADC value to temperature using a lookup table and 16-bit integer math.
 * * @param adc_val The raw 12-bit integer value (0-4095) from the sensor.
 * @return The calculated temperature in tenths of a degree Celsius (e.g., 985 means 98.5 C).
 */
int16_t getTemperature(uint16_t adc_val) {
  // Handle edge cases: if the reading is outside our table's range,
  // return the lowest or highest temperature without calculating.
  if (adc_val <= pgm_read_word(&adc_table[0])) {
    return pgm_read_word(&temp_table[0]);
  }
  if (adc_val >= pgm_read_word(&adc_table[NUM_TABLE_ENTRIES - 1])) {
    return pgm_read_word(&temp_table[NUM_TABLE_ENTRIES - 1]);
  }

  // Find the segment in the lookup table that contains our ADC value.
  uint8_t i = 0;
  for (i = 0; i < NUM_TABLE_ENTRIES - 1; i++) {
    if (adc_val < pgm_read_word(&adc_table[i + 1])) {
      break; // Found the lower bound of our segment
    }
  }

  // Get the four points defining the line segment from PROGMEM
  uint16_t adc_low = pgm_read_word(&adc_table[i]);
  uint16_t adc_high = pgm_read_word(&adc_table[i + 1]);
  int16_t temp_low = pgm_read_word(&temp_table[i]);
  int16_t temp_high = pgm_read_word(&temp_table[i + 1]);

  // Perform linear interpolation using ONLY 16-bit integer math.
  // The intermediate multiplication (adc_delta * temp_range) is safe from overflow
  // because the maximum value (~300 * 100 = 30000) fits within a signed 16-bit integer.
  int16_t adc_range = adc_high - adc_low;
  int16_t temp_range = temp_high - temp_low;
  int16_t adc_delta = adc_val - adc_low;

  // The core interpolation formula using 16-bit variables
  int16_t interpolated_temp = temp_low + ( (adc_delta * temp_range) / adc_range );

  return interpolated_temp;
}


// #define PRINT_PACKET
// Poll serial SR501 sensor for temperature and level data
// Returns true if valid data received, false on timeout, error or incomplete data
bool pollsensorSR501() {
  // State machine for packet parsing
  enum ParseState { 
    WAIT_START1 = 0,
    WAIT_START2 = 1,
    READ_T2 = 5, 
    READ_T1 = 6, 
    READ_L2 = 27, 
    READ_L1 = 28, 
    READ_LAST = 29 
  };
  static unsigned char state = WAIT_START1;
  unsigned int t;
  #ifdef PRINT_PACKET
  static char packet[READ_LAST+1]; // Buffer to hold the packet data
  #endif
  
  // static int skipCount = 0;
  static byte T1 = 0, T2 = 0, L1 = 0, L2 = 0;
  
  if(!SENSOR_TYPE_SR501.available()) {
    return false; // Wait for data
  }
    
  byte b = SENSOR_TYPE_SR501.read();
//    Serial.println(b, HEX);
//  Serial.println(state, DEC);   
  // DEBUG_PRINT_P("--> Read %02hX\n",b); 
  #ifdef PRINT_PACKET
  packet[state] = b;
  #endif
    
  switch(state) {
    case WAIT_START1:
      if(b == 0x00) {
        state += 1;
      } else {
        state = WAIT_START1; // Reset if not matching
      }
      break;
      
    case WAIT_START2:
      if(b == 0x01) {
        state += 1;
        // DEBUG_PRINT_P("Acquired Water Tank sensor (%huc, %hu%%)\n",sensors.analogSensors[TE_T04_O_OFF+1], sensors.analogSensors[TE_T04_O_OFF]);
        // DEBUG_PRINT_P("Got header");
      } else {
        if (b != 0x00) // This ensures that we will start even if there are many zeros before the 1
          state = WAIT_START1; // Reset if not matching
      }
      break;
            
    case READ_T2:
      T2 = b;
      state = READ_T1;
      break;

    case READ_T1:
      T1 = b;
      state += 1;
      break;
      
            
    case READ_L2:
      L2 = b;
      state = READ_L1;
      break;
      
    case READ_L1:
      L1 = b;
      state += 1;
      break;

    case READ_LAST:
      state = WAIT_START1;

      // Combine the data bytes (T1 and T2) into the 12-bit raw value 'V'.
      t = ((unsigned int)T1 << 8) | T2;

      // Divide by 10 to remove decimals. Add 10% reduction for safety margin.
      // Specially to account for when the tank is filling.
      tempWH=getTemperature(t)/(10U + 1U); 

      //Raw level mapping and normalized value
      //L0 0x0026 in bin = 0b100110    -->   8% --> This means that no water is touching the sensor (not really 8)
      //L1 0x0092 in bin = 0b10010010  -->  32%
      //L2 0x00e4 in bin = 0b11100100  -->  50%
      //L3 0x013e in bin = 0b111110    -->  71%
      //L4 0x0149 in bin = 0b1001001   -->  72% -> 80%
      //L5 0x01c2 in bin = 0b11000010  --> 100%
      // Serial.print("Raw level:");Serial.print(L1, HEX);Serial.print(" "); Serial.println(L2, HEX);    

      // level = (100U*((L1 << 8) + L2))/450U;
      t = (100U*(((unsigned int)L1 << 8) + L2))/450U;
      levelWH = t;

      // L0 is really means that no water touches the sensor
      if(levelWH <= 8)
        levelWH = 0;
      // L4 is really mid way between L3 and L5
      else if(levelWH > 72 && levelWH < 80)
        levelWH = 80;

      #ifdef PRINT_PACKET
        DEBUG_PRINT_P("SR501 Packet: ");
        for(int i = 0; i <= READ_LAST; i++) {
          DEBUG_PRINT_P("%02hX ", packet[i]);
        }
        // // Print any remaining bytes in the packet
        // while(SENSOR_TYPE_SR501.peek()!=0) {
        //   b = SENSOR_TYPE_SR501.read();
        //   DEBUG_PRINT_P("%02hX ", b);
        // }

        DEBUG_PRINT_P("\n");
        DEBUG_PRINT_P("--> Temp %02hX%02hX\n",T1, T2); 
        DEBUG_PRINT_P("--> Level %02hX%02hX\n",L1, L2);
      #endif

      return true; // Success!
      
    default:
      state += 1;
//      Serial.println(state, DEC);    
  }
  
  return false; 
}
#endif
