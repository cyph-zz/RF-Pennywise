/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/rfclown

   RF-Pennywise by Cypher:
   https://github.com/cyph-zz/RF-Pennywise
   ________________________________________ */

#include "setting.h"
#include "config.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
Adafruit_NeoPixel pixels(1, 14, NEO_GRB + NEO_KHZ800);

OperationMode current_Mode = WiFi_MODULE;
volatile Operation current = DEACTIVE_MODE;

volatile bool ChangeRequested  = false;
volatile bool ChangeRequested1 = false;
volatile bool ChangeRequested2 = false;

unsigned long lastPressTime = 0;
unsigned long lastPressTime1 = 0;
unsigned long lastPressTime2 = 0;

bool neoPixelActive = false;
uint8_t oledBrightness = 100;

RF24 RadioA(NRF_CE_PIN_A, NRF_CSN_PIN_A);
RF24 RadioB(NRF_CE_PIN_B, NRF_CSN_PIN_B);
RF24 RadioC(NRF_CE_PIN_C, NRF_CSN_PIN_C);

void setRadiosNeutralState() {
  RadioA.stopListening();
  RadioA.setAutoAck(false);
  RadioA.setRetries(0, 0);
  RadioA.powerDown(); 
  digitalWrite(NRF_CE_PIN_A, LOW); 

  RadioB.stopListening();
  RadioB.setAutoAck(false);
  RadioB.setRetries(0, 0);
  RadioB.powerDown(); 
  digitalWrite(NRF_CE_PIN_B, LOW); 

  RadioC.stopListening();
  RadioC.setAutoAck(false);
  RadioC.setRetries(0, 0);
  RadioC.powerDown(); 
  digitalWrite(NRF_CE_PIN_C, LOW); 
}

void configureNrf(RF24 &radio) {
  radio.begin();
  radio.setAutoAck(false);
  radio.stopListening();
  radio.setRetries(0, 0);
  radio.setPALevel(RF24_PA_MAX, true);
  radio.setDataRate(RF24_2MBPS);
  radio.setCRCLength(RF24_CRC_DISABLED);
}

void setupRadioA() {
  configureNrf(RadioA);
}

void setupRadioB() {
  configureNrf(RadioB);
}

void setupRadioC() {
  configureNrf(RadioC);
}

void initAllRadios() {
  setupRadioA();
  setupRadioB();
  setupRadioC();
}

void Str(uint8_t x, uint8_t y, const uint8_t* asciiArray, size_t len) {
  char buf[64]; 
  for (size_t i = 0; i < len && i < sizeof(buf) - 1; i++) {
    buf[i] = (char)asciiArray[i];
  }
  buf[len] = '\0';

  u8g2.drawStr(x, y, buf);
}

void CenteredStr(uint8_t screenWidth, uint8_t y, const uint8_t* asciiArray, size_t len, const uint8_t* font) {
  char buf[64];
  for (size_t i = 0; i < len && i < sizeof(buf) - 1; i++) {
    buf[i] = (char)asciiArray[i];
  }
  buf[len] = '\0';

  u8g2.setFont((const uint8_t*)font);
  int16_t w = u8g2.getUTF8Width(buf);
  u8g2.setCursor((screenWidth - w) / 2, y);
  u8g2.print(buf);
}

void conf() {
  u8g2.setBitmapMode(1);
  u8g2.clearBuffer();
  CenteredStr(128, 15, txt_n, sizeof(txt_n), u8g2_font_ncenB10_tr);
  CenteredStr(120, 35, txt_c, sizeof(txt_c), u8g2_font_ncenB08_tr);
  CenteredStr(128, 50, txt_v, sizeof(txt_v), u8g2_font_6x10_tf);
  u8g2.sendBuffer();
  setNeoPixelColour("red");
  delay(750);
  setNeoPixelColour("green");
  delay(750);
  setNeoPixelColour("blue");
  delay(750);
  setNeoPixelColour("purple");
  delay(750);
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, 128, 64, cred);
  u8g2.sendBuffer();
  setNeoPixelColour("white");
  delay(2000);
  setNeoPixelColour("null");
}

 
