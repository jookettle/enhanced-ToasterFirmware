---
trigger: always_on
glob: "**/*.{cpp,h,hpp}"
description: General C++ coding standards for the NeoToasterFirmware project.
---

# C++ Usage Standards

To ensure code quality and consistency across the `NeoToasterFirmware` codebase, follow these C++ standards.

## 1. File Structure & Headers
- **Header Guards**: Use `#pragma once` at the top of every header file.
- **Includes**: Group includes by project-local first, then libraries, then standard libraries. Use relative paths within the project.
- **Namespaces**: All project-owned code must reside within the `toaster` namespace. Third-party or vendored code (for example, code under `src/lib/`) is exempt and should keep its upstream namespace and structure.

## 2. Naming Conventions
- **Classes**: `PascalCase` (e.g., `EffectDino`).
- **Public Methods**: `camelCase` (e.g., `getDinoX()`).
- **Private/Protected Members**: `_snake_case` with a leading underscore (e.g., `_jump_frame`).
- **Local Variables**: `snake_case` (e.g., `dino_x_expand`).
- **Constants & Enums**: `SCREAMING_SNAKE_CASE` (e.g., `STATE_GAMEOVER`).
- **Structs**: Same as Classes/Enums (`PascalCase`).

## 3. Formatting & Style
- **Indentation**: Use **2 spaces** for indentation. No tabs.
- **Braces**: Use **K&R style** (braces on the same line) for functions, classes, and control structures (if, for, while, switch).
  ```cpp
  void myFunction() {
    if (condition) {
      // ...
    }
  }
  ```
- **Line Length**: Aim for a maximum of 100-120 characters per line.

## 4. `const` Correctness
- Use `const` for any member function that does not modify the object state.
- Use `const` for parameters that are not modified by the function (especially references and pointers).
- Prefer `static const` or `constexpr` over `#define` for constants.

## 5. Memory Management
- **Embedded Constraint**: Avoid frequent `new`/`delete` calls to prevent heap fragmentation.
- **Static Allocation**: Favor stack allocation or pre-allocated static buffers for fixed-size data.
- **Cleanup**: Always pair `new` with `delete` in the `release()` or destructor methods. Check for `nullptr` before deleting and set to `nullptr` after.
  ```cpp
  if (_ptr != nullptr) {
    delete _ptr;
    _ptr = nullptr;
  }
  ```

## 6. Language Features
- **Exceptions**: Do **not** use C++ exceptions. Use return values or status flags for error handling.
- **Templates**: Use sparingly. Prefer polymorphism or simple abstractions to keep binary size small.
- **Standard Library**: Favor `<vector>`, `<string>`, and `<map>` where performance allows, but be mindful of their memory overhead on ESP32.
