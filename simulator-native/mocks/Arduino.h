#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <stdint.h>
#include <stdio.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <stdarg.h>
#include <cstring>
#include <cstdint>
#include <functional>
#include <filesystem>
#include <fstream>
#include <memory>
#include "Print.h"
#include "native_scene.h"

// FreeRTOS mocks
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "Wire.h"
#include "SPI.h"
#include <map>
#include <set>


// Binary literals (classic Arduino)
#define B00000000 0
#define B00000001 1
#define B00000010 2
#define B00000011 3
#define B00000100 4
#define B00000101 5
#define B00000110 6
#define B00000111 7
#define B00001000 8
#define B00001001 9
#define B00001010 10
#define B00001011 11
#define B00001100 12
#define B00001101 13
#define B00001110 14
#define B00001111 15
#define B00010000 16
#define B00010001 17
#define B00011000 24
#define B00011100 28
#define B00011111 31
#define B00100000 32
#define B00111000 56
#define B00111110 62
#define B00111111 63
#define B01100010 98
#define B01100110 102
#define B01101100 108
#define B01110000 112
#define B01111000 120
#define B01111100 124
#define B01111111 127
#define B10000000 128
#define B11000000 192
#define B11100000 224
#define B11110000 240
#define B11111000 248
#define B11111100 252
#define B11111110 254
#define B11111111 255

// Additional binary literals found in hud_base.cpp
#define B00111100 60
#define B01100000 96

#define MSBFIRST 1
#define LSBFIRST 0
#define SPI_BITORDER_MSBFIRST MSBFIRST
#define SPI_MODE0 0x00

typedef int BitOrder;

class __FlashStringHelper;

class String : public std::string {
public:
  using std::string::string;
  using std::string::operator=;

  String() = default;
  String(const std::string& other) : std::string(other) {}
  operator const char*() const { return c_str(); }

  bool isEmpty() const {
    return empty();
  }
};

namespace fs {
  class File;
}

namespace nativefs {
  struct State {
    enum class Kind {
      None,
      File,
      Directory,
    };

    Kind kind{Kind::None};
    std::filesystem::path root;
    std::filesystem::path path;
    std::shared_ptr<std::ifstream> stream;
    std::vector<std::filesystem::path> entries;
    size_t dir_index{0};
    size_t size{0};
    size_t pos{0};
  };

  inline std::filesystem::path normalize_path(const std::filesystem::path& root, const char* path) {
    std::string rel = (path == nullptr) ? "" : path;
    if (rel.rfind("/sd/", 0) == 0) {
      rel.erase(0, 4);
    }
    else if (rel.rfind("sd/", 0) == 0) {
      rel.erase(0, 3);
    }
    else if (!rel.empty() && rel[0] == '/') {
      rel.erase(0, 1);
    }

    return root / std::filesystem::path(rel);
  }

  inline std::filesystem::path to_relative_display_path(const std::filesystem::path& root, const std::filesystem::path& path) {
    std::error_code ec;
    auto rel = std::filesystem::relative(path, root, ec);
    if (ec) {
      rel = path.filename();
    }

    auto rel_str = rel.generic_string();
    if (rel_str.empty()) {
      rel_str = path.filename().generic_string();
    }
    if (rel_str.empty()) {
      rel_str = "/";
    }
    if (rel_str.front() != '/') {
      rel_str.insert(rel_str.begin(), '/');
    }
    return std::filesystem::path(rel_str);
  }
}

namespace fs {
  class File : public Stream {
  public:
    File() = default;

    operator bool() const {
      return _state != nullptr && _state->kind != nativefs::State::Kind::None;
    }

    size_t write(uint8_t) override { return 1; }
    size_t write(const uint8_t*, size_t size) override { return size; }

    int read() override {
      if (!isFile() || _state->stream == nullptr || !_state->stream->good()) {
        return -1;
      }

      char ch = 0;
      _state->stream->get(ch);
      if (_state->stream->eof()) {
        return -1;
      }

      ++_state->pos;
      return static_cast<unsigned char>(ch);
    }

    size_t read(uint8_t* buffer, size_t size) {
      if (!isFile() || buffer == nullptr || size == 0 || _state->stream == nullptr) {
        return 0;
      }

      _state->stream->read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(size));
      size_t read_size = static_cast<size_t>(_state->stream->gcount());
      _state->pos += read_size;
      return read_size;
    }

    size_t readBytes(char* buffer, size_t size) {
      return read(reinterpret_cast<uint8_t*>(buffer), size);
    }

    size_t readBytes(uint8_t* buffer, size_t size) {
      return read(buffer, size);
    }

    size_t size() const {
      return (_state != nullptr) ? _state->size : 0;
    }

