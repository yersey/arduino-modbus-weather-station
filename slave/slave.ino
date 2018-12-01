#include <ModbusRtu.h>
#include "DHT.h"
#include <SFE_BMP180.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define ALTITUDE 193.0
#define SLAVE_ID 1
#define BUTTON_PIN 7
#define RS_PIN 5
#define DS_PIN 10
#define DHT11_PIN 3
#define SUN_PIN A1
#define TEMP_REG 0
#define HUM_REG 1
#define PRES_REG 2
#define SUN_REG 3

uint16_t au16data[9] = {
  111, 1, 2, 3, 2018, 11, 16, 12, 56};

Modbus slave(SLAVE_ID, 0, RS_PIN);
DHT dht;
SFE_BMP180 pressure;
OneWire oneWire(DS_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress ds18b20 = { 0x28, 0x90, 0x63, 0x45, 0x92, 0x06, 0x02, 0xE0 };
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

int humidity = 0;
unsigned long getHumTime = 0;
int pressure_ = 0;
int temperature = 0;
unsigned long getTempTime = 0;
unsigned long lastUpdate = 0;
unsigned long buttonTime = 0;
int buttonHoldTime = 0;
byte sun = 0;
bool page = 0;
bool lcdLight = true;

void setup() {
  dht.setup(DHT11_PIN);
  //Serial.begin( 9600 );
  pressure.begin();
  sensors.begin();
  slave.begin( 9600 ); // baud-rate at 9600
  lcd.begin(16,2); 
  pinMode(BUTTON_PIN, INPUT);
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
    au16data[TEMP_REG] = temperature;

  sun = readSun();
  if(sun != NULL)
    au16data[SUN_REG] = sun;
    
  slave.poll(au16data, 16);
  
  if(page == 0){
    if(millis()-lastUpdate >= 1500UL)
    {
      lcd.clear();
      printTime();
      lcd.setCursor(0, 1);
      printPage0();
      lastUpdate = millis();
    }
  }

  else if(page == 1){
    if(millis()-lastUpdate >= 1500UL)
    {
      lcd.clear();
      printTime();
      lcd.setCursor(0, 1);
      printPage1();
      lastUpdate = millis();
    }
  }
  
  if(digitalRead(BUTTON_PIN) == HIGH){
    if(buttonTime == 0) 
      buttonTime = millis();
      
    buttonHoldTime += millis()-buttonTime;
    buttonTime = millis();
  }
  else{
    if(buttonHoldTime > 1000){
      if(lcdLight == true)
        lcdLight = false;
      else lcdLight = true;
    }

    else if(buttonHoldTime < 500 && buttonHoldTime > 50)
      if(page == 0)
        page = 1;
      else page = 0;
      
    buttonHoldTime = 0;  
    buttonTime = 0;
  }
//////////////////////////////////////
}

int readHumidity(){
  if(dht.getMinimumSamplingPeriod() <= (millis()-getHumTime) ){
    getHumTime = millis();
    return dht.getHumidity();
  }
}

int readPressure(){
  
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

int readTemp(){
  if(millis()-getTempTime >= 1000UL){
    sensors.requestTemperatures();
    getTempTime = millis();
    return sensors.getTempC(ds18b20)*10;
  }
  else return temperature;
}

byte readSun(){
  return map(analogRead(SUN_PIN), 0, 1023, 0, 100);
}

void printTime(){
    lcd.print(au16data[7]);
    lcd.print(":");
    lcd.print(au16data[8]);
    lcd.print(" ");
    lcd.print(au16data[6]);
    lcd.print(".");
    lcd.print(au16data[5]);
    lcd.print(".");
    lcd.print(au16data[4]);
}

void printPage0(){
    lcd.print(" T:");
    lcd.print(au16data[TEMP_REG]/10);
    lcd.print(".");
    lcd.print(au16data[TEMP_REG]-au16data[TEMP_REG]/10*10);
    lcd.print("*C");
    lcd.print(" ");
    lcd.print("W:");
    lcd.print(au16data[HUM_REG]);
    lcd.print("%");
}

void printPage1(){
    lcd.print(au16data[PRES_REG]);
    lcd.print("hPa");
    lcd.print(" ");
    lcd.print("sun:");
    lcd.print(au16data[SUN_REG]);
    lcd.print("%");  
}
