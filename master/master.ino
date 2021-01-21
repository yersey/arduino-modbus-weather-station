#include <Arduino.h>
#include <U8g2lib.h>
#include <ModbusMaster.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <OnewireKeypad.h>
#include <SPI.h>
#include <SD.h>
#include <RCSwitch.h>

#include "private.h"

#define TXRX D4
#define SDSELECT D3
#define RF_PIN D8
#define KP_PIN A0

File dataFile;

RCSwitch mySwitch = RCSwitch();

bool sdState = false;

int timezone = 1 * 3600;
int dst = 0;
unsigned long tgUpdate = 0;

bool down = false;
bool up = false;
bool pick = false;
bool back = false;
bool inputMode = false;

char key = '-';
String input = "";

int frame = 0;
int pos = 1;
int x = 0;
int inputPos = 1;

bool loadResult = false;
bool toSave = false;

uint16_t slave1[2] = {9999, 9999};
uint16_t slave2[2] = {9999, 9999};
uint16_t slave3[2] = {9999, 9999};
uint16_t slave4[2] = {9999, 9999};

String request = "";

bool syrena = false;

int master_alarm = 0;

bool s1_timeout = false;
float s1_minTemp = 0;
int s1_alarm = 0;

bool s2_timeout = false;
float s2_minTemp = 0;
int s2_alarm = 0;

bool s3_timeout = false;
float s3_minTemp = 0;
int s3_alarm = 0;

bool s4_timeout = false;
float s4_minTemp = 0;
int s4_alarm = 0;

char KEYS[] = {
  '1', '4', '7', '*',
  '2', '5', '8', '0',
  '3', '6', '9', '#',
  'A', '.', '.', 'D'
};


U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, D0, D2, D1);
OnewireKeypad <Print, 16 > KP2(Serial, KEYS, 4, 4, KP_PIN, 2500, 600 );
WiFiClient  client;

ModbusMaster node1;
ModbusMaster node2;
ModbusMaster node3;
ModbusMaster node4;

#define c_width 6
#define c_height 10
static unsigned char c_bits[] = {
   0x02, 0x05, 0x02, 0x18, 0x24, 0x04, 0x04, 0x24, 0x18, 0x00 };

#define sun_width 15
#define sun_height 10
static unsigned char sun_bits[] = {
   0x80, 0x00, 0x84, 0x10, 0x08, 0x08, 0xc0, 0x01, 0xe0, 0x03, 0xe6, 0x33,
   0xe0, 0x03, 0xc8, 0x09, 0x04, 0x10, 0x80, 0x00 };

#define sd_width 15
#define sd_height 10
static unsigned char sd_bits[] = {
   0xc0, 0x07, 0x60, 0x05, 0x50, 0x05, 0x10, 0x04, 0x10, 0x04, 0x10, 0x04,
   0x10, 0x04, 0x10, 0x04, 0x10, 0x04, 0xf0, 0x07 };

#define wifi_width 15
#define wifi_height 10
static unsigned char wifi_bits[] = {
   0x00, 0x00, 0xe0, 0x01, 0x38, 0x07, 0x04, 0x08, 0xf0, 0x03, 0x08, 0x04,
   0xe0, 0x01, 0x00, 0x00, 0xc0, 0x00, 0xc0, 0x00 };

#define alarm_width 10
#define alarm_height 7
static unsigned char alarm_bits[] = {
   0x90, 0x00, 0x18, 0x01, 0x5e, 0x01, 0x5e, 0x01, 0x5e, 0x01, 0x18, 0x01,
   0x90, 0x00 };

#define tick_width 10
#define tick_height 7
static unsigned char tick_bits[] = {
   0x00, 0x03, 0x80, 0x03, 0xc0, 0x01, 0xe3, 0x00, 0x77, 0x00, 0x3e, 0x00,
   0x1c, 0x00 };

#define cross_width 10
#define cross_height 7
static unsigned char cross_bits[] = {
   0x86, 0x03, 0xce, 0x01, 0xfc, 0x00, 0x70, 0x00, 0xf8, 0x00, 0xdc, 0x01,
   0x8e, 0x03 };

