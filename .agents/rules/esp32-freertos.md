---
trigger: always_on
glob: "**/*.{cpp,h,hpp}"
description: Rules for multi-threading and task management on ESP32.
---

# ESP32 FreeRTOS & Concurrency

The `enhanced-ToasterFirmware` utilizes the ESP32's dual-core architecture. Follow these rules to ensure thread safety and optimal task management.

## 1. Task Management
- **Core Affinity**: Use `xTaskCreatePinnedToCore` to specify which core a task should run on. Typically, the main application runs on Core 1 (`APP_CPU_NUM`), while networking and background HAL processing (like the HUD task) can run on Core 0 (`PRO_CPU_NUM`).
- **Stack Size**: Ensure adequate stack size for tasks (e.g., 4096 bytes for tasks interacting with complex libraries). Monitor for stack overflows during development.
- **Task Priorities**: Keep task priorities balanced. High-priority tasks that don't yield (e.g., via `vTaskDelay` or `delay`) will starve lower-priority tasks and the IDLE task.

## 2. Synchronization
- **Resource Locking**: Use the `syncLock()` and `syncUnlock()` methods in the `Toaster` class when accessing hardware resources or shared state that is modified across different task contexts (e.g., the Hub75 display buffer).
- **Semaphores & Mutexes**: 
  - Use `SemaphoreHandle_t` for signaling or resource locking.
  - Prefer `Recursive Mutexes` if a single task might need to acquire the same lock multiple times.
  - Always check the return value of `xSemaphoreTake` to handle timeouts gracefully.

## 3. Interrupt Safety (ISR)
- **ISR-Safe Functions**: Only use FreeRTOS functions suffixed with `FromISR` (e.g., `xSemaphoreGiveFromISR`) inside Interrupt Service Routines.
- **Minimize ISR Time**: Keep ISRs as short as possible. Perform only critical actions and defer complex processing to a task using a semaphore or queue.
- **`volatile` Keyword**: Use `volatile` for variables shared between ISRs and main tasks to prevent compiler optimizations from caching values.

## 4. Watchdog Timer (WDT)
- **Yielding**: Long-running loops in any task must include `delay(1)` or `vTaskDelay(1)` to allow the FreeRTOS scheduler to run other tasks and prevent the Task Watchdog Timer (TWDT) from triggering.
- **CPU Starvation**: Ensure Core 0 and Core 1 IDLE tasks get enough CPU time to reset their respective watchdogs.

## 5. Thread-Safe Logging
- Use the `TF_LOG*` macros for logging. If these macros are not thread-safe in the current implementation, ensure that logging calls from different tasks do not corrupt the serial output by using a mutex if necessary.
