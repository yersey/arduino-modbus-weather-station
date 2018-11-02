#include <ModbusRtu.h>
#include "DHT.h"
#define DHT11_PIN 3

// registers in the slave
uint16_t au16data[16] = {
  63000, 63000, 63000, 7777, 2, 7182, 28182, 8, 0, 0, 0, 0, 0, 0, 1, -1 };

Modbus slave(10,0,5); // this is slave @1 and RS-232 or USB-FTDI
//10
DHT dht;

int lol = 0;
int wilgotnosc = 0;
int temperatura = 0;
int czas = millis();

void setup() {
  dht.setup(DHT11_PIN);
  slave.begin( 9600 ); // baud-rate at 9600
}

void loop() {

  wilgotnosc = dht.getHumidity();
  temperatura = dht.getTemperature();
  
  if (dht.getStatusString() == "OK" && dht.getMinimumSamplingPeriod()<=(millis()-czas)) 
  {
    czas = millis();
    au16data[0] = temperatura;
    au16data[1] = wilgotnosc;
  }
    
    slave.poll( au16data, 16 );
    //delay(200);
}
