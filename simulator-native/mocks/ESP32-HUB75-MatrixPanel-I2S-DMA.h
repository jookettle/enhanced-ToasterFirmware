#pragma once
#include <stdint.h>

namespace HUB75_I2S_CFG {
struct i2s_pins {
    int r1, g1, b1, r2, g2, b2, a, b, c, d, e, lat, oe, clk;
};
}

class MatrixPanel_I2S_DMA {
public:
    MatrixPanel_I2S_DMA(const HUB75_I2S_CFG::i2s_pins& pins) {}
    bool begin() { return true; }
    void setPanelBrightness(int b) {}
    void drawPixelRGB888(int x, int y, uint8_t r, uint8_t g, uint8_t b) {}
    void flipDMABuffer() {}
    int get_refresh_rate() { return 120; }
};
