#include<mwc_stepper.h>
#include "HX711.h"
#include <SPI.h>
#include "printf.h"
#include "RF24.h"

//DC Pins
#define ENCA 18 //Yellow
#define ENCB 19 //white
#define IN1 5
#define IN2 6
int DCticks=0;

//Voltage divider
#define VPin A0

//rf pins
#define CE_PIN 53
#define CSN_PIN 3
RF24 radio(CE_PIN, CSN_PIN);
uint8_t address[][6] = { "1Node", "2Node" };
bool radioNumber = 1;
bool role = false;
uint8_t payload[6];

//eStop
#define eStop_pin 39

//hx 711 pins
const int LOADCELL_DOUT_PIN = 11;
const int LOADCELL_SCK_PIN = 12;
HX711 scale;

//Stepp pins
#define EN_PIN1 29
#define DIR_PIN1 31
#define STEP_PIN1 9
#define EN_PIN2 28
#define DIR_PIN2 30
#define STEP_PIN2 10
#define RPM 40
#define PULSE 1600
#define CLOCKWISE 1
#define OTHERWISE 0
MWCSTEPPER xNema(EN_PIN1, DIR_PIN1, STEP_PIN1);
MWCSTEPPER yNema(EN_PIN2, DIR_PIN2, STEP_PIN2);
int currX=0;
int currY=0;
int xin=0;
int yin=0;

//Pulse
unsigned long currMillis=0;
unsigned long prevMillis=0;
const unsigned long HBPeriod=5000;


void readEncoder(){
  int b = digitalRead(ENCB);
  if(b>0){
    DCticks++;
  }
  else{
    DCticks--;
  }
}

void retractDC(){
  while(DCticks>0){
    setMotor(1,IN1,IN2);
    Serial.println(DCticks);
  }
  setMotor(0,IN1,IN2);
}

void distDC(uint16_t DCDist){
  while(DCticks<int(DCDist)){
    setMotor(-1,IN1,IN2);
    Serial.println(DCticks);
  }
  setMotor(0,IN1,IN2);
}

void distRetractDC(uint16_t DCDist){
  DCticks=0;
  while(abs(DCticks)<int(DCDist)){
    setMotor(1,IN1,IN2);
    Serial.println(DCticks);
  }
  DCticks=0;
  setMotor(0,IN1,IN2);
  Serial.println(DCticks);
}

void parsePayload(){
  if (payload[0]==255 && payload[5]==238){
    switch (payload[1]){
      case 1:
        motorHome();
        break;
      case 2:
        motorHalf();
        break;
      case 3:
        retractDC();
        break;
      case 4:
        eStop();
        break;
      case 11:
        xin=payload[2];
        yin=payload[3];
        motorGOTO();
        break;
      case 13:
        uint16_t distin=(payload[2])|(payload[3]<<8);
        Serial.print(distin);
        distRetractDC(distin);
        break;
    }
    switch(payload[1]){
      case 12:
        uint16_t distin=(payload[2])|(payload[3]<<8);
        Serial.print(distin);
        distDC(distin);
        break;
    }
  }
}

void motorHome(){
  xNema.set(OTHERWISE, RPM, PULSE);
  yNema.set(OTHERWISE, RPM, PULSE);
  while (currX>0){
    xNema.run();
    yNema.run();
    currX--;
  }
  xNema.set(CLOCKWISE, RPM, PULSE);
  while (currY>0)
  {
    xNema.run();
    yNema.run();
    currY--;
  }
}

void motorHalf(){
  //4456 Ticks full extend
  xNema.set(CLOCKWISE, RPM, PULSE);
  yNema.set(CLOCKWISE, RPM, PULSE);
  if (currX<=450){
    for (size_t i = 0+currX; i < 450; i++)
    {
      xNema.run();
      yNema.run();
    }
  }
  else{
    xNema.set(OTHERWISE, RPM, PULSE);
    yNema.set(OTHERWISE, RPM, PULSE);
    for (size_t i = 0; i < currX-450; i++)
    {
      xNema.run();
      yNema.run();
    }
  }
  if (currY<=625){
    xNema.set(OTHERWISE, RPM, PULSE);
    yNema.set(CLOCKWISE, RPM, PULSE);
    for (size_t i = 0+currY; i < 625; i++)
    {
      xNema.run();
      yNema.run();
    }
  }
  else{
    xNema.set(CLOCKWISE, RPM, PULSE);
    yNema.set(OTHERWISE, RPM, PULSE);
    for (size_t i = 0; i < currY-625; i++)
    {
      xNema.run();
      yNema.run();
    }
  }
  currX=450;
  currY=625;
}

