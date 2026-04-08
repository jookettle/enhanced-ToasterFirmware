#include "hal/display/hub75_display.h"
#include "hal/display/neopixel_display.h"
#include "native_scene.h"

#include <algorithm>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace toaster {

namespace {

enum class AnsiState {
  None,
  Esc,
  Csi,
};

struct SceneState {
  std::mutex mutex;

  std::vector<uint8_t> hub_pixels;
  size_t hub_width{0};
  size_t hub_height{0};

  std::vector<uint8_t> side_pixels;
  size_t side_width{0};
  size_t side_height{0};
  bool side_enabled{false};

  std::vector<uint8_t> oled_pixels;
  size_t oled_width{0};
  size_t oled_height{0};

  std::deque<std::string> log_lines;
  std::string current_line;
  std::string status_line;
  AnsiState ansi_state{AnsiState::None};
};

SceneState g_scene;
constexpr size_t kMaxLogLines = 128;

void push_log_line_locked() {
  g_scene.status_line = g_scene.current_line;
  g_scene.log_lines.push_back(g_scene.current_line);
  if (g_scene.log_lines.size() > kMaxLogLines) {
    g_scene.log_lines.pop_front();
  }
  g_scene.current_line.clear();
}

void copy_rgb_pixels(std::vector<uint8_t>& dst, const uint8_t* src, size_t width, size_t height) {
  const size_t size = width * height * 3;
  if (src == nullptr || size == 0) {
    dst.clear();
    return;
  }

  dst.assign(src, src + size);
}

void copy_mono_pixels(std::vector<uint8_t>& dst, const uint8_t* src, size_t width, size_t height) {
  const size_t size = width * height;
  if (src == nullptr || size == 0) {
    dst.clear();
    return;
  }

  dst.assign(src, src + size);
}

}

void simulator_log_append(const std::string& text) {
  std::lock_guard<std::mutex> lock(g_scene.mutex);

  for (char ch : text) {
    switch (g_scene.ansi_state) {
    case AnsiState::None:
      if (ch == '\x1b') {
        g_scene.ansi_state = AnsiState::Esc;
        continue;
      }
      break;
    case AnsiState::Esc:
      if (ch == '[') {
        g_scene.ansi_state = AnsiState::Csi;
        continue;
      }
      g_scene.ansi_state = AnsiState::None;
      break;
    case AnsiState::Csi:
      if (ch >= '@' && ch <= '~') {
        g_scene.ansi_state = AnsiState::None;
      }
      continue;
    }

    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      push_log_line_locked();
      continue;
    }

    g_scene.current_line.push_back(ch);
    if (g_scene.current_line.size() > 512) {
      g_scene.current_line.erase(0, g_scene.current_line.size() - 512);
    }
  }
}

void simulator_set_oled_pixels(const uint8_t* pixels, size_t width, size_t height) {
  std::lock_guard<std::mutex> lock(g_scene.mutex);
  g_scene.oled_width = width;
  g_scene.oled_height = height;
  copy_mono_pixels(g_scene.oled_pixels, pixels, width, height);
}

bool simulator_copy_snapshot(SimulatorSnapshot& out) {
  std::lock_guard<std::mutex> lock(g_scene.mutex);

  out.hub_pixels = g_scene.hub_pixels;
  out.hub_width = g_scene.hub_width;
  out.hub_height = g_scene.hub_height;

  out.side_pixels = g_scene.side_pixels;
  out.side_width = g_scene.side_width;
  out.side_height = g_scene.side_height;
  out.side_enabled = g_scene.side_enabled;

  out.oled_pixels = g_scene.oled_pixels;
  out.oled_width = g_scene.oled_width;
  out.oled_height = g_scene.oled_height;

  out.log_lines.assign(g_scene.log_lines.begin(), g_scene.log_lines.end());
  out.status_line = !g_scene.current_line.empty() ? g_scene.current_line : g_scene.status_line;

  return out.hub_width > 0 || out.side_width > 0 || out.oled_width > 0 || !out.log_lines.empty() || !out.status_line.empty();
}

void simulator_render_frame() {
  // GUI mode repaints from a timer, so there is nothing to do here.
}

// Hub75Display Simulation
Hub75Display::Hub75Display() {}

bool Hub75Display::begin(int width, int height, int chain) {
  _chain = chain;
  _panel_width = width;
  return Display::begin(width * chain, height);
}

bool Hub75Display::begin(int width, int height, int chain, HUB75_I2S_CFG::i2s_pins pins, uint16_t min_refresh_rate) {
  (void)pins;
  (void)min_refresh_rate;
  return begin(width, height, chain);
}

void Hub75Display::beginDraw() {
  Display::beginDraw();
}

void Hub75Display::endDraw() {
  {
    std::lock_guard<std::mutex> lock(g_scene.mutex);
    g_scene.hub_width = _width;
    g_scene.hub_height = _height;
    copy_rgb_pixels(g_scene.hub_pixels, _buffer, _width, _height);
  }

  Display::endDraw();
}

bool Hub75Display::draw_image_newcolor(const Image* image, COLOR_FUNC color_func, uint8_t param, DRAW_MODE draw_mode, int offset_x, int offset_y, int rotate_cw) {
  return Display::draw_image_newcolor(image, color_func, param, draw_mode, offset_x, offset_y, rotate_cw);
}

bool Hub75Display::draw_image_newcolor_ex(const Image* image, COLOR_FUNC color_func, uint8_t param, DRAW_MODE draw_mode, int offset_x, int offset_y, int w, int h, int source_x, int source_y, int rotate_cw) {
  return Display::draw_image_newcolor_ex(image, color_func, param, draw_mode, offset_x, offset_y, w, h, source_x, source_y, rotate_cw);
}

// NeoPixelDisplay Simulation
NeoPixelDisplay::NeoPixelDisplay() {}

bool NeoPixelDisplay::begin(int numleds, int pin, bool reversed) {
  if (!Display::begin(numleds, 1)) {
    return false;
  }

  _reversed = reversed;
  (void)pin;

  {
    std::lock_guard<std::mutex> lock(g_scene.mutex);
    g_scene.side_enabled = true;
    g_scene.side_width = _width;
    g_scene.side_height = _height;
    copy_rgb_pixels(g_scene.side_pixels, _buffer, _width, _height);
  }

  return true;
}

void NeoPixelDisplay::beginDraw() {
  Display::beginDraw();
}

void NeoPixelDisplay::endDraw() {
  {
    std::lock_guard<std::mutex> lock(g_scene.mutex);
    g_scene.side_width = _width;
    g_scene.side_height = _height;
    g_scene.side_enabled = true;
    copy_rgb_pixels(g_scene.side_pixels, _buffer, _width, _height);
  }

  Display::endDraw();
}

}
