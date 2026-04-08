#pragma once
#include "FreeRTOS.h"
#include <mutex>
#include <chrono>
#include <thread>

enum class NativeSemaphoreKind {
  RecursiveMutex,
  Binary,
};

struct NativeSemaphore {
  explicit NativeSemaphore(NativeSemaphoreKind semaphore_kind) : kind(semaphore_kind) {}

  NativeSemaphoreKind kind;
  std::recursive_mutex mutex;
  bool available{false};
};

inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() {
  return new NativeSemaphore(NativeSemaphoreKind::RecursiveMutex);
}

inline SemaphoreHandle_t xSemaphoreCreateBinary() {
  return new NativeSemaphore(NativeSemaphoreKind::Binary);
}

inline BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t xMutex, TickType_t xBlockTime) {
  (void)xBlockTime;
  if (xMutex == nullptr || xMutex->kind != NativeSemaphoreKind::RecursiveMutex) return pdFALSE;
  xMutex->mutex.lock();
  return pdTRUE;
}

inline BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t xMutex) {
  if (xMutex == nullptr || xMutex->kind != NativeSemaphoreKind::RecursiveMutex) return pdFALSE;
  xMutex->mutex.unlock();
  return pdTRUE;
}

inline BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime) {
  if (xSemaphore == nullptr || xSemaphore->kind != NativeSemaphoreKind::Binary) return pdFALSE;

  const auto started_at = std::chrono::steady_clock::now();

  while (true) {
    {
      std::lock_guard<std::recursive_mutex> lock(xSemaphore->mutex);
      if (xSemaphore->available) {
        xSemaphore->available = false;
        return pdTRUE;
      }
    }

    if (xBlockTime == 0) return pdFALSE;

    if (xBlockTime != portMAX_DELAY) {
      const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started_at
      ).count();
      if (elapsed_ms >= static_cast<long long>(xBlockTime) * portTICK_PERIOD_MS) {
        return pdFALSE;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

inline BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
  if (xSemaphore == nullptr || xSemaphore->kind != NativeSemaphoreKind::Binary) return pdFALSE;

  std::lock_guard<std::recursive_mutex> lock(xSemaphore->mutex);
  xSemaphore->available = true;
  return pdTRUE;
}

inline void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) {
  delete xSemaphore;
}
