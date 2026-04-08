#pragma once

#include "Arduino.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#define SSD1306_WHITE 1
#define SSD1306_BLACK 0
#define SSD1306_SWITCHCAPVCC 0x2

typedef struct {
    uint16_t bitmapOffset;
    uint8_t width;
    uint8_t height;
    uint8_t xAdvance;
    int8_t xOffset;
    int8_t yOffset;
} GFXglyph;

typedef struct {
    uint8_t* bitmap;
    GFXglyph* glyph;
    uint8_t first;
    uint8_t last;
    uint8_t yAdvance;
} GFXfont;

namespace toaster {
void simulator_set_oled_pixels(const uint8_t* pixels, size_t width, size_t height);
}

class Adafruit_GFX {
public:
    Adafruit_GFX(int16_t w, int16_t h) : _width(w), _height(h) {}
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    void fillScreen(uint16_t color) { (void)color; }
    int16_t width() const { return _width; }
    int16_t height() const { return _height; }

protected:
    int16_t _width, _height;
};

class Adafruit_SSD1306 : public Adafruit_GFX {
public:
    Adafruit_SSD1306(int16_t w, int16_t h, void* wire = nullptr, int8_t rst_pin = -1)
        : Adafruit_GFX(w, h), _pixels(static_cast<size_t>(w) * static_cast<size_t>(h), 0) {
        (void)wire;
        (void)rst_pin;
    }

    bool begin(uint8_t switchvcc = 0, uint8_t i2caddr = 0x3C, bool reset = true, bool periphBegin = true) {
        (void)switchvcc;
        (void)i2caddr;
        (void)reset;
        (void)periphBegin;
        clearDisplay();
        display();
        return true;
    }

    void display() {
        toaster::simulator_set_oled_pixels(_pixels.data(), static_cast<size_t>(_width), static_cast<size_t>(_height));
    }

    void clearDisplay() {
        std::fill(_pixels.begin(), _pixels.end(), 0);
        _cursor_x = 0;
        _cursor_y = 0;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        setPixelClipped(x, y, color != SSD1306_BLACK);
    }

    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
        if (w <= 0) {
            return;
        }

        const bool on = color != SSD1306_BLACK;
        for (int16_t dx = 0; dx < w; ++dx) {
            setPixelClipped(x + dx, y, on);
        }
    }

    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
        if (h <= 0) {
            return;
        }

        const bool on = color != SSD1306_BLACK;
        for (int16_t dy = 0; dy < h; ++dy) {
            setPixelClipped(x, y + dy, on);
        }
    }

    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color) {
        if (bitmap == nullptr || w <= 0 || h <= 0) {
            return;
        }

        const bool on = color != SSD1306_BLACK;
        const int byte_width = (w + 7) / 8;
        for (int16_t yy = 0; yy < h; ++yy) {
            for (int16_t xx = 0; xx < w; ++xx) {
                const size_t byte_index = static_cast<size_t>(yy) * static_cast<size_t>(byte_width) + static_cast<size_t>(xx / 8);
                const uint8_t byte = bitmap[byte_index];
                const bool bit_on = (byte & (0x80 >> (xx & 7))) != 0;
                if (bit_on) {
                    setPixelClipped(x + xx, y + yy, on);
                }
            }
        }
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (w <= 0 || h <= 0) {
            return;
        }

        const bool on = color != SSD1306_BLACK;
        for (int16_t dy = 0; dy < h; ++dy) {
            for (int16_t dx = 0; dx < w; ++dx) {
                setPixelClipped(x + dx, y + dy, on);
            }
        }
    }

    void setCursor(int16_t x, int16_t y) { _cursor_x = x; _cursor_y = y; }
    int16_t getCursorX() const { return _cursor_x; }
    int16_t getCursorY() const { return _cursor_y; }
    void setTextSize(uint8_t s) { _text_size = std::max<uint8_t>(1, s); }
    void setFont(const void* f) { _font = static_cast<const GFXfont*>(f); }
    void setTextWrap(bool w) { _wrap = w; }
    void setTextColor(uint16_t c) { _text_color = c; }

    void getTextBounds(const char* str, int16_t x, int16_t y, int16_t* bx, int16_t* by, uint16_t* bw, uint16_t* bh) {
        (void)x;
        (void)y;

        const std::string text = (str != nullptr) ? str : "";
        if (bx != nullptr) {
            *bx = 0;
        }
        if (by != nullptr) {
            *by = 0;
        }

        if (_font != nullptr && _font->glyph != nullptr && _font->bitmap != nullptr && !_hasEmptyFont()) {
            uint32_t width = 0;
            for (char ch : text) {
                if (ch == '\n' || ch == '\r') {
                    break;
                }

                const auto* glyph = lookupGlyph(static_cast<uint8_t>(ch));
                if (glyph != nullptr) {
                    width += static_cast<uint32_t>(glyph->xAdvance) * _text_size;
                }
            }

            if (bw != nullptr) {
                *bw = static_cast<uint16_t>(width);
            }
            if (bh != nullptr) {
                *bh = static_cast<uint16_t>(static_cast<uint32_t>(_font->yAdvance) * _text_size);
            }
            return;
        }

        if (bw != nullptr) {
            *bw = static_cast<uint16_t>(text.size() * 6 * _text_size);
        }
        if (bh != nullptr) {
            *bh = static_cast<uint16_t>(8 * _text_size);
        }
    }

    size_t print(const char* s) { return appendString((s != nullptr) ? s : ""); }
    size_t print(const String& s) { return appendString(s.c_str()); }

    size_t println(const char* s) {
        size_t n = print(s);
        n += write('\n');
        return n;
    }

    size_t println() {
        return write('\n');
    }

    size_t write(uint8_t c) {
        if (c == '\r') {
            return 1;
        }

        if (c == '\n') {
            _cursor_x = 0;
            _cursor_y += lineAdvance();
            return 1;
        }

        if (_font == nullptr || _font->glyph == nullptr || _font->bitmap == nullptr || _hasEmptyFont()) {
            drawFallbackChar(static_cast<char>(c));
            return 1;
        }

        const auto* glyph = lookupGlyph(c);
        if (glyph == nullptr) {
            return 1;
        }

        const int16_t base_x = _cursor_x + static_cast<int16_t>(glyph->xOffset) * _text_size;
        const int16_t base_y = _cursor_y + static_cast<int16_t>(glyph->yOffset) * _text_size;
        const uint8_t* bitmap = _font->bitmap;
        uint16_t byte_index = glyph->bitmapOffset;
        uint8_t bits = 0;

        for (uint8_t yy = 0; yy < glyph->height; ++yy) {
            for (uint8_t xx = 0; xx < glyph->width; ++xx) {
                if ((xx + yy * glyph->width) % 8U == 0U) {
                    bits = bitmap[byte_index++];
                }

                if (bits & 0x80U) {
                    drawScaledPixel(base_x + static_cast<int16_t>(xx) * _text_size, base_y + static_cast<int16_t>(yy) * _text_size, _text_color != SSD1306_BLACK);
                }

                bits <<= 1;
            }
        }

        _cursor_x += static_cast<int16_t>(glyph->xAdvance) * _text_size;
        if (_wrap && _cursor_x >= _width) {
            _cursor_x = 0;
            _cursor_y += lineAdvance();
        }

        return 1;
    }

