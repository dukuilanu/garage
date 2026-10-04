#include <WiFi.h>
#include <RadioLib.h>
#include "Arduino.h"
#include <PubSubClient.h>

#define I2C_SDA                     21
#define I2C_SCL                     22
#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN              26
#define RADIO_RST_PIN               23
#define RADIO_DIO1_PIN              33
#define RADIO_DIO2_PIN              32
#define SDCARD_MOSI                 15
#define SDCARD_MISO                 2
#define SDCARD_SCLK                 14
#define SDCARD_CS                   13
#define BOARD_LED                   25
#define LED_ON                      HIGH
#define ADC_PIN                     35
#define BOARD_VARIANT_NAME          "T3 LoRa32" //https://lilygo.cc/products/lora3
#define CONFIG_RADIO_FREQ           433.0
#define CONFIG_RADIO_OUTPUT_POWER   17
bool doorState = 0;
bool lastState = 0;
volatile bool operationDone = false;

WiFiClient espClient;
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
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

void bounce(void)
{
    // we sent or received a packet, set the flag
    operationDone = true;
}

comm cc = comm();

void setup() {
  Serial.begin(9600);
  while (!Serial);

  lastState = doorState;

  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
  // initialize SX1276 with default settings
  Serial.print(F("[SX1276] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
  } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true) {
          delay(10);
      }
  }
  radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER);
  radio.setPacketReceivedAction(bounce);
  radio.startReceive();

  while (!cc.wfconnect() == 0);
  cc.mqttconnect();
  
}

void loop() {
  if (operationDone) {
    // reset flag
    operationDone = false;
    radio.startReceive();

    String str;
    int state = radio.readData(str);

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
        Serial.println(F("[SX1276] Received packet!"));

        // print data of the packet
        Serial.print(F("[SX1276] Data:\t\t"));
        Serial.println(str);

        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("[SX1276] RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("[SX1276] SNR:\t\t"));
        Serial.print(radio.getSNR());
        Serial.println(F(" dB"));

        if (str == "GDOOR STATE 1 DE N1XQR K"){
          doorState = 1;
          lastState = doorState;
          Serial.println("doorState changed to " + doorState);
          cc.send();
        };

          if (str == "GDOOR STATE 0 DE N1XQR K"){
          doorState = 0;
          lastState = doorState;
          Serial.println("doorState changed to " + doorState);
          cc.send();
        };

    } else {
      Serial.println("receive failed.");
    }
  }
  
  //delay(5000);
}