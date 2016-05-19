/*
Arduino sketch that reads a webparameter used to set a timer on a relay and replies with json data
 */
#include <Average.h>
#include <OneWire.h>
#include <SPI.h>
#include <Ethernet.h>
int pos = 0;

const int RELAY_PIN_1 = 8;                  // Connect Digital Pin 8 on Arduino to CH1 on Relay Module
const int RELAY_PIN_2 = 9;                  // Connect Digital Pin 9 on Arduino to CH2 on Relay Module
const int TEMP_PIN = 4;                  // pin for temperature sensor
const int RESISTANCE_ANALOG_PIN_1 = 0;          // analog pin for resistance measure 1
const int RESISTANCE_ANALOG_PIN_2 = 1;          // analog pin for resistance measure 2
const int HUMIDITY_ARRAY_SIZE = 5;

OneWire ds(TEMP_PIN);  // on pin 4 (a 4.7K resistor is necessary)

unsigned long relay1Timer; // the timer for relay 1
long RELAY_1_INTERVAL = 0; // the repeat interval
unsigned long tempTimer; // the timer for temp check
long TEMP_INTERVAL = 1000L * 60L; // the repeat interval for the temp (1 minute)
unsigned long humidTimer; // the timer for humid check
long HUMID_INTERVAL = 1000L * 10L; // the repeat interval for the temp (10 secs)

int latestTemp;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   //physical mac address
byte ip[] = { 192, 168, 1, 12 };                      // ip in lan (that's what you need to use in your browser. ("192.168.1.178")
byte gateway[] = { 192, 168, 1, 1 };                   // internet access via router
byte subnet[] = { 255, 255, 255, 0 };                  //subnet mask
EthernetServer server(80);                             //server port

Average<int> humidities1(HUMIDITY_ARRAY_SIZE); // Rolling array object giving an average for the humidity values
Average<int> humidities2(HUMIDITY_ARRAY_SIZE); // Rolling array object giving an average for the humidity values

String readString(100);

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
  humidTimer = millis();
  
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
  } else if (webresponse == -1) {
    // force turn off
    RELAY_1_INTERVAL = 0L;
    Serial.println("Turned off relay timer");
  }

  //read temp every minute (takes at least 1 second) and the humidity
  if (TEMP_INTERVAL != 0 && (millis() - tempTimer) > TEMP_INTERVAL) {
    Serial.print("Reading temperature");
    tempTimer = millis();
    readTemperature();
  }

  //read humid every 10 secs
  if (HUMID_INTERVAL != 0 && (millis() - humidTimer) > HUMID_INTERVAL) {
    Serial.print("Reading humidity");
    if (millis() % 2) // Measure 1 or 2, not both, random (since analogread needs a delay before reading another pin)
      int humid1 = analogRead(RESISTANCE_ANALOG_PIN_1);
      humidities1.rolling(humid1);
    } else {
      int humid2 = analogRead(RESISTANCE_ANALOG_PIN_2);
      humidities2.rolling(humid2);
    }
  }

  // TIMER block
  if (RELAY_1_INTERVAL != 0 && (millis() - relay1Timer) > RELAY_1_INTERVAL) {
    Serial.print("Timeout relay 1! At millis: ");
    Serial.println(millis());

    // timed out
    RELAY_1_INTERVAL = 0;// reset timer by setting interval to zero

    // toggle relay OFF
    digitalWrite(RELAY_PIN_1, HIGH); // turn relay off
  }

}

long webServerLoop() {
  long result = -9999;
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


          client.print("{ \"relay1SwitchedOn\": ");
          client.print(RELAY_1_INTERVAL > 0);
          client.print("},");

          client.print("{ \"relay1TimerValue\": ");
          client.print(relay1Timer);
          client.print("},");

          client.print("{ \"millis\": ");
          client.print(millis());
          client.print("},");

          client.print("{ \"relayTimeSpentSeconds\": ");
          if (RELAY_1_INTERVAL == 0) {
            client.print(0);
          } else {
            client.print((millis() - relay1Timer)/1000);
          }
          client.print("},");

          client.print("{ \"relayTimeLeftSeconds\": ");
          if (RELAY_1_INTERVAL == 0) {
            client.print(0);
          } else {
            client.print((RELAY_1_INTERVAL - (millis() - relay1Timer))/1000);          
          }
          client.print("},");

          client.print("{ \"temp\": ");
          client.print(latestTemp);
          client.print("},");

          client.print("{ \"tempTimer\": ");
          client.print(tempTimer);
          client.print("},");

          client.print("{ \"tempInterval\": ");
          client.print(TEMP_INTERVAL);
          client.print("},");

          client.print("{ \"humid1\": ");
          client.print((10 - (humidities1.mean() / 50));
          client.print("},");

          client.print("{ \"humid2\": ");
          client.print((10 - (humidities2.mean() / 50));
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
            String finalstring = readString.substring(index, index + 2);
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

void readTemperatureOLD() {

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
  int temp = (data[1]<<8)+data[0];
  Serial.print("tempvalue read: ");
  Serial.println(temp);
  temp = temp>>4;
  latestTemp = temp;
  Serial.print("latestTemp: ");
  Serial.println(latestTemp);
}


void readTemperature() {
    byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  latestTemp = celsius;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
}

static void send_header (EthernetClient client) {
  client.println("HTTP/1.1 200 OK"); //send new page
  client.println("Content-Type: text/html; charset=UTF-8\nContent-Language: en");
  client.println();
}
