#pragma once
#include <stdint.h>
#include <stddef.h>
#include <chrono>
#include <thread>

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdFAIL 0

struct NativeSemaphore;

typedef void* TaskHandle_t;
typedef NativeSemaphore* SemaphoreHandle_t;
typedef uint32_t TickType_t;
typedef uint32_t BaseType_t;

#define portMAX_DELAY (TickType_t)0xffffffffUL
#define portTICK_PERIOD_MS 1

inline void vTaskDelay(TickType_t ticks) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ticks * portTICK_PERIOD_MS));
}
