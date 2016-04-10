/*
Arduino sketch that reads a webparameter used to set a timer on a relay
 */

    #include <SPI.h>
    #include <Ethernet.h>
    int pos = 0;

    const int RELAY_PIN = 8;                  // Connect Digital Pin 8 on Arduino to CH1 on Relay Module

    unsigned long timer; // the timer
    long INTERVAL = 0; // the repeat interval

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
        pinMode(RELAY_PIN, OUTPUT);

        // start the Ethernet connection and the server:
        Ethernet.begin(mac, ip, gateway, subnet);
        server.begin();
        Serial.print("server is at ");
        Serial.println(Ethernet.localIP());
        
        timer = millis(); // start timer
    }


    void loop() {
      long webresponse = webServerLoop();
      
      if (webresponse > 0) { // if we have a parameter response from the webserver, turn on relay and set the timer
        // start timer for #webresponse minutes and turn relay ON
        INTERVAL = 1000L * 60L * webresponse;
        timer = millis(); // start timer
        Serial.print("Timer set to: ");
        Serial.println(timer);

        digitalWrite(RELAY_PIN, LOW); // toggle relay ON (= relay pin low)
      }
      
      
  
  
      // TIMER block
      if (INTERVAL != 0 && (millis()-timer) > INTERVAL) {
        Serial.print("Timeout! Millis: ");
        Serial.println(millis());

        // timed out
        INTERVAL = 0;// reset timer by setting interval to zero

        // toggle relay OFF
        digitalWrite(RELAY_PIN, HIGH); // turn relay off
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
                        client.print("{ \"interval\": ");
                        client.print(INTERVAL);
                        client.print("},");
                          client.print("{ \"timer\": ");
                          client.print(timer);
                          client.print("},");
                          client.print("{ \"millis\": ");
                          client.print(millis());
                          client.print("},");
                          client.print("{ \"time\": ");
                          client.print((millis()-timer));
                          client.print("},");
                        client.print("]");

                        delay(1);
                        //stopping client
                        client.stop();

                        //GET /?setTimer=10 HTTP/1.1
                        
                        int index = readString.indexOf("?setTimer=");
                        if(index > 0) {
                          index += 10; // add length of teststring
                            Serial.print("Index of setTimer=");
                            
                            Serial.println(index);

                            int index2 = index+1;
                            if (readString.charAt(index2+1) == ' ') {
                              Serial.println("double number");
                              index2++;
                            }
                            finalstring = readString.substring(index, index+2);
                            Serial.print("finalstring: ");
                            Serial.println(finalstring);
                            result = finalstring.toInt();
                        }
                        

                        //clearing string for next read
                        readString="";
                        
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
    
    static void send_header (EthernetClient client) {
      client.println("HTTP/1.1 200 OK"); //send new page
      client.println("Content-Type: text/html; charset=UTF-8\nContent-Language: en");
      client.println();
    }
