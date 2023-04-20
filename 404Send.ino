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
int payload=0;
void setup() {
  Serial.begin(115200);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.
  
  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);  // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]);  // using pipe 1
  radio.stopListening();
} 

void loop() {
  payload = [0];
  radio.write(payload, sizeof(payload));
  
  delay(1000);
}
