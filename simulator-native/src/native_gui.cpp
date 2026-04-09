#include <windows.h>

#include "native_app.h"
#include "native_memory.h"
#include "native_scene.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <cstdio>
#include <string>
#include <vector>

using namespace toaster;

namespace {

constexpr UINT kFrameTimerId = 1;
constexpr UINT kFrameTimerMs = 33;
constexpr int kOuterMargin = 16;
constexpr int kCardGap = 12;
constexpr auto kCacheRefreshInterval = std::chrono::seconds(2);

HWND g_window = nullptr;
HFONT g_title_font = nullptr;
HFONT g_body_font = nullptr;
HFONT g_mono_font = nullptr;
HBRUSH g_bg_brush = nullptr;
HBRUSH g_card_brush = nullptr;
HBRUSH g_card_alt_brush = nullptr;
HBRUSH g_accent_brush = nullptr;
HPEN g_border_pen = nullptr;
HPEN g_accent_pen = nullptr;

void create_font(HFONT& out, const char* face, int height, int weight) {
  out = CreateFontA(
    height,
    0,
    0,
    0,
    weight,
    FALSE,
    FALSE,
    FALSE,
    ANSI_CHARSET,
    OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS,
    CLEARTYPE_QUALITY,
    FIXED_PITCH | FF_DONTCARE,
    face);
}

void ensure_resources() {
  if (g_title_font != nullptr) {
    return;
  }

  create_font(g_title_font, "Consolas", -20, FW_BOLD);
  create_font(g_body_font, "Consolas", -14, FW_NORMAL);
  create_font(g_mono_font, "Consolas", -13, FW_NORMAL);

  g_bg_brush = CreateSolidBrush(RGB(10, 12, 16));
  g_card_brush = CreateSolidBrush(RGB(14, 17, 22));
  g_card_alt_brush = CreateSolidBrush(RGB(16, 19, 24));
  g_accent_brush = CreateSolidBrush(RGB(90, 120, 150));
  g_border_pen = CreatePen(PS_SOLID, 1, RGB(58, 68, 78));
  g_accent_pen = CreatePen(PS_SOLID, 1, RGB(90, 120, 150));
}

void destroy_resources() {
  if (g_title_font != nullptr) {
    DeleteObject(g_title_font);
    g_title_font = nullptr;
  }
  if (g_body_font != nullptr) {
    DeleteObject(g_body_font);
    g_body_font = nullptr;
  }
  if (g_mono_font != nullptr) {
    DeleteObject(g_mono_font);
    g_mono_font = nullptr;
  }
  if (g_bg_brush != nullptr) {
    DeleteObject(g_bg_brush);
    g_bg_brush = nullptr;
  }
  if (g_card_brush != nullptr) {
    DeleteObject(g_card_brush);
    g_card_brush = nullptr;
  }
  if (g_card_alt_brush != nullptr) {
    DeleteObject(g_card_alt_brush);
    g_card_alt_brush = nullptr;
  }
  if (g_accent_brush != nullptr) {
    DeleteObject(g_accent_brush);
    g_accent_brush = nullptr;
  }
  if (g_border_pen != nullptr) {
    DeleteObject(g_border_pen);
    g_border_pen = nullptr;
  }
  if (g_accent_pen != nullptr) {
    DeleteObject(g_accent_pen);
    g_accent_pen = nullptr;
  }
}

struct FontSelect {
  HDC hdc;
  HGDIOBJ old;
  FontSelect(HDC in_hdc, HFONT font) : hdc(in_hdc), old(SelectObject(in_hdc, font)) {}
  ~FontSelect() {
    if (hdc != nullptr && old != nullptr) {
      SelectObject(hdc, old);
    }
  }
};

RECT inset_rect(RECT rect, int dx, int dy) {
  rect.left += dx;
  rect.top += dy;
  rect.right -= dx;
  rect.bottom -= dy;
  return rect;
}

RECT fit_rect(const RECT& outer, int src_w, int src_h, int padding = 0) {
  RECT inner = inset_rect(outer, padding, padding);
  const int inner_w = std::max(1, static_cast<int>(inner.right - inner.left));
  const int inner_h = std::max(1, static_cast<int>(inner.bottom - inner.top));
  if (src_w <= 0 || src_h <= 0) {
    return inner;
  }

  const double scale = std::min(
    static_cast<double>(inner_w) / static_cast<double>(src_w),
    static_cast<double>(inner_h) / static_cast<double>(src_h));
  const int draw_w = std::max(1, static_cast<int>(src_w * scale));
  const int draw_h = std::max(1, static_cast<int>(src_h * scale));
  const int x = inner.left + (inner_w - draw_w) / 2;
  const int y = inner.top + (inner_h - draw_h) / 2;

  return RECT{ x, y, x + draw_w, y + draw_h };
}

void fill_rect(HDC hdc, const RECT& rect, HBRUSH brush) {
  FillRect(hdc, &rect, brush);
}

void draw_round_card(HDC hdc, const RECT& rect, bool alt = false) {
  HBRUSH old_brush = static_cast<HBRUSH>(SelectObject(hdc, alt ? g_card_alt_brush : g_card_brush));
  HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, g_border_pen));
  Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
  SelectObject(hdc, old_pen);
  SelectObject(hdc, old_brush);
}