void preTransmission()
{
  digitalWrite(TXRX, 1);
}

void postTransmission()
{
  digitalWrite(TXRX, 0);
}

bool ifDown(){
  if(down == true)  
  {
    down = 0;
    return true;
  }
  else return false;
}

bool ifUp(){
  if(up == true)  
  {
    up = 0;
    return true;
  }
  else return false;
}

bool ifPick(){
  if(pick == true)  
  {
    pick = 0;
    return true;
  }
  else return false;
}

bool ifBack(){
  if(back == true)  
  {
    inputMode = 0;
    input = "";
    back = 0;
    return true;
  }
  else return false;
}

void clearPick(){
    pick = 0;
}

void clearBack(){
    back = 0;
}

void goFrame(int f){
  pos = 1;
  frame = f;  
}

int convertStrToInt(String s){
  int len = s.length();
  int result = 0;
  
  for(int i = 0; i < len; i++)
    if(s[i] != '.')
      result = result * 10 + ( s[i] - '0' );
    
  return result;
}

void loadSettings(){
  if(sdState){
    dataFile = SD.open("settings.txt");
    if (dataFile) {
      dataFile.seek(0);
      
      dataFile.find("alarm");
      dataFile.seek(dataFile.position()+1);
      master_alarm = convertStrToInt(dataFile.readStringUntil(';'));
  
      dataFile.find("s1 alarm");
      dataFile.seek(dataFile.position()+1);
      s1_alarm = convertStrToInt(dataFile.readStringUntil(';'));
  
      dataFile.find("s1 min. temp.");
      dataFile.seek(dataFile.position()+1);
      s1_minTemp = dataFile.readStringUntil(';').toFloat();

      dataFile.find("s2 alarm");
      dataFile.seek(dataFile.position()+1);
      s2_alarm = convertStrToInt(dataFile.readStringUntil(';'));
  
      dataFile.find("s2 min. temp.");
      dataFile.seek(dataFile.position()+1);
      s2_minTemp = dataFile.readStringUntil(';').toFloat();
      
      dataFile.find("s3 alarm");
      dataFile.seek(dataFile.position()+1);
      s3_alarm = convertStrToInt(dataFile.readStringUntil(';'));
  
      dataFile.find("s3 min. temp.");
      dataFile.seek(dataFile.position()+1);
      s3_minTemp = dataFile.readStringUntil(';').toFloat();

      dataFile.find("s4 alarm");
      dataFile.seek(dataFile.position()+1);
      s4_alarm = convertStrToInt(dataFile.readStringUntil(';'));
  
      dataFile.find("s4 min. temp.");
      dataFile.seek(dataFile.position()+1);
      s4_minTemp = dataFile.readStringUntil(';').toFloat();
      
      dataFile.close();
      loadResult = true;
    }
  }
  else loadResult = false;
}

void SaveSettings(int alarm, int s11, float s12,int s21, float s22,int s31, float s32, int s41, float s42){
  if(sdState){
    dataFile = SD.open("settings.txt", FILE_WRITE);  
    if (dataFile) {
      dataFile.seek(0);
      
      dataFile.find("alarm");
      dataFile.seek(dataFile.position()+1);
      dataFile.print(alarm);
      dataFile.print(";");
  
      dataFile.find("s1 alarm");
      dataFile.seek(dataFile.position()+1);
      dataFile.print(s11);
      dataFile.print(";");
  
      dataFile.find("s1 min. temp.");
      dataFile.seek(dataFile.position()+1);
      dataFile.print(s12);
      dataFile.print(";");
      
      dataFile.find("s2 alarm");
      dataFile.seek(dataFile.position()+1);
      dataFile.print(s21);
      dataFile.print(";");
  
      dataFile.find("s2 min. temp.");
      dataFile.seek(dataFile.position()+1);
      dataFile.print(s22);
      dataFile.print(";");

      dataFile.find("s3 alarm");
      dataFile.seek(dataFile.position()+1);
      dataFile.print(s31);
      dataFile.print(";");
  
      dataFile.find("s3 min. temp.");
      dataFile.seek(dataFile.position()+1);
      dataFile.print(s32);
      dataFile.print(";");

      dataFile.find("s4 alarm");
      dataFile.seek(dataFile.position()+1);
      dataFile.print(s41);
      dataFile.print(";");
  
      dataFile.find("s4 min. temp.");
      dataFile.seek(dataFile.position()+1);
      dataFile.print(s42);
      dataFile.print(";");
    }
  } 
}

