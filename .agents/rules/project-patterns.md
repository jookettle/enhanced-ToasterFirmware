---
trigger: always_on
glob: "**/*.{cpp,h,hpp}"
description: Guidelines for following project-specific architectural patterns.
---

# Project Architecture & Patterns

The `NeoToasterFirmware` project uses several key architectural patterns to manage state, visual effects, and hardware interaction.

## 1. The `Worker` Pattern
Most background tasks should inherit from the `Worker` class located in `src/lib/worker.h`.
- **`work()`**: This method is called periodically based on the target frequency/FPS. Implement your core logic here.
- **`workPerSecond()`**: This method is called once per second, useful for logging performance metrics (like FPS) or handling low-frequency updates.
- **Initialization**: Call `Worker::begin(fps)` or `Worker::beginPeriod(ms)` in your initialization code to start the periodic task.

## 2. The `Effect` System
Visual effects for the Hub75 and NeoPixel displays must inherit from the `Effect` base class in `src/effects/effect_base.h`.
- **`init(Display& display)`**: Setup code for the effect.
- **`process(Display& display)`**: Core rendering logic. This is called in every display refresh cycle.
- **`release(Display& display)`**: Cleanup code, including freeing any dynamically allocated memory.
- **Static vs. Dynamic**: Use `setStaticMode(true)` if the effect should pause movement, or `setStaticMode(false)` to resume.

## 3. Hardware Abstraction Layer (HAL)
Hardware-specific code should be isolated in the `src/hal/` directory.
- **Display**: Use the `Display` interface for drawing. Avoid calling Hub75 or NeoPixel-specific functions directly from effects.
- **Sensors**: Sensors should follow the `Worker` pattern to sample data periodically and provide an interface for other parts of the system to read values.
- **Communication**: Peripheral logic (IR, ESP-NOW) should be encapsulated in their respective controller classes.

## 4. Logging Patterns
Always use the `TF_LOG*` macros from `src/lib/logger.h` for output. These macros include a high-resolution timestamp and color-coding for different log levels:
- `TF_LOGE(TAG, fmt, ...)`: Error logs. Use for critical failures.
- `TF_LOGW(TAG, fmt, ...)`: Warning logs. Use for non-critical issues.
- `TF_LOGI(TAG, fmt, ...)`: Information logs. Use for major system events.
- `TF_LOGD(TAG, fmt, ...)`: Debug logs. Use for detailed troubleshooting (muted in production).

## 5. Configuration (YAML)
The project uses a YAML-based configuration system.
- Use `YamlNodeArray::fromFile("/config.yaml", FFat)` to load the main configuration.
- Access configuration values using `getInt()`, `getFloat()`, `getString()`, or `getBool()`.
- Always provide a default value when reading configuration to handle missing entries gracefully.
