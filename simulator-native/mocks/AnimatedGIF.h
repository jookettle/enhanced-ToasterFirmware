#pragma once
#include <stdint.h>

struct GIFFILE {
  void* fHandle{nullptr};
  int32_t iPos{0};
};

struct GIFDRAW {
  void* pUser{nullptr};
  const uint8_t* pPalette{nullptr};
  const uint8_t* pPixels{nullptr};
  uint8_t ucTransparent{0};
  uint8_t ucHasTransparency{0};
  uint8_t ucDisposalMethod{0};
  int iWidth{0};
  int iHeight{0};
  int iX{0};
  int iY{0};
  int x{0};
  int y{0};
};

#define GIF_PALETTE_RGB888 0

class AnimatedGIF {
public:
  using OpenCallback = void* (*)(const char*, int32_t*);
  using CloseCallback = void (*)(void*);
  using ReadCallback = int32_t (*)(GIFFILE*, uint8_t*, int32_t);
  using SeekCallback = int32_t (*)(GIFFILE*, int32_t);
  using DrawCallback = void (*)(GIFDRAW*);

  void begin(int) {}
  bool open(const char*, OpenCallback, CloseCallback, ReadCallback, SeekCallback, DrawCallback) { return false; }
  int getLastError() const { return 0; }
  int getCanvasWidth() const { return 0; }
  int getCanvasHeight() const { return 0; }
  bool nextFrame(bool) { return false; }
  int playFrame(bool, int*, void*) { return -1; }
  void close() {}
};