void setup(void) {
  
  KP2.SetDebounceTime(25);
  
  WiFi.config(ip, dns, gateway, subnet);
  WiFi.begin(ssid,password);
  
  pinMode(SDSELECT, OUTPUT);
  sdState = SD.begin(SDSELECT);
  loadSettings();

  configTime(timezone, dst, "pool.ntp.org","time.nist.gov");

  u8g2.begin();
  u8g2.setDrawColor(2);
  u8g2.setFontMode(1);
  
  mySwitch.enableTransmit(RF_PIN);
  mySwitch.setRepeatTransmit(3);
  
  pinMode(TXRX, OUTPUT);
  digitalWrite(TXRX, 0);
  Serial.begin(9600);
  
  node1.begin(1, Serial);
  node1.ku16MBResponseTimeout = 30;
  node1.preTransmission(preTransmission);
  node1.postTransmission(postTransmission);

  node2.begin(2, Serial);
  node2.ku16MBResponseTimeout = 30;
  node2.preTransmission(preTransmission);
  node2.postTransmission(postTransmission);

  node3.begin(3, Serial);
  node3.ku16MBResponseTimeout = 30;
  node3.preTransmission(preTransmission);
  node3.postTransmission(postTransmission);

  node4.begin(4, Serial);
  node4.ku16MBResponseTimeout = 30;
  node4.preTransmission(preTransmission);
  node4.postTransmission(postTransmission);
}

