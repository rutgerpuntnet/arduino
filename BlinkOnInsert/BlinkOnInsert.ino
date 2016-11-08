/* 
  IR Breakbeam sensor blinking led!
*/
 
const int LEDPIN = 13;
const int SENSORPIN = 4;
const int BLINKLED1 = 9;           // the PWM pin the LED is attached to
const int BLINKLED2 = 10;           // the PWM pin the second LED is attached to

int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by

// variables will change:
int sensorState = 0, lastState=0;         // variable for reading the ir status
boolean blinkingState = false;

void setup() {
  // initialize the LED pins as an output:
  pinMode(BLINKLED1, OUTPUT);
  pinMode(BLINKLED2, OUTPUT);
  pinMode(LEDPIN, OUTPUT);      
  // initialize the sensor pin as an input:
  pinMode(SENSORPIN, INPUT);

  digitalWrite(SENSORPIN, HIGH); // turn on the pullup
  
  Serial.begin(9600);
}
 
void loop(){
  if (blinkingState) {
    // fade-blink the leds, no need to check the IR leds here
      // set the brightness of the blinkleds:
    analogWrite(BLINKLED1, brightness);
    analogWrite(BLINKLED2, brightness);
  
    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;
  
    // reverse the direction of the fading at the ends of the fade:
    if (brightness >= 255) {
      Serial.println("blink fading down");
      fadeAmount = -fadeAmount;
    }
    if (brightness <= 0) {
      Serial.println("done blink");
      brightness = 0;
      blinkingState - false;
    }
    // wait for 30 milliseconds to see the dimming effect
    delay(30);

  } else {
    analogWrite(BLINKLED1, 0);
    analogWrite(BLINKLED2, 0);

    // read the state of the pushbutton value:
    sensorState = digitalRead(SENSORPIN);
   
    // check if the sensor beam is broken
    // if it is, the sensorState is LOW:
    if (sensorState == LOW) {     
      // turn LED on:
      digitalWrite(LEDPIN, HIGH);  
    } 
    else {
      // turn LED off:
      digitalWrite(LEDPIN, LOW); 
    }
    
    if (sensorState && !lastState) {
      Serial.println("Unbroken");
      blinkingState = true;
    } 
    if (!sensorState && lastState) {
      Serial.println("Broken");
    }
    lastState = sensorState;
  }

}
