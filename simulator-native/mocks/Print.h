#pragma once
#include <stddef.h>
#include <stdint.h>

class Print {
public:
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *buffer, size_t size) {
    (void)buffer;
    return size;
  }
  // Add other print methods as needed
  size_t print(const char*) { return 0; }
  size_t println(const char*) { return 0; }
  size_t print(int, int = 10) { return 0; }
  size_t print(unsigned int, int = 10) { return 0; }
  size_t print(long, int = 10) { return 0; }
  size_t print(unsigned long, int = 10) { return 0; }
  size_t println() { return 0; }
};

class Stream : public Print {
public:
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual int peek() { return -1; }
  virtual void flush() {}
};