void loop(void) {  
  if(WiFi.status() == WL_CONNECTION_LOST)
    WiFi.begin(ssid,password);

  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  
   if(master_alarm > 0){
    if((float((int16_t)slave1[0]/10) < s1_minTemp && (s1_alarm > 0)) || (s1_timeout == true && (s1_alarm > 0)) )
      syrena = true;
    else if((float((int16_t)slave2[0]/10) < s2_minTemp && (s2_alarm > 0)) || (s2_timeout == true && (s2_alarm > 0)) )
      syrena = true;
    else if((float((int16_t)slave3[0]/10) < s3_minTemp && (s3_alarm > 0)) || (s3_timeout == true && (s3_alarm > 0)) )
      syrena = true;
    else if((float((int16_t)slave4[0]/10) < s4_minTemp && (s4_alarm > 0)) || (s4_timeout == true && (s4_alarm > 0)) )
      syrena = true;
    else syrena = false;
  }else syrena = false;

    if(syrena == true)
      mySwitch.send(101, 24);
    else
      mySwitch.send(100, 24);

  node1.result = node1.readHoldingRegisters(0, 2);
    if(node1.result == node1.ku8MBSuccess)
    {
        slave1[0] = node1.getResponseBuffer(0);
        slave1[1] = node1.getResponseBuffer(1);
        node1.responseTimeoutCount = 0;
        
        node1.writeSingleRegister(4, p_tm->tm_year + 1900);
        node1.writeSingleRegister(5, p_tm->tm_mon + 1);
        node1.writeSingleRegister(6, p_tm->tm_mday);
        node1.writeSingleRegister(7, p_tm->tm_hour);
        node1.writeSingleRegister(8, p_tm->tm_min);
    }
    else if(node1.result == node1.ku8MBResponseTimedOut)
    {  
      node1.responseTimeoutCount++;
    }
    if(node1.responseTimeoutCount > 10)
      s1_timeout = true;
    else s1_timeout = false;
    delay(10);//musi tu byc maly delay, nie wiem dlaczego

      node2.result = node2.readHoldingRegisters(0, 2);
    if(node2.result == node2.ku8MBSuccess)
    {
        slave2[0] = node2.getResponseBuffer(0);
        slave2[1] = node2.getResponseBuffer(1);
        node2.responseTimeoutCount = 0;
        
        node2.writeSingleRegister(4, p_tm->tm_year + 1900);
        node2.writeSingleRegister(5, p_tm->tm_mon + 1);
        node2.writeSingleRegister(6, p_tm->tm_mday);
        node2.writeSingleRegister(7, p_tm->tm_hour);
        node2.writeSingleRegister(8, p_tm->tm_min);
    }
    else if(node2.result == node2.ku8MBResponseTimedOut)
    {  
      node2.responseTimeoutCount++;
    }
    if(node2.responseTimeoutCount > 10)
      s2_timeout = true;
    else s2_timeout = false;
    delay(10);//musi tu byc maly delay, nie wiem dlaczego

      node3.result = node3.readHoldingRegisters(0, 2);
    if(node3.result == node3.ku8MBSuccess)
    {
        slave3[0] = node3.getResponseBuffer(0);
        slave3[1] = node3.getResponseBuffer(1);
        node3.responseTimeoutCount = 0;
        
        node3.writeSingleRegister(4, p_tm->tm_year + 1900);
        node3.writeSingleRegister(5, p_tm->tm_mon + 1);
        node3.writeSingleRegister(6, p_tm->tm_mday);
        node3.writeSingleRegister(7, p_tm->tm_hour);
        node3.writeSingleRegister(8, p_tm->tm_min);
    }
    else if(node3.result == node3.ku8MBResponseTimedOut)
    {  
      node3.responseTimeoutCount++;
    }
    if(node3.responseTimeoutCount > 10)
      s3_timeout = true;
    else s3_timeout = false;
    delay(10);//musi tu byc maly delay, nie wiem dlaczego

    node4.result = node4.readHoldingRegisters(0, 2);
    if(node4.result == node4.ku8MBSuccess)
    {
        slave4[0] = node4.getResponseBuffer(0);
        slave4[1] = node4.getResponseBuffer(1);
        node4.responseTimeoutCount = 0;
        
        node4.writeSingleRegister(4, p_tm->tm_year + 1900);
        node4.writeSingleRegister(5, p_tm->tm_mon + 1);
        node4.writeSingleRegister(6, p_tm->tm_mday);
        node4.writeSingleRegister(7, p_tm->tm_hour);
        node4.writeSingleRegister(8, p_tm->tm_min);
    }
    else if(node4.result == node4.ku8MBResponseTimedOut)
    {  
      node4.responseTimeoutCount++;
    }
    if(node4.responseTimeoutCount > 10)
      s4_timeout = true;
    else s4_timeout = false;
    delay(10);//musi tu byc maly delay, nie wiem dlaczego
    
    
  if(KP2.Key_State() == PRESSED){
  key = KP2.Getkey();
    
    switch(key){
      case 'D':
        down = true;
      break;

      case 'A':
        up = true;
      break;
      
      case '*':
        pick = true;
      break;
     
      case '#':
        back = true;
      break;
    }

    if(inputMode == true){
      if(key != '*' && key != '#' && key != '-' && key != 'A' && key != 'D')
        input += key;
    }
  }

  if(ifDown() == true){
    pos++;  
  }

  if(ifUp() == true){
    pos--;  
  }

  u8g2.clearBuffer(); 

  if(frame == 0){
    clearBack();
    
    if(ifPick() == true){
      if(pos == 1) 
        goFrame(1);
      else if(pos == 2)
        goFrame(2);
      else if(pos == 3)
        goFrame(3);
    }
       
    if(pos > 3) 
      pos = 1;
    if(pos < 1)
      pos = 3;
    if(pos == 1)
      x = 21 ;
    else if(pos == 2)
      x = 31;
    else if(pos == 3)
      x = 42; 

    if(sdState)
      u8g2.drawXBMP( 114, 0, sd_width, sd_height, sd_bits);
   
    if(WiFi.status() == WL_CONNECTED)
      u8g2.drawXBMP( 100, 0, wifi_width, wifi_height, wifi_bits);
      
    u8g2.setFont(u8g_font_6x10);
        
    u8g2.drawBox(100,x,28,11);
      
    u8g2.setCursor(102, 30);
    u8g2.print("Ust");
    
    u8g2.setCursor(102, 40);
    u8g2.print("Pog");
    
    u8g2.setCursor(102, 50);
    u8g2.print("Inf");
  
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g_font_6x10);
    u8g2.setCursor(0, 7);
    if(p_tm->tm_hour <10) u8g2.print(0);
    u8g2.print(p_tm->tm_hour);
    u8g2.print(":");
    if(p_tm->tm_min <10) u8g2.print(0);
    u8g2.print(p_tm->tm_min);
    u8g2.print(" ");
    u8g2.print(p_tm->tm_mday);
    u8g2.print(".");
    u8g2.print(p_tm->tm_mon + 1);
    u8g2.print(".");
    u8g2.print(p_tm->tm_year + 1900);  
    u8g2.setCursor(82, 63);
    u8g2.print(p_tm->tm_sec);
    u8g2.print(" ");
    if(syrena == true)
      u8g2.print("ALARM");
       
    u8g2.drawHLine(0, 21, 75);
    u8g2.drawHLine(0, 32, 75);
    u8g2.drawHLine(0, 43, 75);
    u8g2.drawHLine(0, 54, 75);
    u8g2.drawVLine(14, 12, 50);
    u8g2.drawVLine(42, 12, 50);
    u8g2.drawVLine(60, 12, 50);
  
    u8g2.drawXBMP( 24, 10, c_width, c_height, c_bits);
    u8g2.drawXBMP( 44, 9, sun_width, sun_height, sun_bits);
    u8g2.drawXBMP( 64, 12, alarm_width, alarm_height, alarm_bits);
       
    u8g2.drawStr( 0, 30, "S1");
    u8g2.drawStr( 0, 41, "S2");
    u8g2.drawStr( 0, 52, "S3");
    u8g2.drawStr( 0, 63, "S4");
    
    u8g2.setCursor( 17,30 );
    u8g2.print(float((int16_t)slave1[0])/10, 1);
    u8g2.setCursor( 45,30 );
    u8g2.print(slave1[1]);
    if(s1_alarm > 0 && master_alarm > 0)
      u8g2.drawXBMP( 63, 23, tick_width, tick_height, tick_bits);
    else u8g2.drawXBMP( 63, 23, cross_width, cross_height, cross_bits);
    if(s1_timeout == true)
      u8g2.drawHLine(0, 26, 75);   

    u8g2.setCursor( 17,41 );
    u8g2.print(float((int16_t)slave2[0])/10, 1);
    u8g2.setCursor( 45,41 );
    u8g2.print(slave2[1]);
    if(s2_alarm > 0 && master_alarm > 0)
      u8g2.drawXBMP( 63, 34, tick_width, tick_height, tick_bits);
    else u8g2.drawXBMP( 63, 34, cross_width, cross_height, cross_bits);
    if(s2_timeout == true)
      u8g2.drawHLine(0, 37, 75);

    u8g2.setCursor( 17,52 );
    u8g2.print(float((int16_t)slave3[0])/10, 1);
    u8g2.setCursor( 45,52 );
    u8g2.print(slave3[1]);
    if(s3_alarm > 0 && master_alarm > 0)
      u8g2.drawXBMP( 63, 45, tick_width, tick_height, tick_bits);
    else u8g2.drawXBMP( 63, 45, cross_width, cross_height, cross_bits);
    if(s3_timeout == true)
      u8g2.drawHLine(0, 48, 75); 

    u8g2.setCursor( 17,63 );
    u8g2.print(float((int16_t)slave4[0])/10, 1);
    u8g2.setCursor( 45,63 );
    u8g2.print(slave4[1]);
    if(s4_alarm > 0 && master_alarm > 0)
      u8g2.drawXBMP( 63, 56, tick_width, tick_height, tick_bits);
    else u8g2.drawXBMP( 63, 56, cross_width, cross_height, cross_bits);
    if(s4_timeout == true)
      u8g2.drawHLine(0, 59, 75); 
      
      
      
    u8g2.setDrawColor(2);
  }   

  if(frame == 1){

  if(ifBack() == true)
      goFrame(0);

  if(ifPick() == true){
    if(pos == 1) 
      goFrame(11);
    else if(pos == 2)
      goFrame(12);
    else if(pos == 3)
      goFrame(13);
    else if(pos == 4)
      goFrame(14);
    else if(pos == 5)
      goFrame(15);
  }
     
    if(pos > 5) 
      pos = 1;/////////
    if(pos < 1)
      pos = 5;/////////
    if(pos == 1)
      x = 17 ;
    else if(pos == 2)
      x = 28;
    else if(pos == 3)
      x = 39;
    else if(pos == 4)
      x = 50;
    else if(pos == 5)
      x = 61;
      
      u8g2.clearBuffer(); 
      u8g2.setFont(u8g2_font_6x12_mr);

      u8g2.setCursor(10, 11);
      u8g2.print("    Ustawienia");
      u8g2.drawHLine(0, 13, 128);
      
      u8g2.drawBox(0,x,128,11);
    
      u8g2.setCursor(10, 26);
      u8g2.print("Alarm");
  
      u8g2.setCursor(10, 37);
      u8g2.print("S1");
  
      u8g2.setCursor(10, 48);
      u8g2.print("S2");

      u8g2.setCursor(10, 59);
      u8g2.print("S3");

      u8g2.setCursor(10, 70);
      u8g2.print("S4");
      
  }

        if(frame == 11){

          if(ifBack() == true)
            goFrame(1);
      
         if(ifPick() == true){
          if(inputMode == true){
            inputMode = false;
            pos = inputPos;
            
            if(pos == 1){
              master_alarm = convertStrToInt(input);   ///////////////////       ZAPIS       ///////////////////
              toSave = true;
            }
              
            input = "";
          }
          else{
            inputMode = true;
            inputPos = pos;
          }
        }
      
          if(inputMode == false){
            if(pos != 1) 
              pos = 1;/////////
            
            if(pos == 1)
              x = 16 ;
          }
          
            u8g2.clearBuffer(); 
            u8g2.setFont(u8g2_font_6x12_mr);
      
            if(inputMode == true && inputPos == 1){
                  u8g2.setCursor(120, 25);
                  u8g2.print("#");
                  u8g2.setCursor(90, 25);
                  u8g2.print(input);
              }
              else{
                u8g2.setCursor(90, 25);
                u8g2.print(master_alarm);
              }

            u8g2.setCursor(5, 11);
            u8g2.print("Alarm dla wszystkich urzadzen");
            u8g2.drawHLine(0, 13, 128);
            
            u8g2.drawBox(0,x,128,11);
          
            u8g2.setCursor(10, 25);
            u8g2.print("alarm wl/wyl");
        }
      
        if(frame == 12){
          
        if(ifBack() == true)
            goFrame(1);
            
        if(ifPick() == true){
          if(inputMode == true){
            inputMode = false;
            pos = inputPos;
            
            if(pos == 1){
              s1_alarm = convertStrToInt(input);   ///////////////////       ZAPIS       ///////////////////
              toSave = true;
            }
            if(pos == 2){
              s1_minTemp = input.toFloat();   ///////////////////       ZAPIS       ///////////////////
              toSave = true;
            }
              
            input = "";
          }
          else{
            inputMode = true;
            inputPos = pos;
          }
        }
      
          if(inputMode == false){
            if(pos > 2) 
            pos = 1;/////////
            if(pos < 1)
              pos = 2;/////////
            if(pos == 1)
              x = 16 ;
            else if(pos == 2)
              x = 27;
          }
          
            u8g2.clearBuffer(); 
            u8g2.setFont(u8g2_font_6x12_mr);
      
              if(inputMode == true && inputPos == 1){
                  u8g2.setCursor(120, 25);
                  u8g2.print("#");
                  u8g2.setCursor(70, 25);
                  u8g2.print(input);
              }
              else{
                u8g2.setCursor(70, 25);
                u8g2.print(s1_alarm);
              }
              if(inputMode == true && inputPos == 2){
                  u8g2.setCursor(120, 36);
                  u8g2.print("#");
                  u8g2.setCursor(70, 36);
                  u8g2.print(input);
              }
              else{
                u8g2.setCursor(70, 36);
                u8g2.print(s1_minTemp);
              }
      
            u8g2.setCursor(10, 11);
            u8g2.print("       S1");
            u8g2.drawHLine(0, 13, 128);
            
            u8g2.drawBox(0,x,128,11);
          
            u8g2.setCursor(10, 25);
            u8g2.print("alarm");
        
            u8g2.setCursor(10, 36);
            u8g2.print("min. temp");
        }
      
        if(frame == 13){
      
        if(ifBack() == true)
            goFrame(1);
      
         if(ifPick() == true){
          if(inputMode == true){
            inputMode = false;
            pos = inputPos;
            
            if(pos == 1){
              s2_alarm = convertStrToInt(input);   ///////////////////       ZAPIS       ///////////////////
              toSave = true;
            }
            if(pos == 2){
              s2_minTemp = input.toFloat();   ///////////////////       ZAPIS       ///////////////////
               toSave = true;
            }
              
            input = "";
          }
          else{
            inputMode = true;
            inputPos = pos;
          }
        }
      
          if(inputMode == false){
            if(pos > 2) 
            pos = 1;/////////
            if(pos < 1)
              pos = 2;/////////
            if(pos == 1)
              x = 16 ;
            else if(pos == 2)
              x = 27;
          }
          
            u8g2.clearBuffer(); 
            u8g2.setFont(u8g2_font_6x12_mr);
      
            if(inputMode == true && inputPos == 1){
                  u8g2.setCursor(120, 25);
                  u8g2.print("#");
                  u8g2.setCursor(70, 25);
                  u8g2.print(input);
              }
              else{
                u8g2.setCursor(70, 25);
                u8g2.print(s2_alarm);
              }
              if(inputMode == true && inputPos == 2){
                  u8g2.setCursor(120, 36);
                  u8g2.print("#");
                  u8g2.setCursor(70, 36);
                  u8g2.print(input);
              }
              else{
                u8g2.setCursor(70, 36);
                u8g2.print(s2_minTemp);
              }
      
            u8g2.setCursor(10, 11);
            u8g2.print("       S2");
            u8g2.drawHLine(0, 13, 128);
            
            u8g2.drawBox(0,x,128,11);
          
            u8g2.setCursor(10, 25);
            u8g2.print("alarm");
        
            u8g2.setCursor(10, 36);
            u8g2.print("min. temp");     
        }
      
        if(frame == 14){
      
        if(ifBack() == true)
            goFrame(1);
   
         if(ifPick() == true){
          if(inputMode == true){
            inputMode = false;
            pos = inputPos;
            
            if(pos == 1){
              s3_alarm = convertStrToInt(input);   ///////////////////       ZAPIS       ///////////////////
              toSave = true;
            }
            if(pos == 2){
              s3_minTemp = input.toFloat();   ///////////////////       ZAPIS       ///////////////////
              toSave = true;
            }
              
            input = "";
          }
          else{
            inputMode = true;
            inputPos = pos;
          }
        }
      
          if(inputMode == false){
            if(pos > 2) 
            pos = 1;/////////
            if(pos < 1)
              pos = 2;/////////
            if(pos == 1)
              x = 16 ;
            else if(pos == 2)
              x = 27;
          }
          
            u8g2.clearBuffer(); 
            u8g2.setFont(u8g2_font_6x12_mr);
      
            if(inputMode == true && inputPos == 1){
                  u8g2.setCursor(120, 25);
                  u8g2.print("#");
                  u8g2.setCursor(70, 25);
                  u8g2.print(input);
              }
              else{
                u8g2.setCursor(70, 25);
                u8g2.print(s3_alarm);
              }
              if(inputMode == true && inputPos == 2){
                  u8g2.setCursor(120, 36);
                  u8g2.print("#");
                  u8g2.setCursor(70, 36);
                  u8g2.print(input);
              }
              else{
                u8g2.setCursor(70, 36);
                u8g2.print(s3_minTemp);
              }
      
            u8g2.setCursor(10, 11);
            u8g2.print("       S3");
            u8g2.drawHLine(0, 13, 128);
            
            u8g2.drawBox(0,x,128,11);
          
            u8g2.setCursor(10, 25);
            u8g2.print("alarm");
        
            u8g2.setCursor(10, 36);
            u8g2.print("min. temp");
        }


        if(frame == 15){
          
        if(ifBack() == true)
            goFrame(1);
            
        if(ifPick() == true){
          if(inputMode == true){
            inputMode = false;
            pos = inputPos;
            
            if(pos == 1){
              s4_alarm = convertStrToInt(input);   ///////////////////       ZAPIS       ///////////////////
              toSave = true;
            }
            if(pos == 2){
              s4_minTemp = input.toFloat();   ///////////////////       ZAPIS       ///////////////////
              toSave = true;
            }
              
            input = "";
          }
          else{
            inputMode = true;
            inputPos = pos;
          }
        }
      
          if(inputMode == false){
            if(pos > 2) 
            pos = 1;/////////
            if(pos < 1)
              pos = 2;/////////
            if(pos == 1)
              x = 16 ;
            else if(pos == 2)
              x = 27;
          }
          
            u8g2.clearBuffer(); 
            u8g2.setFont(u8g2_font_6x12_mr);
      
              if(inputMode == true && inputPos == 1){
                  u8g2.setCursor(120, 25);
                  u8g2.print("#");
                  u8g2.setCursor(70, 25);
                  u8g2.print(input);
              }
              else{
                u8g2.setCursor(70, 25);
                u8g2.print(s4_alarm);
              }
              if(inputMode == true && inputPos == 2){
                  u8g2.setCursor(120, 36);
                  u8g2.print("#");
                  u8g2.setCursor(70, 36);
                  u8g2.print(input);
              }
              else{
                u8g2.setCursor(70, 36);
                u8g2.print(s4_minTemp);
              }
      
            u8g2.setCursor(10, 11);
            u8g2.print("       S4");
            u8g2.drawHLine(0, 13, 128);
            
            u8g2.drawBox(0,x,128,11);
          
            u8g2.setCursor(10, 25);
            u8g2.print("alarm");
        
            u8g2.setCursor(10, 36);
            u8g2.print("min. temp");
        }
        
  if(frame == 2){

  clearPick();
  if(ifBack() == true)
      goFrame(0);   
    
      u8g2.clearBuffer(); 
      u8g2.setFont(u8g2_font_helvB18_tf);
      u8g2.setCursor(10, 30);
      u8g2.print("Pogoda");
  }

  if(frame == 3){

  clearPick();
  if(ifBack() == true)
      goFrame(0);
      
      u8g2.clearBuffer(); 
      u8g2.setFont(u8g2_font_helvB18_tf);
      u8g2.setCursor(10, 30);
      u8g2.print("Informacje");
  }


  u8g2.setCursor(60, 64);
  u8g2.print(input);
     
