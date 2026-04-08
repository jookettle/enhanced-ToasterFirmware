#pragma once
#include "FreeRTOS.h"
#include <thread>

inline BaseType_t xTaskCreatePinnedToCore(void (*pvTaskCode)(void*), const char* pcName, uint32_t usStackDepth, void* pvParameters, BaseType_t uxPriority, TaskHandle_t* pxCreatedTask, BaseType_t xCoreID) {
  (void)pcName;
  (void)usStackDepth;
  (void)uxPriority;
  (void)xCoreID;

  if (pvTaskCode == nullptr) {
    if (pxCreatedTask != nullptr) {
      *pxCreatedTask = nullptr;
    }
    return pdFAIL;
  }

  std::thread task_thread([pvTaskCode, pvParameters]() {
    pvTaskCode(pvParameters);
  });
  task_thread.detach();

  if (pxCreatedTask != nullptr) {
    *pxCreatedTask = nullptr;
  }

  return pdPASS;
}

inline void vTaskDelete(TaskHandle_t xTaskToDelete) {
  (void)xTaskToDelete;
}
