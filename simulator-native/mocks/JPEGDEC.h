#pragma once
#include <stdint.h>
#include <stddef.h>

struct JPEGDRAW {
  int iWidth{0};
  int iHeight{0};
  int iWidthUsed{0};
  int x{0};
  int y{0};
  int iBpp{16};
  uint16_t* pPixels{nullptr};
  void* pUser{nullptr};
};

#define RGB565_LITTLE_ENDIAN 0

class JPEGDEC {
public:
  using DrawCallback = int (*)(JPEGDRAW*);

  bool openRAM(uint8_t*, size_t, DrawCallback) { return false; }
  int getLastError() const { return 0; }
  int getWidth() const { return 0; }
  int getHeight() const { return 0; }
  void setUserPointer(void*) {}
  void setPixelType(int) {}
  bool decode(int, int, int) { return false; }
  void close() {}
};
