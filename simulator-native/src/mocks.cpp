#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"
#include "SD.h"
#include "hal/controller/serial_debug.h"
#include "hal/controller/esp_now.h"
#include "hal/controller/ir_remote.h"
#include "hal/sensor/boopsensor.h"
#include "hal/sensor/lightsensor.h"
#include "hal/sensor/thermometer.h"
#include "hal/sensor/rtc.h"

#include <cmath>
#include <ctime>

SerialMock Serial;
ESPMock ESP;
TwoWire Wire;
SPIClass SPI;

// SD and FFat are both fs::FS-based mock filesystems
fs::SDClass SD;

// FFat uses the same FS base class
namespace fs {
class FFatClass : public FS {};
}
fs::FFatClass FFat;

namespace toaster {

SerialDebug::SerialDebug() {}

bool SerialDebug::begin() {
  return true;
}

void SerialDebug::loop() {
}

bool SerialDebug::pressKey(uint16_t key, uint8_t mode) {
  (void)key;
  (void)mode;
  return false;
}

bool SerialDebug::processCommand(const char* cmd) {
  (void)cmd;
  return false;
}

ESPNowRemote::ESPNowRemote() {}

bool ESPNowRemote::beginLocal() {
  _init = true;
  return true;
}

bool ESPNowRemote::beginI2C(uint8_t addr) {
  _i2c_addr = addr;
  _i2c_found = true;
  _init = true;
  Worker::begin(60);
  return true;
}

void ESPNowRemote::clearWhitelist() {
  _whitelist.clear();
}

void ESPNowRemote::addWhitelist(const uint8_t* mac) {
  MAC_ADDRESS address{};
  memcpy(address.mac, mac, sizeof(address.mac));
  _whitelist.push_back(address);
}

bool ESPNowRemote::checkWhitelist(const uint8_t* mac) const {
  (void)mac;
  return true;
}

bool ESPNowRemote::work() {
  return true;
}

IRRemote::IRRemote() {}

IRRemote::~IRRemote() {}

bool IRRemote::begin(uint16_t pin, bool debug) {
  (void)pin;
  _debug = debug;
  _init = true;
  return true;
}

void IRRemote::loop() {
}

bool IRRemote::processNEC_Emotion(uint32_t nec_code) {
  (void)nec_code;
  return false;
}

bool IRRemote::processNEC_Keypress(uint32_t nec_code) {
  (void)nec_code;
  return false;
}

bool BoopSensor::begin(uint8_t i2c_addr, uint16_t threshold, int sampling) {
  _i2c = i2c_addr;
  _threshold = threshold;
  _sampling = sampling;
  _init = true;
  _error = false;
  return true;
}

bool BoopSensor::work() {
  return true;
}

bool BoopSensor::isBoop() const {
  return false;
}

bool BoopSensor::tof_init() {
  return true;
}

void BoopSensor::tof_error(bool critical) {
  (void)critical;
  _error = true;
  ++_errors;
  ++_error_total;
}

void BoopSensor::tof_no_error() {
  _error = false;
}

LightSensor::~LightSensor() {}

bool LightSensor::beginLDR(int pin, float alpha, float alpha_init) {
  _type = LST_LDR;
  _pin = pin;
  _alpha = alpha;
  _alpha_init = alpha_init;
  Worker::begin(30);
  return true;
}

bool LightSensor::beginBH1750(uint8_t i2c, float alpha, float alpha_init) {
  _type = LST_BH1750;
  _pin = i2c;
  _alpha = alpha;
  _alpha_init = alpha_init;
  Worker::begin(30);
  return true;
}

bool LightSensor::work() {
  return true;
}

bool LightSensor::_begin() {
  return true;
}

float LightSensor::read() {
  return _value;
}

Thermometer::~Thermometer() {}

bool Thermometer::beginSHT31(uint32_t period, uint8_t i2c) {
  _type = TST_SHT31;
  _ptr = this;
  _temperature = 23.4f;
  _humidity = 46.8f;
  _read_tick_ms = Timer::get_millis();
  Worker::beginPeriod(period);
  (void)i2c;
  return true;
}

bool Thermometer::work() {
  if (!isBegin()) {
    return false;
  }

  const float seconds = static_cast<float>(Timer::get_millis()) / 1000.0f;
  _temperature = 23.2f + 0.8f * std::sin(seconds * 0.14f);
  _humidity = 45.5f + 2.5f * std::sin(seconds * 0.09f + 0.7f);
  _read_tick_ms = Timer::get_millis();
  return true;
}

RTC::~RTC() {}

bool RTC::beginDS3231(bool nofail) {
  (void)nofail;
  _type = RST_DS3231;
  _ptr = this;
  _synced = true;
  _blink = 0;

  const std::time_t now = std::time(nullptr);
  if (now != static_cast<std::time_t>(-1)) {
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    setDateTime(
      static_cast<uint16_t>(local.tm_year + 1900),
      static_cast<uint8_t>(local.tm_mon + 1),
      static_cast<uint8_t>(local.tm_mday),
      static_cast<uint8_t>(local.tm_hour),
      static_cast<uint8_t>(local.tm_min),
      static_cast<uint8_t>(local.tm_sec));
  }
  else {
    setDateTime(2026, 4, 8, 12, 0, 0);
  }

  Worker::begin(2);
  return true;
}

bool RTC::work() {
  if (!isBegin()) {
    return false;
  }

  const std::time_t now = std::time(nullptr);
  if (now != static_cast<std::time_t>(-1)) {
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    setDateTime(
      static_cast<uint16_t>(local.tm_year + 1900),
      static_cast<uint8_t>(local.tm_mon + 1),
      static_cast<uint8_t>(local.tm_mday),
      static_cast<uint8_t>(local.tm_hour),
      static_cast<uint8_t>(local.tm_min),
      static_cast<uint8_t>(local.tm_sec));
  }

  _blink = static_cast<uint8_t>((Timer::get_millis() / 500) % 2);
  return true;
}

void RTC::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  _year = year;
  _month = month;
  _day = day;
  _hour = hour;
  _minute = minute;
  _second = second;
  _synced = true;
}

void RTC::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
  _hour = hour;
  _minute = minute;
  _second = second;
  _synced = true;
}

bool RTC::sync() {
  return true;
}

bool RTC::isLeapYear(uint16_t year) {
  return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

uint8_t RTC::getDaysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  return (month >= 1 && month <= 12) ? days[month - 1] : 30;
}

}
