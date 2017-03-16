#include <ESP8266WiFi.h>
#include "Arduino.h"
#include "Ultrasonic.h"

const int ceVacPin = 4;
Ultrasonic ultrasonic(12,13);

class comm {
  public:
  comm()
  {

  }
  
  const char* ssid     = "errans";
  const char* password = "zamb0rah";
  const char* host = "192.168.1.143";
  const int httpPort = 80;
  int failCount = 0;
  bool connected = 0;
  bool started = 0;
  bool doorState = 0;
  bool vacState = 0;
  int ledPin = 2;
  
  bool connect() {
    if (started == 0) {
      Serial.print("Connecting to ");
      Serial.println(ssid);
      WiFi.begin(ssid, password);
      started == 1;
      unsigned long connectMillis = millis();
       while (WiFi.status() != WL_CONNECTED) {
        if (connectMillis + 30000 <= millis()) {
          //we failed--reset and try again next time.
          WiFi.disconnect();
          connected = 0;
          return 1;
        };
        delay(500);
        Serial.print(".");
      };
      
      Serial.println("");
      Serial.println("WiFi connected");  
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connected = 1;
    } else {
      if (connected == 0) {
        unsigned long connectMillis = millis();
        WiFi.reconnect();
        while (WiFi.status() != WL_CONNECTED) {
          if (connectMillis + 30000 <= millis()) {
            //we failed--reset and try again next time.
            WiFi.disconnect();
            connected = 0;
            return 1;
          };
          delay(500);
          Serial.print(".");
        };
        
        Serial.println("");
        Serial.println("WiFi connected");  
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        connected = 1;
        return 0;
      };
    };
  };
  
  bool push() {
    int  rf = ultrasonic.Ranging(0);
    Serial.print("CC rangefinder: ");
    Serial.println(rf);
    if (rf <= 50) {
      if (rf <= 8) {
        doorState = 1;
      }
      else {
        doorState = 0;
      };
    }
    else {
      doorState = 0;
    };
    
    WiFiClient client;
    if (!client.connect(host, httpPort)) {
      failCount++;
      Serial.println(failCount);
      delay(1000);
      if (failCount == 5) {
        connected = 0;
        connect();
        failCount = 0;
        return 1;
      };
      return 1;
    };

    String url = "/api.php?device_api_pushing=true&vac=";
    url = url + vacState;
    url = url + "&door=";
    url = url + doorState;
    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
                 
    //delay(1000);
    bool available = 0;
    unsigned int timer = millis();
    while(available == 0) {
      if(client.available() || (millis() >= timer + 15000)) {
        available = 1;
        while(client.available()){
          failCount = 0;
          String line = client.readStringUntil('\r');
          Serial.println(line);
          Serial.println();
        };
      };
    };
  };
  
  bool pull() {
    WiFiClient client;
    if (!client.connect(host, httpPort)) {
      failCount++;
      Serial.println(failCount);
      delay(1000);
      if (failCount == 5) {
        connected = 0;
        connect();
        failCount = 0;
        return 1;
      };
      return 1;
    };
    String url = "/api.php?device_api_pulling=true";
    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" + 
             "Connection: close\r\n\r\n");

    bool available = 0;
    unsigned int timer = millis();
    while(available == 0) {
      if(client.available() || (millis() >= timer + 15000)) {
        available = 1;
        while(client.available()){
          failCount = 0;
          String line = client.readStringUntil('\r');
          //Serial.println(line);
          //Serial.println();
          if(line == "\nceVac") {
            Serial.println(line);
            if (vacState == 0) {
              digitalWrite(ceVacPin,HIGH);
              digitalWrite(ledPin,HIGH);
              vacState = 1;
            }
            else {
              digitalWrite(ceVacPin,LOW);
              digitalWrite(ledPin,LOW);
              vacState = 0;
            };
          };
        };
      };
    };
  };
  
};

comm cc = comm();

void setup() {
  Serial.begin(9600);
  delay(100);
  pinMode(ceVacPin,OUTPUT);
  digitalWrite(ceVacPin,LOW);

  cc.connect();
  
}

void loop() {
  cc.push();
  cc.pull();
}