void draw_accent_line(HDC hdc, const RECT& rect) {
  HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, g_accent_pen));
  MoveToEx(hdc, rect.left + 16, rect.top + 34, nullptr);
  LineTo(hdc, rect.right - 16, rect.top + 34);
  SelectObject(hdc, old_pen);
}

void draw_text(HDC hdc, const RECT& rect, HFONT font, COLORREF color, const std::string& text, UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX) {
  FontSelect select(hdc, font);
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, color);
  RECT copy = rect;
  DrawTextA(hdc, text.c_str(), -1, &copy, format);
}

std::string format_bytes(uintmax_t bytes) {
  static const char* kUnits[] = { "B", "KB", "MB", "GB", "TB" };
  double value = static_cast<double>(bytes);
  size_t unit = 0;
  while (value >= 1024.0 && unit + 1 < (sizeof(kUnits) / sizeof(kUnits[0]))) {
    value /= 1024.0;
    ++unit;
  }

  char buffer[64];
  if (unit == 0) {
    std::snprintf(buffer, sizeof(buffer), "%.0f %s", value, kUnits[unit]);
  }
  else {
    std::snprintf(buffer, sizeof(buffer), "%.1f %s", value, kUnits[unit]);
  }
  return buffer;
}

std::string format_usage(uintmax_t used, uintmax_t total) {
  return format_bytes(used) + " / " + format_bytes(total);
}

struct CacheStats {
  uintmax_t bytes{0};
  size_t files{0};
  bool valid{false};
};

CacheStats scan_cache_stats() {
  CacheStats stats{};
  const std::filesystem::path root = std::filesystem::current_path() / "data";
  std::error_code ec;

  if (!std::filesystem::exists(root, ec)) {
    return stats;
  }

  for (std::filesystem::recursive_directory_iterator it(
         root,
         std::filesystem::directory_options::skip_permission_denied,
         ec), end;
       it != end;
       it.increment(ec)) {
    if (ec) {
      ec.clear();
      continue;
    }

    const auto& entry = *it;
    ec.clear();
    if (!entry.is_regular_file(ec)) {
      continue;
    }

    const uintmax_t file_size = entry.file_size(ec);
    if (ec) {
      ec.clear();
      continue;
    }

    stats.bytes += file_size;
    ++stats.files;
  }

  stats.valid = true;
  return stats;
}

CacheStats query_cache_stats() {
  using clock = std::chrono::steady_clock;
  static CacheStats cached{};
  static clock::time_point last_scan{};

  const auto now = clock::now();
  if (!cached.valid || last_scan.time_since_epoch().count() == 0 || (now - last_scan) >= kCacheRefreshInterval) {
    cached = scan_cache_stats();
    last_scan = now;
  }

  return cached;
}

