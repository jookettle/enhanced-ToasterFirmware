#pragma once
#include <Arduino.h>

namespace toaster {

void* malloc_auto(size_t size);
void free_auto(void* ptr);

};  // namespace toaster