    size_t position() const {
      return (_state != nullptr) ? _state->pos : 0;
    }

    bool seek(size_t pos) {
      if (!isFile() || _state->stream == nullptr) {
        return false;
      }

      _state->stream->clear();
      _state->stream->seekg(static_cast<std::streamoff>(pos));
      if (!_state->stream->good() && !_state->stream->eof()) {
        return false;
      }

      _state->pos = pos;
      return true;
    }

    int available() override {
      if (!isFile() || _state == nullptr) {
        return 0;
      }
      if (_state->pos >= _state->size) {
        return 0;
      }
      return static_cast<int>(_state->size - _state->pos);
    }

    bool isDirectory() const {
      return _state != nullptr && _state->kind == nativefs::State::Kind::Directory;
    }

    String getNextFileName() {
      if (!isDirectory() || _state->dir_index >= _state->entries.size()) {
        return String();
      }

      auto child = _state->entries[_state->dir_index++];
      auto display_path = nativefs::to_relative_display_path(_state->root, child);
      return String(display_path.generic_string());
    }

    File openNextFile() {
      if (!isDirectory() || _state->dir_index >= _state->entries.size()) {
        return File();
      }

      auto child = _state->entries[_state->dir_index++];
      auto state = std::make_shared<nativefs::State>();
      state->root = _state->root;
      state->path = child;
      state->kind = std::filesystem::is_directory(child) ? nativefs::State::Kind::Directory : nativefs::State::Kind::File;

      if (state->kind == nativefs::State::Kind::Directory) {
        std::vector<std::filesystem::path> entries;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(child, ec)) {
          entries.push_back(entry.path());
        }
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
          return a.generic_string() < b.generic_string();
        });
        state->entries = std::move(entries);
      }
      else {
        std::error_code ec;
        state->size = std::filesystem::file_size(child, ec);
        state->stream = std::make_shared<std::ifstream>(child, std::ios::binary);
        if (state->stream == nullptr || !state->stream->is_open()) {
          return File();
        }
      }

      return File(std::move(state));
    }

    void close() {
      _state.reset();
    }

    static File fromState(std::shared_ptr<nativefs::State> state) {
      return File(std::move(state));
    }

  private:
    explicit File(std::shared_ptr<nativefs::State> state) : _state(std::move(state)) {}

    bool isFile() const {
      return _state != nullptr && _state->kind == nativefs::State::Kind::File;
    }

  private:
    std::shared_ptr<nativefs::State> _state;
  };
}
using fs::File;

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1


// Basic Arduino types
typedef bool boolean;
typedef uint8_t byte;

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x01
#define OUTPUT 0x02
#define INPUT_PULLUP 0x03

// Serial Mock
class SerialMock : public Stream {
public:
  void begin(unsigned long speed) {}
  size_t write(uint8_t) override { return 1; }
  size_t write(const uint8_t*, size_t size) override { return size; }
  void print(const char* s) { toaster::simulator_log_append(s != nullptr ? std::string(s) : std::string()); }
  void print(String s) { toaster::simulator_log_append(std::string(s.c_str())); }
  void println(const char* s) { toaster::simulator_log_append((s != nullptr ? std::string(s) : std::string()) + "\n"); }
  void println(String s) { toaster::simulator_log_append(std::string(s.c_str()) + "\n"); }
  void println() { toaster::simulator_log_append("\n"); }
  void printf(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    toaster::simulator_log_append(buffer);
  }
  operator bool() { return true; }
  int available() { return 0; }
  int read() { return -1; }
  void loop() {}
  bool isTextMode() { return false; }
};

extern SerialMock Serial;

// Time functions
inline unsigned long millis() {
  static auto start = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

inline unsigned long micros() {
  static auto start = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
}

inline long long esp_timer_get_time() {
  return micros();
}

inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void yield() {
  std::this_thread::yield();
}

// Misc
inline void pinMode(int pin, int mode) {}
inline int digitalRead(int pin) { return LOW; }
inline void digitalWrite(int pin, int val) {}
inline int analogRead(int pin) { return 0; }

#define PROGMEM
#define PSTR(s) (s)
#define F(s) (s)
#define FILE_READ "r"
#define HEX 16

// Min/Max
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

// ESP32 specific stubs often used in Arduino
inline bool psramFound() { return true; }
inline void psramInit() {}

#ifndef CONFIG_WL_SECTOR_SIZE
#define CONFIG_WL_SECTOR_SIZE 512
#endif

class ESPMock {
public:
  uint32_t getPsramSize() { return 4 * 1024 * 1024; }
  uint32_t getFreeHeap() { return 256 * 1024; }
  uint32_t getFreePsram() { return 2 * 1024 * 1024; }
};

extern ESPMock ESP;
