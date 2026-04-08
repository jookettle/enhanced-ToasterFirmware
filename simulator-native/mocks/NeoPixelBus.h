#pragma once
#include <stdint.h>
#include <vector>

struct RgbColor {
    uint8_t R, G, B;
    RgbColor(uint8_t r, uint8_t g, uint8_t b) : R(r), G(g), B(b) {}
};

template<typename T_FEATURE, typename T_METHOD>
class NeoPixelBus {
public:
    NeoPixelBus(uint16_t numPixels, uint8_t pin) : _pixels(numPixels, RgbColor(0,0,0)) {}
    void Begin() {}
    void Show() {}
    void SetPixelColor(uint16_t index, RgbColor color) {
        if (index < _pixels.size()) _pixels[index] = color;
    }
private:
    std::vector<RgbColor> _pixels;
};

class NeoGrbFeature {};
class NeoEsp32BitBangWs2812xMethod {};
class NeoEsp32I2s0Ws2812xMethod {};
class NeoEsp32Rmt0Ws2812xMethod {};