void draw_rgb_buffer(HDC hdc, const RECT& dst, const uint8_t* pixels, size_t src_w, size_t src_h) {
  if (pixels == nullptr || src_w == 0 || src_h == 0) {
    return;
  }

  std::vector<uint32_t> converted(src_w * src_h, 0);
  for (size_t i = 0; i < src_w * src_h; ++i) {
    const uint8_t r = pixels[i * 3 + 0];
    const uint8_t g = pixels[i * 3 + 1];
    const uint8_t b = pixels[i * 3 + 2];
    converted[i] = static_cast<uint32_t>(b)
      | (static_cast<uint32_t>(g) << 8)
      | (static_cast<uint32_t>(r) << 16);
  }

  BITMAPINFO bmi{};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = static_cast<LONG>(src_w);
  bmi.bmiHeader.biHeight = -static_cast<LONG>(src_h);
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  SetStretchBltMode(hdc, COLORONCOLOR);
  StretchDIBits(
    hdc,
    dst.left,
    dst.top,
    std::max(1, static_cast<int>(dst.right - dst.left)),
    std::max(1, static_cast<int>(dst.bottom - dst.top)),
    0,
    0,
    static_cast<int>(src_w),
    static_cast<int>(src_h),
    converted.data(),
    &bmi,
    DIB_RGB_COLORS,
    SRCCOPY);
}

void draw_mono_buffer(HDC hdc, const RECT& dst, const uint8_t* pixels, size_t src_w, size_t src_h) {
  if (pixels == nullptr || src_w == 0 || src_h == 0) {
    return;
  }

  std::vector<uint32_t> converted(src_w * src_h, 0);
  for (size_t i = 0; i < src_w * src_h; ++i) {
    const bool on = pixels[i] != 0;
    converted[i] = on ? RGB(238, 244, 250) : RGB(7, 8, 12);
  }

  BITMAPINFO bmi{};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = static_cast<LONG>(src_w);
  bmi.bmiHeader.biHeight = -static_cast<LONG>(src_h);
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  SetStretchBltMode(hdc, COLORONCOLOR);
  StretchDIBits(
    hdc,
    dst.left,
    dst.top,
    std::max(1, static_cast<int>(dst.right - dst.left)),
    std::max(1, static_cast<int>(dst.bottom - dst.top)),
    0,
    0,
    static_cast<int>(src_w),
    static_cast<int>(src_h),
    converted.data(),
    &bmi,
    DIB_RGB_COLORS,
    SRCCOPY);
}

void draw_lines(HDC hdc, RECT rect, HFONT font, COLORREF color, const std::vector<std::string>& lines, int max_lines) {
  FontSelect select(hdc, font);
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, color);

  TEXTMETRICA metrics{};
  GetTextMetricsA(hdc, &metrics);
  const int line_h = std::max(1, static_cast<int>(metrics.tmHeight + metrics.tmExternalLeading + 2));
  const int start = std::max(0, static_cast<int>(lines.size()) - max_lines);
  int y = rect.top;

  for (int i = start; i < static_cast<int>(lines.size()); ++i) {
    RECT line_rect = rect;
    line_rect.top = y;
    line_rect.bottom = y + line_h;
    if (line_rect.top >= rect.bottom) {
      break;
    }
    DrawTextA(
      hdc,
      lines[static_cast<size_t>(i)].c_str(),
      -1,
      &line_rect,
      DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += line_h;
  }
}

void draw_header(HDC hdc, const RECT& client, const toaster::SimulatorSnapshot& snap) {
  RECT header = { client.left, client.top, client.right, client.top + 64 };
  fill_rect(hdc, header, g_bg_brush);

  RECT title_rect = inset_rect(header, kOuterMargin, 10);
  title_rect.bottom = title_rect.top + 26;
  draw_text(hdc, title_rect, g_title_font, RGB(228, 232, 236), "Protogen Native Emulator");

  RECT subtitle_rect = title_rect;
  subtitle_rect.top = title_rect.bottom + 2;
  subtitle_rect.bottom = subtitle_rect.top + 18;
  draw_text(
    hdc,
    subtitle_rect,
    g_body_font,
    RGB(146, 154, 164),
    "q/Esc quit | 1-9 emotions | F/J menu | D/K static | Tab next | arrows/Enter");

  RECT status_rect = title_rect;
  status_rect.left = client.right - 220;
  status_rect.right = client.right - kOuterMargin;
  draw_text(
    hdc,
    status_rect,
    g_body_font,
    RGB(160, 173, 184),
    native_is_running() ? "RUNNING" : "STOPPING",
    DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void draw_panel_title(HDC hdc, const RECT& rect, const char* title, const char* subtitle, bool alt = false) {
  RECT title_rect = rect;
  title_rect.top += 10;
  title_rect.left += 14;
  title_rect.right -= 14;
  title_rect.bottom = title_rect.top + 20;
  draw_text(hdc, title_rect, g_body_font, RGB(228, 232, 236), title);

  RECT subtitle_rect = title_rect;
  subtitle_rect.top += 18;
  subtitle_rect.bottom = subtitle_rect.top + 16;
  draw_text(hdc, subtitle_rect, g_mono_font, RGB(132, 140, 148), subtitle);

  HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, g_border_pen));
  MoveToEx(hdc, rect.left + 1, rect.top + 38, nullptr);
  LineTo(hdc, rect.right - 1, rect.top + 38);
  SelectObject(hdc, old_pen);
}

