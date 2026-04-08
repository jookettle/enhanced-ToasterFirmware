#pragma once
#include "Arduino.h"

class TwoWire {
public:
  bool begin(int sda = -1, int scl = -1, uint32_t frequency = 0) { return true; }
  void beginTransmission(uint8_t address) {}
  uint8_t endTransmission(bool stop = true) { (void)stop; return 0; }
  size_t write(uint8_t data) { return 1; }
  size_t write(const uint8_t* data, size_t size) { (void)data; return size; }
  uint8_t read() { return 0; }
  int available() { return 0; }
  size_t requestFrom(uint8_t address, size_t size, uint8_t stop = true) { (void)address; (void)stop; return size; }
  size_t readBytes(uint8_t* buffer, size_t size) { (void)buffer; return size; }
  void end() {}
  void setClock(uint32_t frequency) { (void)frequency; }
};

extern TwoWire Wire;
