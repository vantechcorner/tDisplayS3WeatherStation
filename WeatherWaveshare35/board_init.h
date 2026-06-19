#pragma once

#include <Wire.h>

// Waveshare ESP32-Touch-LCD-3.5 (ESP32 classic, ST7796, TCA9554 @ 0x20, AXP2101 @ 0x34)
#define WS_I2C_SDA 21
#define WS_I2C_SCL 22
#define WS_TCA9554_ADDR 0x20
#define WS_AXP2101_ADDR 0x34
#define WS_TFT_BL_PIN 25

#define WS_TCA9554_INPUT_REG 0x00
#define WS_TCA9554_OUTPUT_REG 0x01
#define WS_TCA9554_CONFIG_REG 0x03

inline bool wsI2cWrite(uint8_t addr, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

inline void wsAxp2101Write(uint8_t reg, uint8_t value) {
  wsI2cWrite(WS_AXP2101_ADDR, reg, value);
}

// Power rails required before the LCD can run (from Waveshare / xiaozhi reference)
inline void wsInitAxp2101() {
  wsAxp2101Write(0x22, 0b110);
  wsAxp2101Write(0x27, 0x10);
  wsAxp2101Write(0x80, 0x01);
  wsAxp2101Write(0x90, 0x00);
  wsAxp2101Write(0x91, 0x00);
  wsAxp2101Write(0x82, (3300 - 1500) / 100);
  wsAxp2101Write(0x92, (3300 - 500) / 100);
  wsAxp2101Write(0x96, (1500 - 500) / 100);
  wsAxp2101Write(0x97, (2800 - 500) / 100);
  wsAxp2101Write(0x90, 0x31);
  wsAxp2101Write(0x64, 0x02);
  wsAxp2101Write(0x61, 0x02);
  wsAxp2101Write(0x62, 0x08);
  wsAxp2101Write(0x63, 0x01);
}

// LCD reset/power via TCA9554 EXIO P0..P2 — must run before tft.init()
inline void wsInitTca9554Display() {
  const uint8_t mask = 0x07;  // P0, P1, P2
  wsI2cWrite(WS_TCA9554_ADDR, WS_TCA9554_CONFIG_REG, ~mask);
  delay(100);
  wsI2cWrite(WS_TCA9554_ADDR, WS_TCA9554_OUTPUT_REG, 0x02);  // P1 low (reset)
  delay(100);
  wsI2cWrite(WS_TCA9554_ADDR, WS_TCA9554_OUTPUT_REG, mask);   // P0..P2 high
  delay(20);
}

inline void wsInitBacklight(uint8_t brightness = 200) {
  pinMode(WS_TFT_BL_PIN, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(WS_TFT_BL_PIN, 0);
  ledcWrite(0, brightness);
}

inline void wsInitBoard() {
  Wire.begin(WS_I2C_SDA, WS_I2C_SCL);
  wsInitAxp2101();
  wsInitTca9554Display();
  wsInitBacklight();
}
