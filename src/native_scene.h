#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace toaster {

struct SimulatorSnapshot {
  std::vector<uint8_t> hub_pixels;
  size_t hub_width{0};
  size_t hub_height{0};

  std::vector<uint8_t> side_pixels;
  size_t side_width{0};
  size_t side_height{0};

  std::vector<uint8_t> oled_pixels;
  size_t oled_width{0};
  size_t oled_height{0};

  std::vector<std::string> log_lines;
  std::string status_line;
  bool side_enabled{false};
};

void simulator_log_append(const std::string& text);
void simulator_set_oled_pixels(const uint8_t* pixels, size_t width, size_t height);
bool simulator_copy_snapshot(SimulatorSnapshot& out);
void simulator_render_frame();

}
