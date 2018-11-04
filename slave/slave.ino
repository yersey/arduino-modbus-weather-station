#include <ModbusRtu.h>
#include "DHT.h"

#define SLAVE_ID 1
#define DHT11_PIN 3
#define HUM_REG 0

// registers in the slave
uint16_t au16data[16] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

Modbus slave(SLAVE_ID, 0, 5);
DHT dht;

int humidity = 0;
int getHumTime = millis();

void setup() {
  dht.setup(DHT11_PIN);
  slave.begin( 9600 ); // baud-rate at 9600
}

void loop() {

  if(dht.getMinimumSamplingPeriod() <= millis()-getHumTime){
    humidity = dht.getHumidity();
    getHumTime = millis();
  }

  if(dht.getStatusString() == "OK")
    au16data[HUM_REG] = humidity;

  slave.poll( au16data, 16 );
}
