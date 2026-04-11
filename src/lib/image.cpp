#include "image.h"

#include <JPEGDEC.h>

#include "logger.h"
#include "ram.h"
#ifdef USE_SD
#include "SD.h"
#endif

namespace toaster {

static const char* TAG = "Image";

bool Image::_load_png(void* ptr, bool rgb565) {
  TF_LOGW(TAG, "PNG decoding disabled at build-time; _load_png() unsupported.");
  (void)ptr;
  (void)rgb565;
  return false;
}

Image::Image(const char* path, bool from_sd, bool rgb565) {
  _type = IMAGE_NONE;
  if (path == nullptr) return;

  const char* ext = strrchr(path, '.');
  if (ext == nullptr) return;

  if (strcasecmp(ext + 1, "png") == 0) {
    _type = IMAGE_PNG;
    load_png(path, from_sd, rgb565);
  } else if (strcasecmp(ext + 1, "jpg") == 0 || strcasecmp(ext + 1, "jpeg") == 0) {
    _type = IMAGE_JPEG;
    load_jpeg(path, from_sd);
  }
}

Image::Image(uint8_t type, const uint8_t* buffer, size_t size, bool rgb565) {
  _type = type;
  if (type == IMAGE_PNG) {
    load_png_from_bytes(buffer, size, rgb565);
  } else if (type == IMAGE_JPEG) {
    load_jpeg_from_bytes(buffer, size);
  }
}

Image::Image(size_t width, size_t height, uint8_t bpp, uint8_t has_alpha) {
  _width = width;
  _height = height;
  _bpp = bpp;
  _has_alpha = has_alpha;
  _size = _width * _height * (_bpp + _has_alpha);
  _buffer = (uint8_t*) malloc_auto(_size);
}

Image::~Image() {
  release();
}

void Image::release() {
  if (_buffer != nullptr) {
    free_auto(_buffer);
    _buffer = nullptr;
  }
  _width = 0;
  _height = 0;
  _bpp = 0;
  _has_alpha = 0;
  _size = 0;
  _type = IMAGE_NONE;
}

bool Image::isLoaded() {
  return _buffer != nullptr;
}

bool Image::load_png_from_bytes(const uint8_t* buffer, size_t size, bool rgb565) {
  TF_LOGW(TAG, "PNG decoding disabled; load_png_from_bytes() unsupported.");
  return false;
}

bool Image::load_png(const char* path, bool from_sd, bool rgb565) {
  TF_LOGW(TAG, "PNG decoding disabled; load_png(%s) unsupported.", path);
  return false;
}

bool Image::load_jpeg_from_bytes(const uint8_t* buffer, size_t size) {
  release();

  auto jpegdec = (JPEGDEC*) malloc_auto(sizeof(JPEGDEC));
  if (jpegdec == nullptr) {
    TF_LOGE(TAG, "JPEGDEC initialization failed.");
    return false;
  }

  if (!jpegdec->openRAM((uint8_t*) buffer, size, [](JPEGDRAW* draw) -> int {
        auto image = ((Image*) draw->pUser);

        if (draw->iBpp != 16) {
          return 0;
        }

        uint16_t* ptr_in = draw->pPixels;
        uint16_t* ptr_out = (uint16_t*) image->_buffer;

        for (int y = 0; y < draw->iHeight; y++) {
          int index_base = (y + draw->y) * image->_width + draw->x;
          for (int x = 0; x < draw->iWidthUsed; x++) {
            ptr_out[index_base + x] = *ptr_in++;
          }

          ptr_in += (draw->iWidth - draw->iWidthUsed);
        }

        return 1;
      })) {
    TF_LOGE(TAG, "JPEGDEC openRAM failed (%d).", jpegdec->getLastError());
    jpegdec->close();
    free_auto(jpegdec);
    return false;
  }

  _width = jpegdec->getWidth();
  _height = jpegdec->getHeight();
  _bpp = 2;
  _has_alpha = 0;
  _size = _width * _height * (_bpp + _has_alpha);

  _buffer = (uint8_t*) malloc_auto(_size);
  if (_buffer == nullptr) {
    TF_LOGE(TAG, "jpeg memory allocation failed.");
    jpegdec->close();
    free_auto(jpegdec);
    return false;
  }

  // TF_LOGD(TAG, "subsample: 0x%02x", jpegdec->getSubSample());

  jpegdec->setUserPointer(this);
  jpegdec->setPixelType(RGB565_LITTLE_ENDIAN);
  if (!jpegdec->decode(0, 0, 0)) {
    TF_LOGE(TAG, "JPEGDEC decode failed (%d).", jpegdec->getLastError());
    release();
    jpegdec->close();
    free_auto(jpegdec);
    return false;
  }

  jpegdec->close();
  free_auto(jpegdec);

  return true;
}

bool Image::load_jpeg(const char* path, bool from_sd) {
  File file;
  if (from_sd) {
#ifdef USE_SD
    file = SD.open(path);
#endif
  } else {
    file = FFat.open(path);
  }
  if (!file) {
    TF_LOGE(TAG, "jpeg load failed (%s).", path);
    return false;
  }

  size_t size = file.size();
  uint8_t* buffer = (uint8_t*) malloc_auto(size);
  if (buffer == nullptr) {
    TF_LOGE(TAG, "jpeg memory allocation failed (%s).", path);
    file.close();
    return false;
  }
  file.read(buffer, size);
  file.close();

  bool result = load_jpeg_from_bytes(buffer, size);
  free_auto(buffer);
  return result;
}

}  // namespace toaster