u8g2.sendBuffer();

    if(millis()-tgUpdate >= 20000){
      if (client.connect("api.thingspeak.com", 80)) {
        request = "GET /update?api_key="KEY_;
        
        if((int16_t)slave1[0] < 1000 && s1_timeout == false){
          request += "&field1=" + String(float((int16_t)slave1[0])/10);
          request += "&field2=" + String(slave1[1]);
        }

        if((int16_t)slave2[0] < 1000 && s2_timeout == false){
          request += "&field3=" + String(float((int16_t)slave2[0])/10);
          request += "&field4=" + String(slave2[1]);
        }

        if((int16_t)slave3[0] < 1000 && s3_timeout == false){
        request += "&field5=" + String(float((int16_t)slave3[0])/10);
        request += "&field6=" + String(slave3[1]);
        }

        if((int16_t)slave4[0] < 1000 && s4_timeout == false){
        request += "&field7=" + String(float((int16_t)slave4[0])/10);
        request += "&field8=" + String(slave4[1]);
        }
        
        client.println(request);
        client.println();
      }
      tgUpdate = millis();
    }

  if(loadResult == true && toSave == true){
    SaveSettings(master_alarm, s1_alarm, s1_minTemp,s2_alarm, s2_minTemp, s3_alarm, s3_minTemp, s4_alarm, s4_minTemp);
  }
  
}




