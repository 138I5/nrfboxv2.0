/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/l15dev/nrfbox
   ________________________________________ */

#include "setting.h"
#include "icon.h"
#include "config.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
Adafruit_NeoPixel pixels(1, 14, NEO_GRB + NEO_KHZ800);
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

void utils() {
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, 128, 64, bitmap_logo_flip);
  u8g2.sendBuffer();
  delay(2000);

  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.drawHLine(0, 13, 128);
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawBox(0, 0, 128, 13);
  u8g2.setDrawColor(0);
  CenteredStr(128, 10, txt_n, sizeof(txt_n), u8g2_font_5x7_tf);
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_6x10_tf);
  Str(10, 30, Line_A, sizeof(Line_A));
  Str(10, 44, Line_B, sizeof(Line_B));
  Str(10, 58, Line_C, sizeof(Line_C));
  u8g2.sendBuffer();
}

void conf() {
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, 128, 64, bitmap_logo_flip);
  u8g2.setDrawColor(0);
  u8g2.drawBox(30, 20, 68, 28);
  u8g2.setDrawColor(1);
  CenteredStr(128, 32, txt_n, sizeof(txt_n), u8g2_font_ncenB14_tr);
  CenteredStr(128, 45, txt_v, sizeof(txt_v), u8g2_font_5x7_tf);
  u8g2.sendBuffer();
  delay(3000);
}

namespace Setting {

#define EEPROM_ADDRESS_NEOPIXEL 0
#define EEPROM_ADDRESS_BRIGHTNESS 1

int currentOption = 0;
int totalOptions = 3; 

bool buttonUpPressed = false;
bool buttonDownPressed = false;
bool buttonSelectPressed = false;

void updateFirmware() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 15, "Updating Firmware.");
  u8g2.sendBuffer();

  u8g2.setFont(u8g2_font_5x8_tr);
  if (!SD.begin(SD_CS_PIN)) {
    u8g2.drawStr(0, 30, "[ SD Init Failed ]");
    u8g2.sendBuffer();
    delay(2000);
    return;
  }

  if (!SD.exists(FIRMWARE_FILE)) {
    u8g2.drawStr(0, 30, "[ File Not Found ]");
    u8g2.sendBuffer();
    delay(2000);
    return;
  }

  File firmware = SD.open(FIRMWARE_FILE);
  if (!firmware) {
    u8g2.drawStr(0, 30, "[ Open Failed ]");
    u8g2.sendBuffer();
    delay(2000);
    return;
  }

  if (firmware) {
    u8g2.drawStr(0, 30, "[ Wait a Moment ]");
    u8g2.sendBuffer();
  }

  if (!Update.begin(firmware.size())) {
    u8g2.drawStr(0, 30, "[ Update Init Failed ]");
    u8g2.sendBuffer();
    firmware.close();
    delay(2000);
    return;
  }

  Update.writeStream(firmware);
  if (Update.end(true)) {
    u8g2.drawStr(0, 45, "[ Update Success! ]");
    u8g2.sendBuffer();
    delay(1000);
    ESP.restart();
  } else {
    u8g2.drawStr(0, 45, "[ Update Failed ]");
    u8g2.sendBuffer();
    delay(2000);
  }

  firmware.close();
}

void toggleOption(int option) {
  if (option == 0) { 
    neoPixelActive = !neoPixelActive;
    EEPROM.write(EEPROM_ADDRESS_NEOPIXEL, neoPixelActive);
    EEPROM.commit();
    Serial.print("NeoPixel is now ");
    Serial.println(neoPixelActive ? "Enabled" : "Disabled");

  } else if (option == 1) { 
    uint8_t brightnessPercent = map(oledBrightness, 0, 255, 0, 100); 
    brightnessPercent += 10; 
    if (brightnessPercent > 100) brightnessPercent = 0; 
    oledBrightness = map(brightnessPercent, 0, 100, 0, 255); 

    u8g2.setContrast(oledBrightness); 
    EEPROM.write(EEPROM_ADDRESS_BRIGHTNESS, oledBrightness);
    EEPROM.commit();

    Serial.print("Brightness set to: ");
    Serial.print(brightnessPercent);
    Serial.println("%");

  } else if (option == 2) {
    updateFirmware();
  }
}

