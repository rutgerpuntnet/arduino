/*
Arduino sketch that reads a webparameter used to set a timer on a relay and replies with json data
 */

#include <OneWire.h>
#include <SPI.h>
#include <Ethernet.h>
int pos = 0;

const int RELAY_PIN_1 = 8;                  // Connect Digital Pin 8 on Arduino to CH1 on Relay Module
const int RELAY_PIN_2 = 9;                  // Connect Digital Pin 9 on Arduino to CH2 on Relay Module
const int TEMP_PIN = 4;                  // pin for temperature sensor
const int RESISTANCE_ANALOG_PIN_1 = 0;          // analog pin for resistance measure 1
const int RESISTANCE_ANALOG_PIN_2 = 1;          // analog pin for resistance measure 2

OneWire ds(TEMP_PIN);  // on pin 4 (a 4.7K resistor is necessary)

unsigned long relay1Timer; // the timer for relay 1
long RELAY_1_INTERVAL = 0; // the repeat interval
unsigned long tempTimer; // the timer for temp check
long TEMP_INTERVAL = 1000L * 60L; // the repeat interval for the temp (1 minute)

int latestTemp;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   //physical mac address
byte ip[] = { 192, 168, 1, 12 };                      // ip in lan (that's what you need to use in your browser. ("192.168.1.178")
byte gateway[] = { 192, 168, 1, 1 };                   // internet access via router
byte subnet[] = { 255, 255, 255, 0 };                  //subnet mask
EthernetServer server(80);                             //server port

String readString(100);
String finalstring = String(100);

void setup() {
  // Open serial communications
  Serial.begin(9600);
  pinMode(RELAY_PIN_1, OUTPUT);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  relay1Timer = millis(); // start timer
  tempTimer = millis();
  readTemperature();
}


void loop() {
  long webresponse = webServerLoop();

  if (webresponse > 0) { // if we have a parameter response from the webserver, turn on relay and set the timer
    // start timer for #webresponse minutes and turn relay ON
    RELAY_1_INTERVAL = 1000L * 60L * webresponse;
    relay1Timer = millis(); // start timer
    Serial.print("Timer set to: ");
    Serial.println(relay1Timer);

    digitalWrite(RELAY_PIN_1, LOW); // toggle relay ON (= relay pin low)
  }

  //read temp every minute (takes at least 1 second)
  if (TEMP_INTERVAL != 0 && (millis() - tempTimer) > TEMP_INTERVAL) {
    tempTimer = millis();
    readTemperature();
  }

  // TIMER block
  if (RELAY_1_INTERVAL != 0 && (millis() - relay1Timer) > RELAY_1_INTERVAL) {
    Serial.print("Timeout! Millis: ");
    Serial.println(millis());

    // timed out
    RELAY_1_INTERVAL = 0;// reset timer by setting interval to zero

    // toggle relay OFF
    digitalWrite(RELAY_PIN_1, HIGH); // turn relay off
  }

}

long webServerLoop() {
  long result = -1;
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //read char by char HTTP request
        if (readString.length() < 100) {
          //store characters to string
          readString += c;
        }

        //if HTTP request has ended
        if (c == '\n') {
          Serial.print("ReadString from webserver: ");
          Serial.println(readString);

          // Create (HTML) response
          send_header(client);
          client.print("[");
          client.print("{ \"relay1Interval\": ");
          client.print(RELAY_1_INTERVAL);
          client.print("},");

          client.print("{ \"relay1TimerValue\": ");
          client.print(relay1Timer);
          client.print("},");

          client.print("{ \"millis\": ");
          client.print(millis());
          client.print("},");

          client.print("{ \"relay1TimeSpent\": ");
          client.print((millis() - relay1Timer));
          client.print("},");

          client.print("{ \"temp\": ");
          client.print(latestTemp);
          client.print("},");

          client.print("{ \"humid1\": ");
          client.print(getFirstHumidityIndex());
          client.print("},");

          client.print("{ \"humid2\": ");
          client.print(getSecondHumidityIndex());
          client.print("},");

          client.print("]");

          delay(1);
          //stopping client
          client.stop();

          //GET /?setTimer=10 HTTP/1.1

          int index = readString.indexOf("?setTimer=");
          if (index > 0) {
            index += 10; // add length of teststring
            Serial.print("Index of setTimer=");

            Serial.println(index);

            int index2 = index + 1;
            if (readString.charAt(index2 + 1) == ' ') {
              Serial.println("double number");
              index2++;
            }
            finalstring = readString.substring(index, index + 2);
            Serial.print("finalstring: ");
            Serial.println(finalstring);
            result = finalstring.toInt();
          }


          //clearing string for next read
          readString = "";

          Serial.print("returning: ");
          Serial.println(result);
        }
      } else {
        result = 0;
      }
    }
  }
  return result;
}

void readTemperature() {

  byte data[2];
  ds.reset(); 
  ds.write(0xCC);
  ds.write(0x44);
  delay(750);
  ds.reset();
  ds.write(0xCC);
  ds.write(0xBE);
  data[0] = ds.read(); 
  data[1] = ds.read();
  int Temp = (data[1]<<8)+data[0];
  Temp = Temp>>4;
  latestTemp = Temp;
}

int getFirstHumidityIndex() {
  return (10 - (analogRead(RESISTANCE_ANALOG_PIN_1) / 50));
}

int getSecondHumidityIndex() {
  return (10 - (analogRead(RESISTANCE_ANALOG_PIN_2) / 50));
}

static void send_header (EthernetClient client) {
  client.println("HTTP/1.1 200 OK"); //send new page
  client.println("Content-Type: text/html; charset=UTF-8\nContent-Language: en");
  client.println();
}
