// include libs
#include <ModbusMaster.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include  <ESP8266WiFi.h>

#define MAX485_DE      2
#define MAX485_RE_NEG  2

 
const char* ssid     ="1111"; // Tu wpisz nazwę swojego wifi
const char* password = "1111"; // Tu wpisz hasło do swojego wifi

// instantiate ModbusMaster object
ModbusMaster node;
ModbusMaster node1;
WiFiServer server(80);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);    // Set the LCD I2C address

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

// variable
uint8_t result;
uint8_t result1;

int slave10[2];
int slave11[2];
int slave10_[2];
int slave11_[2];
int timeout10;
int timeout11;

void setup() {   
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  pinMode(12, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  // init vars
  slave10[0] = 91000;
  slave10[1] = 91001;
  slave11[0] = 91100;
  slave11[1] = 91101;
  slave10_[0] = slave10[0];
  slave10_[1] = slave10[1];
  slave11_[0] = slave11[0];
  slave11_[1] = slave11[1];
  timeout10 = 0;
  timeout11 = 0;
  
  // use Serial (port 0); initialize Modbus communication baud rate
  Serial.begin(9600);
  lcd.begin(16,2); 

  // communicate with Modbus slave ID 10 over Serial (port 0)
  node.begin(10, Serial);
  node1.begin(11, Serial);

  node.ku16MBResponseTimeout = 100;
  node1.ku16MBResponseTimeout = 100;

  WiFi.begin(ssid, password);
  //while (WiFi.status() != WL_CONNECTED) {
  //delay(500);
 // }
  server.begin();

   // Callbacks allow to configure the RS485 transceiver correctly
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  node1.preTransmission(preTransmission);
  node1.postTransmission(postTransmission);

  lcd.noBacklight();
  delay(500);
  lcd.backlight();
}

void loop() {
    if(slave10[0] > 25 || slave11[0] > 25)
    {
      tone(12, 1000);
    }
    else 
    {
      noTone(12);
    }
  
    result = node.readHoldingRegisters(0, 2);
    if (result == node.ku8MBSuccess)
    {
        slave10[0] = node.getResponseBuffer(0);
        slave10[1] = node.getResponseBuffer(1);
        timeout10 = 0;
    }
    else if (result == node.ku8MBResponseTimedOut)
    {
        timeout10++;
    }

    delay(10);//musi tu byc maly delay, nie wiem dlaczego
   
    result1 = node1.readHoldingRegisters(0, 2);
    if (result1 == node1.ku8MBSuccess)
    {
        slave11[0] = node1.getResponseBuffer(0);
        slave11[1] = node1.getResponseBuffer(1);
        timeout11 = 0;
    }
    else if (result1 == node1.ku8MBResponseTimedOut)
    {
        timeout11++;
    }

    lcd.setCursor(0,0);
    lcd.print("s10 ");
    lcd.print("T");
    lcd.print(slave10[0]);
    lcd.print(" W");
    lcd.print(slave10[1]);
     //lcd.print(123);
    lcd.setCursor(0,1);
    lcd.print("s11 ");
    lcd.print("T");
    lcd.print(slave11[0]);
    lcd.print(" W");
    lcd.print(slave11[1]);
    //lcd.print(456);

    delay(50);
    
    if(slave10[0] != slave10_[0] || slave10[1] != slave10_[1] || slave11[0] != slave11_[0] || slave11[1] != slave11_[1])
    {
      lcd.setCursor(11,0);
      lcd.print("          ");
      lcd.setCursor(11,1);
      lcd.print("          ");
      slave10_[0] = slave10[0];
      slave10_[1] = slave10[1];
      slave11_[0] = slave11[0];
      slave11_[1] = slave11[1];
    }

    WiFiClient client = server.available();
    if (!client) {
      return;
    }

    int timewate = 0;
    while(!client.available()){
      delay(1);
      timewate = timewate +1;
        if(timewate>1800)
        {
          client.stop();
          return;
        }
      }

    client.print("<font size=50px>");
     client.print("s10 Temperatura: ");
     client.print(slave10[0]);
     client.print("</br>");
     client.print("s10 Wilgotnosc: ");
     client.print(slave10[1]);
     client.print("</br>");
     client.print("s11 Temperatura: ");
     client.print(slave11[0]);
     client.print("</br>");
     client.print("s11 Wilgotnosc: ");
     client.print(slave11[1]);
     client.print("</br>");
     client.print("Timeout s10: ");
     if(timeout10 > 10)
      client.print("TAK");
     else
      client.print("NIE");
     client.print("</br>");
     client.print("Timeout s11: ");
     if(timeout11 > 10)
      client.print("TAK");
     else
      client.print("NIE");
     client.print("</br>");
    client.print("</font>");

    
    
}

