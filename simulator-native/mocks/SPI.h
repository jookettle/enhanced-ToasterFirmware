#pragma once
#include "Arduino.h"

class SPISettings {
public:
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {}
  SPISettings() {}
};

class SPIClass {
public:
  void begin() {}
  void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1) {
    (void)sck; (void)miso; (void)mosi; (void)ss;
  }
  void beginTransaction(const SPISettings& settings) { (void)settings; }
  void endTransaction() {}
  uint8_t transfer(uint8_t data) { return 0; }
  void transfer(uint8_t* buffer, size_t size) { (void)buffer; (void)size; }
  void setBitOrder(uint8_t bitOrder) { (void)bitOrder; }
  void setDataMode(uint8_t dataMode) { (void)dataMode; }
};

extern SPIClass SPI;
