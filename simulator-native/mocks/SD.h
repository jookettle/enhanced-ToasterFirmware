#pragma once
#include "Arduino.h"

namespace fs {

class FS {
public:
  FS()
    : _root(std::filesystem::current_path() / "data") {
  }

  File open(const char* path, const char* mode = "r") {
    (void)mode;
    auto full_path = nativefs::normalize_path(_root, path);
    std::error_code ec;

    if (std::filesystem::is_directory(full_path, ec)) {
      auto state = std::make_shared<nativefs::State>();
      state->root = _root;
      state->path = full_path;
      state->kind = nativefs::State::Kind::Directory;

      std::vector<std::filesystem::path> entries;
      for (const auto& entry : std::filesystem::directory_iterator(full_path, ec)) {
        entries.push_back(entry.path());
      }
      std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.generic_string() < b.generic_string();
      });
      state->entries = std::move(entries);
      return File::fromState(std::move(state));
    }

    if (!std::filesystem::is_regular_file(full_path, ec)) {
      return File();
    }

    auto stream = std::make_shared<std::ifstream>(full_path, std::ios::binary);
    if (stream == nullptr || !stream->is_open()) {
      return File();
    }

    auto state = std::make_shared<nativefs::State>();
    state->root = _root;
    state->path = full_path;
    state->kind = nativefs::State::Kind::File;
    state->stream = stream;
    state->size = static_cast<size_t>(std::filesystem::file_size(full_path, ec));
    state->pos = 0;
    return File::fromState(std::move(state));
  }

  bool exists(const char* path) {
    auto full_path = nativefs::normalize_path(_root, path);
    std::error_code ec;
    return std::filesystem::exists(full_path, ec);
  }

  bool mkdir(const char* path) {
    auto full_path = nativefs::normalize_path(_root, path);
    std::error_code ec;
    return std::filesystem::create_directories(full_path, ec);
  }

  bool remove(const char* path) {
    auto full_path = nativefs::normalize_path(_root, path);
    std::error_code ec;
    return std::filesystem::remove(full_path, ec);
  }

protected:
  std::filesystem::path _root;
};

class SDClass : public FS {
public:
  bool begin(int ssPin = -1, ...) { (void)ssPin; return true; }
  int cardType() const { return 0; }
  uint64_t cardSize() const { return 0; }
};
}
extern fs::SDClass SD;
