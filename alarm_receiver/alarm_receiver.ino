#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

bool alarm = false;
int val = 0;;
void setup() {
  Serial.begin(9600);
  mySwitch.enableReceive(0);
  pinMode(10, OUTPUT);
}

void loop() {
  if (mySwitch.available()) {    
    val = mySwitch.getReceivedValue();
    if(val == 101)
      alarm = true;  
    else if(val == 100)
      alarm = false;   

    mySwitch.resetAvailable();
  }

  Serial.println(val);

  if(alarm == true){
    digitalWrite(10, LOW); 
    Serial.println("alarm");
  } 
  else{
    Serial.println("OK");
    digitalWrite(10, HIGH);  
  } 

}
