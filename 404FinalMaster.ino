#include<mwc_stepper.h>
#include "HX711.h"
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#define led_pin 3   

//rf pins
#define CE_PIN 53
#define CSN_PIN 3
RF24 radio(CE_PIN, CSN_PIN);
uint8_t address[][6] = { "1Node", "2Node" };
bool radioNumber = 1;
bool role = false;
uint8_t payload[6]={0,0,0,0,0,0};

//eStop
#define eStop_pin 39

//hx 711 pins
const int LOADCELL_DOUT_PIN = 11;
const int LOADCELL_SCK_PIN = 12;
HX711 scale;

//end stop pins
#define xStop 35
#define yStop 37

//Stepp pins
#define EN_PIN1 29
#define DIR_PIN1 31
#define STEP_PIN1 9
#define EN_PIN2 28
#define DIR_PIN2 30
#define STEP_PIN2 10
#define RPM 50
#define PULSE 1600
#define CLOCKWISE 1
#define OTHERWISE 0
MWCSTEPPER xNema(EN_PIN1, DIR_PIN1, STEP_PIN1);
MWCSTEPPER yNema(EN_PIN2, DIR_PIN2, STEP_PIN2);
int CurrX=0;
int CurrY=0;

void setup() {
  Serial.begin(115200);
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(payload));
  radio.openWritingPipe(address[radioNumber]);
  radio.openReadingPipe(1, address[!radioNumber]);  
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  xNema.init();
  yNema.init();
  radio.startListening();
}

void loop() {
  uint8_t pipe;
  if (radio.available(&pipe))
  {
     uint8_t bytes = radio.getPayloadSize();
     radio.read(&payload, bytes);
     for(int i=0; i<sizeof(payload); i++){
       Serial.println(payload[i]);
     }
  }
}

int motorHome(){
  xNema.set(OTHERWISE, RPM, PULSE);
  yNema.set(OTHERWISE, RPM, PULSE);
  while (~xStop)
  {
    xNema.run();
  }
  while (~yStop)
  {
    yNema.run();
  }
}

void motorFull(){
  //4456 Ticks full extend
  xNema.set(CLOCKWISE, RPM, PULSE);
  yNema.set(CLOCKWISE, RPM, PULSE);

  for (size_t i = 0; i < 4456; i++)
  {
    xNema.run();
    yNema.run();
  }
}

void motorHalf(){
  //4456 Ticks full extend
  xNema.set(CLOCKWISE, RPM, PULSE);
  yNema.set(CLOCKWISE, RPM, PULSE);

  for (size_t i = 0; i < 2228; i++)
  {
    xNema.run();
    yNema.run();
  }
}

void eStop(){
  digitalWrite(eStop_pin, HIGH);
}

void motorGOTO(int x, int y){ //inputs in CM
  motorHome();
  xNema.set(CLOCKWISE, RPM, PULSE);
  yNema.set(CLOCKWISE, RPM, PULSE);
  x=(x/(5*3.1415926535/10))/200;
  y=(y/(5*3.1415926535/10))/200;
  for (size_t i = 0; i < x; i++)
  {
    xNema.run();
  }
  for (size_t i = 0; i < y; i++)
  {
    yNema.run();
  }
}

void roboPulse(){
  
}

void scaleRead(){
  if (scale.is_ready()) {
    long reading = scale.read();
    Serial.print("HX711 reading: ");
    Serial.println(reading);
  } 
  else{
    Serial.println("HX711 not found.");
  }
}
