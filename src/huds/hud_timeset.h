#pragma once
#include <vector>

#include "hud_base.h"

namespace toaster {

class HUDTimeset : public HUDBase {
public:
  virtual void init();
  virtual void process(Adafruit_SSD1306& oled);
  virtual void release();
  virtual void pressKey(uint16_t key, uint8_t mode);

public:
  int _select{0};
  uint8_t _hour{0};
  uint8_t _minute{0};
};

extern HUDTimeset hud_timeset;

};  // namespace toaster
