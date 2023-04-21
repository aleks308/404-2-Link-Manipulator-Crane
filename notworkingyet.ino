#include <SPI.h>
#include "printf.h"
#include "RF24.h"

#define CE_PIN   9
#define CSN_PIN 10

// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);  
// Let these addresses be used for the pair
uint8_t address[][6] = { "1Node", "2Node" };
bool radioNumber = 0;
bool role = true;  // true = TX role, false = RX role

uint8_t header=255;
uint8_t data1=11;
uint8_t data2=23;
uint8_t data3=48;
uint8_t checksum=55;
uint8_t footer=44;
String Message;
uint8_t payload[6];
void setup() {
  Serial.begin(115200);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.
  
  radio.setPayloadSize(sizeof(payload)); 
  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);  // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]);  // using pipe 1
  radio.stopListening();
} 

void loop() {
//if (Serial.available()){
role =true;
Message=Serial.readStringUntil("\n");
for (int i=0; i=5; i++) {
  uint8_t a=i+(2*i);
  uint8_t b=i+2+(2*i);
  String escrow=Message.substring(a,b);
  payload[i]=atoi(escrow.c_str());
 
}

//}  
if (role){
radio.write(&payload, sizeof(payload));
delay(1000);
}
role=false;
}
