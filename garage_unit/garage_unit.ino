#include <ESP8266WiFi.h>
#include "Arduino.h"
#include "Ultrasonic.h"
#include <PubSubClient.h>

bool doorState = 0;
bool lastState = 0;
Ultrasonic ultrasonic(12,13);
int  rf = ultrasonic.Ranging(0);
WiFiClient espClient;
PubSubClient client(espClient);

class comm {
  public:
  comm()
  {

  }
  
  const char* ssid     = "errans.iot";
  const char* password = "zamb0rah";
  const char* host = "homeassistant.iot";
  const int mqtt_port = 1883;
  int failCount = 0;
  bool connected = 0;
  bool started = 0;
 
  bool vacState = 0;
  int ledPin = 2;
  
  bool wfconnect() {
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
        delay(1000);
        Serial.print(WiFi.status());
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
          if (connectMillis + 60000 <= millis()) {
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

    return 0;
  };

  bool mqttconnect() {
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      client.setServer(host, mqtt_port);
      // Create a unique client ID based on ESP MAC address
      String clientId = "8266iot-GARAGE" + String(random(0, 1000));
      
      if (client.connect(clientId.c_str(), "mqttuser", "949500")) {
        Serial.println("connected");
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" trying again in 5 seconds");
        delay(5000);
      };
    };
    return 0;
  };
  
  bool send() {
    if (!client.connected()) {
      Serial.println("Send loop needs to reconnect MQTT");
      mqttconnect();
    };

    char* mqtt_topic = "outside/sensor/gdoor";
    char payloadStr[1];
    dtostrf(doorState, 1, 0, payloadStr);

    Serial.print("Publishing sensor reading: ");
    Serial.println(payloadStr);
    
    if (client.publish(mqtt_topic, payloadStr)) {
      Serial.println("Publish successful!");
    } else {
      Serial.println("Publish failed.");
    };

    return 0;
  };

};

comm cc = comm();

void setup() {
  Serial.begin(9600);
  while (!Serial);

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

  lastState = doorState;

  while (!cc.wfconnect() == 0);
  cc.mqttconnect();
  
}

void loop() {
  rf = ultrasonic.Ranging(0);
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

  if (lastState != doorState){
    lastState = doorState;
    Serial.println("doorState changed to " + doorState);
    cc.send();
  };
  
  delay(100);
}