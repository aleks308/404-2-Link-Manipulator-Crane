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

//end stop pins
#define xStop 20
#define yStop 21

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
const unsigned long HBPeriod=500;
uint8_t HBpayload[7]={255,12,6,1,23,238,238};

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
  attachInterrupt(digitalPinToInterrupt(xStop),setx0,RISING);
  attachInterrupt(digitalPinToInterrupt(yStop),sety0,RISING);

  //DC 
  pinMode(ENCA,INPUT);
  pinMode(ENCB,INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCA),readEncoder,RISING);

  //Timer
  prevMillis=millis();
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
      case 12:
        uint16_t distin=(payload[2])|(payload[3]<<8);
        //Serial.print(distin);
        Serial.print(distin);
        distDC(distin);
        break;
      case 13
        uint16_t distin=(payload[2])|(payload[3]<<8);
        //Serial.print(distin);
        Serial.print(distin);
        distRetractDC(distin);
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
  if (currX<2228){
    for (size_t i = 0+currX; i < 2228; i++)
    {
      xNema.run();
    }
  }
  else{
    for (size_t i = 0; i < 2228-currX; i++)
    {
      xNema.set(OTHERWISE, RPM, PULSE);
      xNema.run();
    }
  }
  if (currY<2228){
    for (size_t i = 0+currY; i < 2228; i++)
    {
      yNema.run();
    }
  }
  else{
    for (size_t i = 0; i < 2228-currY; i++)
    {
      yNema.set(OTHERWISE, RPM, PULSE);
      yNema.run();
    }
  }
  currX=2228;
  currY=2228;
}

void retractDC(){
  while(DCticks>0){
    setMotor(1,IN1,IN2);
  }
  setMotor(0,IN1,IN2);
}

void eStop(){
  digitalWrite(eStop_pin, HIGH);
}

void motorGOTO(){ //Inputs Converted to Steps
  xNema.set(CLOCKWISE, RPM, PULSE);
  yNema.set(CLOCKWISE, RPM, PULSE);
  int xp=map(xin,0,255,0,2550);
  int yp=map(yin,0,255,0,2550);
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

void distDC(uint16_t DCDist){
  while(DCticks<int(DCDist)){
    setMotor(-1,IN1,IN2);
  }
  setMotor(0,IN1,IN2);
}

void distRetractDC(uint16_t DCDist){
  while(DCticks>int(DCDist)){
    setMotor(1,IN1,IN2);
  }
  setMotor(0,IN1,IN2);
}

void roboPulse(){
//include voltage and scale
  radio.stopListening();
  radio.setPayloadSize(sizeof(HBpayload));
  float weight=scaleRead(); 
//  First number is before the decimal place, second is after. Second is 0-99.
  uint8_t weight1=uint8_t(weight);
  uint8_t weight2=uint8_t(weight*100-weight1*100);
  float voltage=(VPin)/4.8*13  
  uint8_t volt1=uint8_t(voltage);
  uint8_t volt2=uint8_t(voltage*100-volt1*100);
  HBpayload[7]={255,weight1,weight2,volt1,volt2,0,238];
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

void scaleRead(){
  if (scale.is_ready()) {
    long reading = scale.read();
    float weight=abs((float(reading)+333163)/232208*1.1);
    return weight;
  } 
  else{
    Serial.println("HX711 not found.");
  }
}

void readEncoder(){
  int b = digitalRead(ENCB);
  if(b>0){
    DCticks++;
  }
  else{
    DCticks--;
  }
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
    digitalWrite(in1,LOW);
    digitalWrite(in2,LOW);
  }
}

void setx0(){
  currX=0;
  Serial.print("x");
}

void sety0(){
  currY=0;
  Serial.print("y");
}