private:
    const GFXglyph* lookupGlyph(uint8_t c) const {
        if (_font == nullptr || c < _font->first || c > _font->last) {
            return nullptr;
        }

        return &_font->glyph[c - _font->first];
    }

    bool _hasEmptyFont() const {
        return (_font == nullptr || _font->first > _font->last);
    }

    int16_t lineAdvance() const {
        if (_font != nullptr && !_hasEmptyFont()) {
            return static_cast<int16_t>(_font->yAdvance) * _text_size;
        }
        return static_cast<int16_t>(8 * _text_size);
    }

    void drawFallbackChar(char c) {
        if (c == ' ') {
            _cursor_x += static_cast<int16_t>(6 * _text_size);
            return;
        }

        const int16_t x = _cursor_x;
        const int16_t y = _cursor_y;
        for (int16_t yy = 0; yy < 7 * _text_size; ++yy) {
            for (int16_t xx = 0; xx < 5 * _text_size; ++xx) {
                const bool border = (xx == 0 || xx == 5 * _text_size - 1 || yy == 0 || yy == 7 * _text_size - 1);
                if (border) {
                    setPixelClipped(x + xx, y + yy, _text_color != SSD1306_BLACK);
                }
            }
        }

        _cursor_x += static_cast<int16_t>(6 * _text_size);
    }

    void drawScaledPixel(int16_t x, int16_t y, bool on) {
        for (uint8_t sy = 0; sy < _text_size; ++sy) {
            for (uint8_t sx = 0; sx < _text_size; ++sx) {
                setPixelClipped(x + sx, y + sy, on);
            }
        }
    }

    void setPixelClipped(int16_t x, int16_t y, bool on) {
        if (x < 0 || y < 0 || x >= _width || y >= _height) {
            return;
        }

        const size_t index = static_cast<size_t>(y) * static_cast<size_t>(_width) + static_cast<size_t>(x);
        _pixels[index] = on ? 1 : 0;
    }

    size_t appendString(const char* s) {
        if (s == nullptr) {
            return 0;
        }

        size_t written = 0;
        for (const char* p = s; *p != '\0'; ++p) {
            written += write(static_cast<uint8_t>(*p));
        }
        return written;
    }

private:
    std::vector<uint8_t> _pixels;
    int16_t _cursor_x{0};
    int16_t _cursor_y{0};
    const GFXfont* _font{nullptr};
    uint8_t _text_size{1};
    uint16_t _text_color{SSD1306_WHITE};
    bool _wrap{false};
};
