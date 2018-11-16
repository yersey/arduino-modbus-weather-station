#include <ModbusRtu.h>
#include "DHT.h"
#include <SFE_BMP180.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ALTITUDE 193.0
#define SLAVE_ID 1
#define DS_PIN 10
#define DHT11_PIN 3
#define SUN_PIN A1
#define TEMP_REG 0
#define HUM_REG 1
#define PRES_REG 2
#define SUN_REG 3

uint16_t au16data[16] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

Modbus slave(SLAVE_ID, 0, 5);
DHT dht;
SFE_BMP180 pressure;
OneWire oneWire(DS_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress ds18b20 = { 0x28, 0x90, 0x63, 0x45, 0x92, 0x06, 0x02, 0xE0 };

int humidity = 0;
int getHumTime = 0;
float pressure_ = 0;
float temperature = 0;
int getTempTime = 0;
int sun = 0;

void setup() {
  dht.setup(DHT11_PIN);
  Serial.begin( 9600 );
  pressure.begin();
  sensors.begin();
  slave.begin( 9600 ); // baud-rate at 9600
}

void loop() {
  humidity = readHumidity();
  if(dht.getStatusString() == "OK")
    au16data[HUM_REG] = humidity;
   
  pressure_ = readPressure();
  if(pressure_ != NULL)
    au16data[PRES_REG] = pressure_;

  temperature = readTemp(); 
  if(temperature != NULL)
    au16data[TEMP_REG] = temperature*10;

  sun = readSun();
  if(sun != NULL)
    au16data[SUN_REG] = sun;

  Serial.print(au16data[HUM_REG]);
  Serial.print("  ");
  Serial.print(au16data[PRES_REG]);
  Serial.print("  ");
  Serial.print(au16data[TEMP_REG]);
  Serial.print("  ");
  Serial.println(au16data[SUN_REG]);

  //slave.poll( au16data, 16 );
}

int readHumidity(){
  if(dht.getMinimumSamplingPeriod() <= (millis()-getHumTime) ){
    getHumTime = millis();
    return dht.getHumidity();
  }
}

float readPressure(){
  
  char status;
  double T,P,p0,a;
  
  status = pressure.startTemperature();
  if (status != 0)
  {
    delay(status);
    status = pressure.getTemperature(T);
    if (status != 0)
    { 
      status = pressure.startPressure(3);
      if (status != 0)
      {
        delay(status);
        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          p0 = pressure.sealevel(P,ALTITUDE);       
          return p0;
        }
      }
    }
  }
}

float readTemp(){
  if(millis()-getTempTime >= 1000){
    sensors.requestTemperatures();
    getTempTime = millis();
    return sensors.getTempC(ds18b20);
  }
  else return temperature;
}

int readSun(){
  return map(analogRead(SUN_PIN), 0, 1023, 0, 100);
}