void draw_controls_card(HDC hdc, const RECT& rect) {
  RECT inner = inset_rect(rect, 14, 44);
  std::vector<std::string> lines = {
    "q / Esc   quit         1 - 9   emotions",
    "Tab       next         F / J   open menu",
    "D / K     static       S / L   shortcuts",
    "Arrows    nav          Enter   select",
    "Z X C V B colors       \\       dithering",
  };
  draw_lines(hdc, inner, g_mono_font, RGB(201, 208, 216), lines, 12);
}

void draw_status_card(HDC hdc, const RECT& rect, const toaster::SimulatorSnapshot& snap) {
  RECT inner = inset_rect(rect, 14, 44);
  std::vector<std::string> lines;
  const NativeMemoryStats memory = native_query_memory_stats();
  const CacheStats cache = query_cache_stats();
  lines.push_back(std::string("Emulation: ") + (native_is_running() ? "active" : "stopping"));
  lines.push_back(
    std::string("Outputs: HUB ")
    + std::to_string(snap.hub_width) + "x" + std::to_string(snap.hub_height)
    + " / OLED " + std::to_string(snap.oled_width) + "x" + std::to_string(snap.oled_height)
    + " / Side " + (snap.side_enabled ? "enabled" : "disabled"));
  lines.push_back(
    memory.valid
      ? std::string("Working set: ") + format_bytes(memory.working_set)
      : std::string("Working set: unavailable"));
  lines.push_back(
    memory.valid
      ? std::string("Private bytes: ") + format_bytes(memory.private_bytes)
      : std::string("Private bytes: unavailable"));
  lines.push_back(
    cache.valid
      ? std::string("Cache used: ") + format_bytes(cache.bytes) + " in data/ (" + std::to_string(cache.files) + " files)"
      : std::string("Cache used: unavailable"));
  if (!snap.status_line.empty()) {
    lines.push_back("Last: " + snap.status_line);
  }
  draw_lines(hdc, inner, g_mono_font, RGB(201, 208, 216), lines, 8);
}

void draw_log_card(HDC hdc, const RECT& rect, const toaster::SimulatorSnapshot& snap) {
  RECT inner = inset_rect(rect, 14, 46);
  draw_lines(hdc, inner, g_mono_font, RGB(205, 211, 218), snap.log_lines, 18);
}

void draw_render_card(HDC hdc, const RECT& rect, const toaster::SimulatorSnapshot& snap, const char* title, const char* subtitle) {
  draw_round_card(hdc, rect, false);
  draw_panel_title(hdc, rect, title, subtitle, true);

  RECT inner = inset_rect(rect, 16, 44);
  if (inner.right <= inner.left || inner.bottom <= inner.top) {
    return;
  }

  if (std::string(title) == "HUB75") {
    if (snap.hub_pixels.empty()) {
      draw_text(hdc, inner, g_mono_font, RGB(160, 170, 184), "Waiting for HUB75 frame...");
      return;
    }
    const RECT dst = fit_rect(inner, static_cast<int>(snap.hub_width), static_cast<int>(snap.hub_height), 4);
    draw_rgb_buffer(hdc, dst, snap.hub_pixels.data(), snap.hub_width, snap.hub_height);
    return;
  }

  if (std::string(title) == "OLED") {
    if (snap.oled_pixels.empty()) {
      draw_text(hdc, inner, g_mono_font, RGB(160, 170, 184), "Waiting for OLED frame...");
      return;
    }
    const RECT dst = fit_rect(inner, static_cast<int>(snap.oled_width), static_cast<int>(snap.oled_height), 6);
    draw_mono_buffer(hdc, dst, snap.oled_pixels.data(), snap.oled_width, snap.oled_height);
    return;
  }

  if (std::string(title) == "SIDE PANEL") {
    if (!snap.side_enabled || snap.side_pixels.empty()) {
      draw_text(hdc, inner, g_mono_font, RGB(160, 170, 184), "Side panel disabled");
      return;
    }
    const RECT dst = fit_rect(inner, static_cast<int>(snap.side_width), static_cast<int>(snap.side_height), 4);
    draw_rgb_buffer(hdc, dst, snap.side_pixels.data(), snap.side_width, snap.side_height);
  }
}