void eStop(){
  digitalWrite(eStop_pin,HIGH);
}

void motorGOTO(){ //Inputs Converted to Steps
  xNema.set(CLOCKWISE, RPM, PULSE);
  yNema.set(CLOCKWISE, RPM, PULSE);
  int xp=map(xin,0,255,0,900);
  int yp=map(yin,0,255,0,1250);
  if (currX<=xp){
    for (size_t i = 0+currX; i < xp; i++)
    {
      xNema.run();
      yNema.run();
    }
  }
  else{
    xNema.set(OTHERWISE, RPM, PULSE);
    yNema.set(OTHERWISE, RPM, PULSE);
    for (size_t i = 0; i < currX-xp; i++)
    {
      xNema.run();
      yNema.run();
    }
  }
  if (currY<=yp){
    xNema.set(OTHERWISE, RPM, PULSE);
    yNema.set(CLOCKWISE, RPM, PULSE);
    for (size_t i = 0+currY; i < yp; i++)
    {
      xNema.run();
      yNema.run();
    }
  }
  else{
    xNema.set(CLOCKWISE, RPM, PULSE);
    yNema.set(OTHERWISE, RPM, PULSE);
    for (size_t i = 0; i < currY-yp; i++)
    {
      xNema.run();
      yNema.run();
    }
  }
  currX=xp;
  currY=yp;
}

void setMotor(int dir, int in1, int in2){
  if(dir==1){
    digitalWrite(in1,HIGH);
    digitalWrite(in2,LOW);
  }
  else if(dir==-1){
    digitalWrite(in1,LOW);
    digitalWrite(in2,HIGH);
  }
  else{
    digitalWrite(in1,HIGH);
    digitalWrite(in2,HIGH);
  }
}

float scaleRead(){
  if (scale.is_ready()) {
    long reading = scale.read();
    float weight=abs((float(reading)+333163)/232208*1.1);
    return weight;
  } 
  else{
    Serial.println("HX711 not found.");
  }
}

void roboPulse(){
//include voltage and scale
  float weight=scaleRead(); 
//  First number is before the decimal place, second is after. Second is 0-99.
  uint8_t weight1=uint8_t(weight);
  uint8_t weight2=uint8_t(weight*100-weight1*100);
  float voltage=(map(analogRead(VPin),0,1023,3.3,5)/4.2)*13.3;
  uint8_t volt1=uint8_t(voltage);
  uint8_t volt2=uint8_t(voltage*100-volt1*100);
  uint8_t HBpayload[7]={255,weight1,weight2,volt1,volt2,0,238};
  radio.stopListening();
  radio.setPayloadSize(sizeof(HBpayload));
  int chkSUM=0;
  for(int i=0; i<sizeof(HBpayload); i++){
    if (i!=5){
     chkSUM=chkSUM+int(HBpayload[i]);
    }
  }
  HBpayload[5]=chkSUM % 256;
  bool report=radio.write(&HBpayload, sizeof(HBpayload));
  if (report) {
    Serial.println("HB Pos");
  }
  else {
    Serial.println("HB Neg");
  }

  //reset
  radio.setPayloadSize(sizeof(payload));
  radio.startListening();
}

void setup() {
  Serial.begin(115200);

  //RF 
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(payload));
  radio.openWritingPipe(address[radioNumber]);
  radio.openReadingPipe(1, address[!radioNumber]);  
  radio.startListening();

  //HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  //NEMA
  xNema.init();
  yNema.init();

  //DC 
  pinMode(ENCA,INPUT);
  pinMode(ENCB,INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCA),readEncoder,RISING);

  //Timer
  prevMillis=millis();

  //EStop
  pinMode(eStop_pin,OUTPUT);
  digitalWrite(eStop_pin,LOW);
}

void loop() {
  currMillis=millis();
  if (currMillis-prevMillis >= HBPeriod){
    roboPulse();
    prevMillis=currMillis;
  }
  uint8_t pipe;
  if (radio.available(&pipe))
  {
     uint8_t bytes = radio.getPayloadSize();
     radio.read(&payload, bytes);
     for(int i=0; i<sizeof(payload); i++){
       Serial.println(payload[i]);
     }
     int chkSUM=0;
     for(int i=0; i<sizeof(payload); i++){
       if (i!=4){
        chkSUM=chkSUM+int(payload[i]);
       }
     }
     chkSUM=chkSUM % 256;
     if(payload[4]==chkSUM){
       parsePayload();
     }
     else{
      Serial.print("Bad checksum");
     }
  }
}
