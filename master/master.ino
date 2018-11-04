///////////////////////////////////////////////////////////////////
// include the library code:
#include <ModbusMaster.h>

#define TXRX 2

void preTransmission()
{
  digitalWrite(TXRX, 1);
}

void postTransmission()
{
  digitalWrite(TXRX, 0);
}

// instantiate ModbusMaster object
ModbusMaster node1;
ModbusMaster node2;

// variable
int slave1[2] = {0, 0};
int slave2[2] = {0, 0};

void setup() {   
  pinMode(TXRX, OUTPUT);
  // Init in receive mode
  digitalWrite(TXRX, 0);
  
  // use Serial (port 0); initialize Modbus communication baud rate
  Serial.begin(9600);

  // communicate with Modbus slave ID 1 over Serial (port 0)
  node1.begin(1, Serial);
  node1.ku16MBResponseTimeout = 100;
  // Callbacks allow us to configure the RS485 transceiver correctly
  node1.preTransmission(preTransmission);
  node1.postTransmission(postTransmission);
  
  node2.begin(2, Serial);
  node2.ku16MBResponseTimeout = 100;
  node2.preTransmission(preTransmission);
  node2.postTransmission(postTransmission);
}

void loop() {
    node1.result = node1.readHoldingRegisters(0, 2);
    if(node1.result == node1.ku8MBSuccess)
    {
        slave1[0] = node1.getResponseBuffer(0);
        slave1[1] = node1.getResponseBuffer(1);
        node1.responseTimeoutCount = 0;
    }
    else if(node1.result == node1.ku8MBResponseTimedOut)
    {
        node1.responseTimeoutCount++;
    }
    delay(10);//musi tu byc maly delay, nie wiem dlaczego


   
    node2.result = node2.readHoldingRegisters(0, 2);
    if(node2.result == node2.ku8MBSuccess)
    {
        slave2[0] = node2.getResponseBuffer(0);
        slave2[1] = node2.getResponseBuffer(1);
        node2.responseTimeoutCount = 0;
    }
    else if(node2.result == node2.ku8MBResponseTimedOut)
    {
        node2.responseTimeoutCount++;
    }
    delay(10);
}