void render_scene(HDC hdc, const RECT& client, const toaster::SimulatorSnapshot& snap) {
  fill_rect(hdc, client, g_bg_brush);
  draw_header(hdc, client, snap);

  const int content_top = 82;
  const int log_height = std::max(190, static_cast<int>((client.bottom - client.top) / 4));
  const int content_bottom = client.bottom - log_height - kOuterMargin;
  const int content_height = std::max(0, content_bottom - content_top);
  const int left_w = std::max(640, static_cast<int>(((client.right - client.left) - (kOuterMargin * 3)) * 68 / 100));
  const int right_w = std::max(280, static_cast<int>((client.right - client.left) - left_w - (kOuterMargin * 3)));
  const int left_x = kOuterMargin;
  const int right_x = left_x + left_w + kOuterMargin;
  const int mid_gap = kCardGap;

  int left_top_h = std::max(170, content_height * 36 / 100);
  int left_bottom_h = std::max(220, content_height - left_top_h - mid_gap);
  if (left_top_h + left_bottom_h + mid_gap > content_height) {
    const int available = std::max(0, content_height - mid_gap);
    left_top_h = std::max(150, available * 36 / 100);
    left_bottom_h = std::max(180, available - left_top_h);
  }
  RECT hub_card = { left_x, content_top, left_x + left_w, content_top + left_top_h };
  RECT oled_card = { left_x, hub_card.bottom + mid_gap, left_x + left_w, hub_card.bottom + mid_gap + left_bottom_h };

  int right_top_h = std::max(120, content_height * 27 / 100);
  int right_mid_h = std::max(130, content_height * 31 / 100);
  int right_bottom_h = content_height - right_top_h - right_mid_h - (mid_gap * 2);
  if (right_bottom_h < 100) {
    const int available = std::max(0, content_height - (mid_gap * 2));
    right_top_h = std::max(110, available * 27 / 100);
    right_mid_h = std::max(120, available * 31 / 100);
    right_bottom_h = std::max(80, available - right_top_h - right_mid_h);
  }
  RECT side_card = { right_x, content_top, right_x + right_w, content_top + right_top_h };
  RECT controls_card = { right_x, side_card.bottom + mid_gap, right_x + right_w, side_card.bottom + mid_gap + right_mid_h };
  RECT status_card = { right_x, controls_card.bottom + mid_gap, right_x + right_w, controls_card.bottom + mid_gap + right_bottom_h };

  draw_render_card(hdc, hub_card, snap, "HUB75", "Main display output");
  draw_render_card(hdc, oled_card, snap, "OLED", "HUD and menus");
  draw_render_card(hdc, side_card, snap, "SIDE PANEL", "NeoPixel strip");

  draw_round_card(hdc, controls_card, true);
  draw_panel_title(hdc, controls_card, "Controls", "Keyboard shortcuts", true);
  draw_controls_card(hdc, controls_card);

  draw_round_card(hdc, status_card, false);
  draw_panel_title(hdc, status_card, "Status", "Live simulator state", false);
  draw_status_card(hdc, status_card, snap);

  RECT log_card = { kOuterMargin, client.bottom - log_height, client.right - kOuterMargin, client.bottom - kOuterMargin };
  draw_round_card(hdc, log_card, false);
  draw_panel_title(hdc, log_card, "Log", "Recent simulator output", true);
  draw_log_card(hdc, log_card, snap);
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_CREATE:
    ensure_resources();
    g_window = hwnd;
    SetTimer(hwnd, kFrameTimerId, kFrameTimerMs, nullptr);
    native_start_simulation();
    return 0;

  case WM_TIMER:
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;

  case WM_PAINT: {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT client{};
    GetClientRect(hwnd, &client);
    const int width = std::max(1, static_cast<int>(client.right - client.left));
    const int height = std::max(1, static_cast<int>(client.bottom - client.top));

    HDC mem_dc = CreateCompatibleDC(hdc);
    HBITMAP mem_bmp = CreateCompatibleBitmap(hdc, width, height);
    if (mem_dc == nullptr || mem_bmp == nullptr) {
      if (mem_bmp != nullptr) {
        DeleteObject(mem_bmp);
      }
      if (mem_dc != nullptr) {
        DeleteDC(mem_dc);
      }
      EndPaint(hwnd, &ps);
      return 0;
    }
    HGDIOBJ old_bmp = SelectObject(mem_dc, mem_bmp);

    SetBkMode(mem_dc, TRANSPARENT);

    toaster::SimulatorSnapshot snap;
    simulator_copy_snapshot(snap);
    render_scene(mem_dc, client, snap);

    BitBlt(hdc, 0, 0, width, height, mem_dc, 0, 0, SRCCOPY);

    SelectObject(mem_dc, old_bmp);
    DeleteObject(mem_bmp);
    DeleteDC(mem_dc);
    EndPaint(hwnd, &ps);
    return 0;
  }

  case WM_KEYDOWN:
    switch (wParam) {
    case VK_ESCAPE:
      DestroyWindow(hwnd);
      return 0;
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
      native_enqueue_key(static_cast<int>(wParam), 1);
      return 0;
    case VK_RETURN:
      native_enqueue_key(VK_RIGHT, 1);
      return 0;
    case VK_TAB:
      native_enqueue_key('\t', 0);
      return 0;
    default:
      break;
    }
    break;

  case WM_CHAR: {
    const int ch = static_cast<int>(wParam);
    if (ch == 'q' || ch == 'Q') {
      DestroyWindow(hwnd);
      return 0;
    }
    if (ch >= 32 || ch == '\t' || ch == '\'' || ch == '\\') {
      native_enqueue_key(ch, 0);
      return 0;
    }
    break;
  }

  case WM_ERASEBKGND:
    return 1;

  case WM_GETMINMAXINFO: {
    auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
    info->ptMinTrackSize.x = 1100;
    info->ptMinTrackSize.y = 760;
    return 0;
  }

  case WM_CLOSE:
    native_stop_simulation();
    DestroyWindow(hwnd);
    return 0;

  case WM_DESTROY:
    KillTimer(hwnd, kFrameTimerId);
    native_stop_simulation();
    g_window = nullptr;
    destroy_resources();
    PostQuitMessage(0);
    return 0;

  default:
    break;
  }

  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int run_native_gui(HINSTANCE hInstance) {
  const char* class_name = "ProtogenNativeEmulatorWindow";

  WNDCLASSA wc{};
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = window_proc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  wc.lpszClassName = class_name;

  if (!RegisterClassA(&wc)) {
    return 1;
  }

  RECT window_rect = { 0, 0, 1480, 980 };
  AdjustWindowRectEx(&window_rect, WS_OVERLAPPEDWINDOW, FALSE, 0);

  HWND hwnd = CreateWindowExA(
    0,
    class_name,
    "Protogen Native Emulator",
    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    window_rect.right - window_rect.left,
    window_rect.bottom - window_rect.top,
    nullptr,
    nullptr,
    hInstance,
    nullptr);

  if (hwnd == nullptr) {
    return 1;
  }

  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  MSG msg{};
  int msg_result = 0;
  while ((msg_result = GetMessageA(&msg, nullptr, 0, 0)) > 0) {
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  native_wait_for_simulation();
  if (msg_result < 0) {
    return 1;
  }
  return static_cast<int>(msg.wParam);
}

}

namespace toaster {
void native_request_gui_close() {
  if (g_window != nullptr) {
    PostMessageA(g_window, WM_CLOSE, 0, 0);
  }
}
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  return run_native_gui(hInstance);
}
#endif
