

const int inPin = 21; 
const int inInt = 2; 

void setup() {
  pinMode(inPin, INPUT);
  Serial.begin(115200);
  attachInterrupt(inInt, handleInterrupt, CHANGE);
}

//volatile unsigned int count = 0;
volatile char temp,level;

void loop() {
//  Serial.println(millis());
//  Serial.println(count);
  Serial.print(temp,DEC);Serial.print("c ");Serial.println(level,DEC);
  delay(500);

}

void handleInterrupt() {
  /* This rutine has been optimized to just capture the part of the packet 
     that has the temp & level
     On averages it uses 550 us per second
  */
  static unsigned int duration;
  static unsigned long lastTime;
  static char c=15;
  static byte data[5];
  static bool expectMark;
  unsigned long time = micros();
  duration = time - lastTime;
  if(5000>duration && duration>3000){
    //Match MARK_HEADER (new packet)
    c=39;
    memset(data, 0, sizeof(data));
    data[2]=0;
    data[3]=0;
    expectMark=false;
  }
  if(c>=16){ 
    if(expectMark){
      // Skip MARK
      expectMark=false;
    }else{
      // Decode SPACE
      if(1500>duration && duration>1300){
        // #define SPACE_ONE 1500
        data[c>>3]|=(1<<(c & B111));
        c--;
      }else if(800>duration && duration>400){
        // #define SPACE_ZERO 600
        c--;
      }
      expectMark=true;
      if(c==15){
        // Got all the bits
        temp=data[3];
        level=data[2];
      }
    }
  }
  lastTime = time;  
//  count+=micros()-time;
}
