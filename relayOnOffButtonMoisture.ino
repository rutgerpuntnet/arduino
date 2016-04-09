#include <Average.h>

#include <DHT.h>

const int RELAY_PIN = 8;                  // Connect Digital Pin 8 on Arduino to CH1 on Relay Module
const int SWITCH_PIN = 2;                 // the number of the pushbutton pin
const int DHT_PIN = 4;                // pin number for the DHT moist sensor
const int DHT_TYPE = DHT22;         // type of humid sensor
const int POT_PIN = 2;                  // Connect analog Pin 2 on Arduino to potentiometer

const int LOOP_DELAY = 10;                // milliseconds
const long MILLIS_ON_KEYPRESS = 10L * 60L * 1000L;        // number of milliseconds to turn on the relay on 1 keypress
const int MILLIS_TO_CANCEL = 3 * 1000;        // (milli)seconds to hold button before cancelling
const long MILLIS_INTERVAL_HUMIDITY_CHECK = 30L * 1000L; // (milli)seconds interval to do a humidity check
const int HUMIDITY_ARRAY_SIZE = 3;
const float HUMIDITY_ON_TRIGGER = 85;
const float HUMIDITY_OFF_TRIGGER = 75;

// variables will change:
int switchState = 0;         // variable for reading the pushbutton status
int previousSwitchState = 0; // state of the switch/pushbutton in the previous loop
long relayLoopCountDown = 0L;          // countdown for the relay to be open (will be lower each loop until 0)
long pressedMilliseconds = 0L;    // number of milliseconds the button is currently pressed (if pressed)
long humidityCountDown = 5 * 1000L; // wait a few seconds on start
boolean humiditySwitchOn = false;   // boolean to help canceling the relay loop countdown
DHT dht(DHT_PIN, DHT_TYPE); // DHT object representing the humidity sensor
Average<float> humidities(HUMIDITY_ARRAY_SIZE); // Rolling array object giving an average for the humidity values

 void setup(){
   Serial.begin(9600);
   //Setup all the Arduino Pins
   pinMode(RELAY_PIN, OUTPUT);
   pinMode(SWITCH_PIN, INPUT);
   digitalWrite(SWITCH_PIN, HIGH); // connect to internal pull up resistor
      
   //Turn OFF any power to the Relay channels
   digitalWrite(RELAY_PIN,HIGH);
   dht.begin();
   delay(1000); //Wait 1 seconds before starting sequence
   Serial.println("Setup complete!");
 }
 
 void loop(){
  switchState = digitalRead(SWITCH_PIN);
  boolean skipDelay = checkHumidity(); // we'll skip the delay since we already had some time reading the value
  
  if (switchState == LOW) {  // button pressed    
   if (previousSwitchState != switchState) {
    previousSwitchState = switchState;
    
    relayLoopCountDown = relayLoopCountDown + (MILLIS_ON_KEYPRESS / LOOP_DELAY);
    Serial.print("Button pressed. New relayLoopCountDown = ");
    Serial.print(relayLoopCountDown);
    Serial.print(" loop. Or in minutes: ");
    Serial.println((float)((relayLoopCountDown * LOOP_DELAY) / 1000)/60);
   } else { // pressed and still pressed?
    pressedMilliseconds = pressedMilliseconds + LOOP_DELAY;
    if (pressedMilliseconds > MILLIS_TO_CANCEL) {
      if (relayLoopCountDown > 0) {
        Serial.println("Canceling!");
        relayLoopCountDown = 0L;
      }
    }
   }
  } else { // button not pressed
    pressedMilliseconds = 0;
   if (previousSwitchState != switchState) {
    previousSwitchState = switchState;
   }
  }

  determineRelayStatus();

  if(!skipDelay) {
    delay(LOOP_DELAY);
  }
 }

 
 boolean checkHumidity(){
  if (humidityCountDown < 1) {
    humidityCountDown = MILLIS_INTERVAL_HUMIDITY_CHECK;
    float humidity = dht.readHumidity(); // read the humidity (takes approx 270 millis)
    if (isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      humidities.rolling(humidity);
      Serial.print("Current humidity = ");
      Serial.print(humidity);
      Serial.print(" / Average humidity = ");
      Serial.println(humidities.mean());
    }
    return true;
  }
  
  humidityCountDown = humidityCountDown - LOOP_DELAY;
  return false;
 }
 
 /*
  * Determine the status that we should give the relay
  * based on the countDown (or air moisture)
  */
 void determineRelayStatus(){
    int humidityOffset = 10 - (analogRead(POT_PIN) / 50);
    
    if (!humiditySwitchOn && humidities.mean() > (HUMIDITY_ON_TRIGGER + humidityOffset)) {
      Serial.print("Switch on based on humidity value ");
      Serial.println(HUMIDITY_ON_TRIGGER + humidityOffset);
      humiditySwitchOn = true;
    }
    if (humiditySwitchOn && humidities.mean() < (HUMIDITY_OFF_TRIGGER + humidityOffset)) {
      Serial.print("Switch off based on humidity");
      Serial.println(HUMIDITY_OFF_TRIGGER + humidityOffset);
      humiditySwitchOn = false;
    }
  
    if (relayLoopCountDown > 0 || humiditySwitchOn) {
     digitalWrite(RELAY_PIN, LOW); // switch relay on
     if (relayLoopCountDown > 0) {
      relayLoopCountDown--;
     }
    } else {
     digitalWrite(RELAY_PIN, HIGH); // switch relay off
  }
 }