void handleButtons() {
  if (!digitalRead(BUTTON_UP_PIN)) {
    if (!buttonUpPressed) {
      buttonUpPressed = true;
      currentOption = (currentOption - 1 + totalOptions) % totalOptions;
    }
  } else {
    buttonUpPressed = false;
  }

  if (!digitalRead(BUTTON_DOWN_PIN)) {
    if (!buttonDownPressed) {
      buttonDownPressed = true;
      currentOption = (currentOption + 1) % totalOptions;
    }
  } else {
    buttonDownPressed = false;
  }

  if (!digitalRead(BTN_PIN_RIGHT)) {
    if (!buttonSelectPressed) {
      buttonSelectPressed = true;
      toggleOption(currentOption);
    }
  } else {
    buttonSelectPressed = false;
  }
}

void displayMenu() {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Settings:");

  u8g2.setFont(u8g2_font_5x8_tr);
  if (currentOption == 0) {
    u8g2.drawStr(0, 30, "> NeoPixel: ");
  } else {
    u8g2.drawStr(0, 30, "  NeoPixel: ");
  }

  if (currentOption == 1) {
    u8g2.drawStr(0, 45, "> Brightness: ");
  } else {
    u8g2.drawStr(0, 45, "  Brightness: ");
  }

  if (currentOption == 2) {
    u8g2.drawStr(0, 60, "> Update Firmware");
  } else {
    u8g2.drawStr(0, 60, "  Update Firmware");
  }

  u8g2.setCursor(80, 30);
  u8g2.print(neoPixelActive ? "Enabled" : "Disabled");

  u8g2.setCursor(80, 45);
  uint8_t brightnessPercent = map(oledBrightness, 0, 255, 0, 100);
  u8g2.print(brightnessPercent);
  u8g2.print("%");

  u8g2.sendBuffer();
}

void settingSetup() {
  Serial.begin(115200);

  EEPROM.begin(512);

  neoPixelActive = EEPROM.read(EEPROM_ADDRESS_NEOPIXEL);
  oledBrightness = EEPROM.read(EEPROM_ADDRESS_BRIGHTNESS);
  
  if (oledBrightness > 255) oledBrightness = 128; 
  u8g2.setContrast(oledBrightness);

  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_PIN_RIGHT, INPUT_PULLUP);
}

void settingLoop() {
  handleButtons();
  displayMenu();
  }
}

namespace Favorites {
  int favOption = 0;
  int favState = 0;

  void drawFavMenu() {
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(30, 12, "Favorites");
    u8g2.drawHLine(0, 14, 128);

    if (favOption == 0) u8g2.drawStr(5, 32, "> WLAN Jammer");
    else u8g2.drawStr(5, 32, "  WLAN Jammer");

    if (favOption == 1) u8g2.drawStr(5, 52, "> BLE Jammer");
    else u8g2.drawStr(5, 52, "  BLE Jammer");

    u8g2.sendBuffer();
  }

  void favSetup() {
    favOption = 0;
    favState = 0;
    drawFavMenu();
  }

  void favLoop() {
    if (favState == 1) {
      Jammer::jammerLoop();
      return;
    }
    if (favState == 2) {
      BleJammer::blejammerLoop();
      return;
    }

    if (!digitalRead(BUTTON_UP_PIN)) {
      favOption = (favOption - 1 + 2) % 2;
      drawFavMenu();
      delay(200);
    } else if (!digitalRead(BUTTON_DOWN_PIN)) {
      favOption = (favOption + 1) % 2;
      drawFavMenu();
      delay(200);
    } else if (!digitalRead(BTN_PIN_RIGHT)) {
      if (favOption == 0) Jammer::jammerSetup();
      else BleJammer::blejammerSetup();
      favState = favOption + 1;
      delay(200);
    }
  }
} 
