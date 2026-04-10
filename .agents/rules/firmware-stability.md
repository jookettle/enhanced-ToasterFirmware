---
trigger: always_on
glob: "**/*.{cpp,h,hpp}"
description: Stability and performance rules for firmware development.
---

# Firmware Stability & Performance

To ensure the high reliability and responsiveness required for the `NeoToasterFirmware` project, adhere to these stability and performance guidelines.

## 1. Non-Blocking Logic
- **Avoid `delay()`**: Do **not** use `delay()` within `work()`, `loop()`, or any function called frequently. Use the `Worker` class or `Timer::get_millis()` for periodic tasks.
- **Initialization Only**: Small delays are acceptable only during hardware initialization (e.g., in `begin()` methods).
- **Infinite Loops**: Never use loops that could run indefinitely (like `while(true)`) without a timeout or break condition unless waiting for a critical system failure.

## 2. Safety & Error Handling
- **Pointer Validation**: Always check for `nullptr` before dereferencing pointers, especially for hardware objects or buffers that might fail to initialize.
- **I/O Failures**: Handle communication failures (I2C, SPI, SD card) gracefully. Failure to communicate with a sensor should not crash the system.
- **Return Values**: Check return values of critical functions like `begin()`, `snprintf()`, or `malloc()` to handle errors before they lead to undefined behavior.

## 3. Buffer & String Safety
- **Safe Formatting**: Use `snprintf()` instead of `sprintf()` to prevent buffer overflows. Always provide the correct buffer size.
- **Static Buffers**: Prefer fixed-size static buffers over frequent `std::string` or `malloc` calls in timing-critical code to avoid heap fragmentation and allocation failures.
- **Bound Checks**: Always validate indices before accessing arrays or vectors.

## 4. Performance Optimization
- **Expensive Operations**: Minimize expensive math (e.g., `sin`, `cos`, `pow`) and complex data structures (e.g., `std::map`) within high-frequency loops (Hub75 refresh loops). Use Look-Up Tables (LUTs) where possible.
- **Logging Overhead**: Use `TF_LOG*` macros selectively. Excessive logging at high frequencies can starve the CPU of critical task time.
- **Memory Access**: Be mindful of PSRAM access speed vs. SRAM. Use SRAM for frequently accessed buffers if size permits.

## 5. Hardware Constraints
- **Watchdog Timer (WDT)**: Ensure that your code yields control back to the system periodically to prevent triggering the ESP32 hardware watchdog. Long-running computational loops must include a `delay(0)` or `vTaskDelay(1)` if they are not in a higher priority task.
- **Resource Locking**: Use `syncLock()` and `syncUnlock()` in the `Toaster` class for thread-safe access to shared hardware resources like the Hub75 display.
