#include <DHT.h>

const int RELAY_PIN = 8;                  // Connect Digital Pin 8 on Arduino to CH1 on Relay Module
const int SWITCH_PIN = 2;                 // the number of the pushbutton pin
const int DHT_PIN = 4;                // pin number for the DHT moist sensor
const int DHT_TYPE = DHT22;
const int LOOP_DELAY = 10;                // milliseconds
const int SECONDS_ON_KEYPRESS = 10;        // seconds to activate the relay on keypress
const int SECONDS_TO_CANCEL = 3;        // seconds to hold button before cancelling
const int MILLIS_INTERVAL_HUMIDITY_CHECK = 10 * 1000;

// variables will change:
int switchState = 0;         // variable for reading the pushbutton status
int previousSwitchState = 0;
int pushCount = 0;
int countDown = 0;
int pressedMilliseconds = 0;
int humidityCountDown = 5 * 1000; // wait a few seconds on start

DHT dht(DHT_PIN, DHT_TYPE);

 void setup(){
   Serial.begin(9600);
   //Setup all the Arduino Pins
   pinMode(RELAY_PIN, OUTPUT);
   pinMode(SWITCH_PIN, INPUT);
   digitalWrite(SWITCH_PIN, HIGH); // connect to internal pull up resistor
      
   //Turn OFF any power to the Relay channels
   digitalWrite(RELAY_PIN,HIGH);
   dht.begin();
   delay(2000); //Wait 2 seconds before starting sequence
   Serial.println("Setup complete!");
 }
 
 void loop(){
  switchState = digitalRead(SWITCH_PIN);
  checkHumidity();
  
  if (switchState == LOW) {  // button pressed    
   if (previousSwitchState != switchState) {
    previousSwitchState = switchState;
    pushCount=pushCount+1;
    countDown = countDown + (SECONDS_ON_KEYPRESS * 1000 / LOOP_DELAY); // X seconds
    Serial.print("pushCount=");
    Serial.println(pushCount);
    Serial.print("countDown=");
    Serial.println(countDown);
   } else { // pressed and still pressed?
    pressedMilliseconds = pressedMilliseconds + LOOP_DELAY;
    if (pressedMilliseconds > (SECONDS_TO_CANCEL * 1000)) {
      countDown = -1;
    }
   }
  } else { // button not pressed
    pressedMilliseconds = 0;
   if (previousSwitchState != switchState) {
    previousSwitchState = switchState;
   }
  }

  determineRelayStatus();
  
  delay(LOOP_DELAY);
 }

 
 void checkHumidity(){
  if (humidityCountDown < 1) {
    humidityCountDown = MILLIS_INTERVAL_HUMIDITY_CHECK;
    float h = dht.readHumidity();
    if (isnan(h)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      Serial.print("Humidity=");
      Serial.println(h); 
    }
  } else {
    humidityCountDown = humidityCountDown - LOOP_DELAY;
  }
 }
 
 /*
  * Determine the status that we should give the relay
  * based on the countDown (or air moisture)
  */
 void determineRelayStatus(){
    if (countDown > 0) {
    digitalWrite(RELAY_PIN, LOW);
    countDown = countDown - 1;
  } else {
    if (countDown < 0) {
      Serial.println("Cancelling!");
      countDown = 0;
    }
    digitalWrite(RELAY_PIN, HIGH);   
  }
 }

