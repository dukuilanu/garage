#include <RadioLib.h>
#include "Arduino.h"
#include "Ultrasonic.h"

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
//doorState: 1 == closed
bool doorState = 1;
bool lastState = 1;
volatile bool operationDone = false;
// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

//trigger, echo
Ultrasonic ultrasonic(12,13);
int  rf = ultrasonic.Ranging(0);
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

void bounce(void)
{
    // we sent or received a packet, set the flag
    operationDone = true;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

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

}

void loop() {
  rf = ultrasonic.Ranging(0);
  Serial.println(rf);

  if (rf <= 14) {
    doorState = 1;
  }
  else {
    doorState = 0;
  };
  //test code
  //lastState = 0;
  //doorState = 1;

  if (lastState != doorState){
    lastState = doorState;
    Serial.println("doorState changed to " + doorState);
    Serial.print("[SX1276] Sending packet ... ");
    if (doorState == 1) {
    transmissionState = radio.startTransmit("GDOOR STATE 1 DE N1XQR K");
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));
      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);

      }
    } else {
      transmissionState = radio.startTransmit("GDOOR STATE 0 DE N1XQR K");
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));
      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);

      }
    }
  };
  
  delay(5000);
}