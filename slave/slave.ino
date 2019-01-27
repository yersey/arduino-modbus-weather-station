#include <ModbusRtu.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SLAVE_ID 3
#define BUTTON_PIN 7
#define RS_PIN 5
#define DS_PIN 10
#define SUN_PIN A1
#define TEMP_REG 0
#define SUN_REG 1

uint16_t au16data[9] = {
  9999, 9999, 0, 3, 2018, 11, 16, 8, 7};

Modbus slave(SLAVE_ID, 0, RS_PIN);
OneWire oneWire(DS_PIN);
DallasTemperature sensors(&oneWire);
//DeviceAddress ds18b20 = { 0x28, 0x90, 0x63, 0x45, 0x92, 0x06, 0x02, 0xE0 }; //1
//DeviceAddress ds18b20 = { 0x28, 0x68, 0xA6, 0x45, 0x92, 0x03, 0x02, 0x3C }; //2
DeviceAddress ds18b20 = { 0x28, 0x12, 0xC6, 0x45, 0x92, 0x08, 0x02, 0x59 }; //3
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

int temperature = 9999;
unsigned long getTempTime = 0;
unsigned long lastUpdate = 0;
unsigned long buttonTime = 0;
int buttonHoldTime = 0;
byte sun = 0;
bool lcdLight = true;

void setup() {
  sensors.begin();
  slave.begin( 9600 ); // baud-rate at 9600
  lcd.begin(16,2); 
  pinMode(BUTTON_PIN, INPUT);
}

void loop() {
  temperature = readTemp(); 
  if(temperature != NULL){
    if(temperature < 0){
      au16data[2] = 1;
      au16data[TEMP_REG] = temperature*(-1);
    } else if(temperature == 0){
        au16data[2] = 0;
        au16data[TEMP_REG] = 0;
    }else {
      au16data[2] = 0;
      au16data[TEMP_REG] = temperature;
    }
  }

  sun = readSun();
  if(sun != NULL)
    au16data[SUN_REG] = sun;
    
  slave.poll(au16data, 16);
  
    if(millis()-lastUpdate >= 1500UL)
    {
      lcd.clear();
      printTime();
      lcd.setCursor(0, 1);
      printPage0();
      lastUpdate = millis();
    }
    
  if(digitalRead(BUTTON_PIN) == HIGH){
    if(buttonTime == 0) 
      buttonTime = millis();
      
    buttonHoldTime += millis()-buttonTime;
    buttonTime = millis();
  }
  else{
    if(buttonHoldTime > 750){
      if(lcdLight == true)
        lcdLight = false;
      else lcdLight = true;
    }

    else if(buttonHoldTime < 500 && buttonHoldTime > 50)
      ;
      
    buttonHoldTime = 0;  
    buttonTime = 0;
  }

  if(lcdLight == true) lcd.backlight();
  else lcd.noBacklight();
//////////////////////////////////////
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
    if(au16data[7] < 10)
      lcd.print(0);
    lcd.print(au16data[7]);
    lcd.print(":");
    if(au16data[8] < 10)
      lcd.print(0);
    lcd.print(au16data[8]);
    lcd.print(" ");
    lcd.print(au16data[6]);
    lcd.print(".");
    lcd.print(au16data[5]);
    lcd.print(".");
    lcd.print(au16data[4]);
}

void printPage0(){
    lcd.print("T:");
    if(au16data[2] > 0)
      lcd.print("-");
    lcd.print(au16data[TEMP_REG]/10);
    lcd.print(".");
    lcd.print(au16data[TEMP_REG]-au16data[TEMP_REG]/10*10);
    lcd.print("*C");
    lcd.print(" ");
    lcd.print("SUN:");
    lcd.print(au16data[SUN_REG]);
    lcd.print("%");
}
