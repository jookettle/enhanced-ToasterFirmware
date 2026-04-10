# ESP32-S3 Emulator Debugging Guide

This document serves as a persistent context for debugging the `enhanced-ToasterFirmware` on the custom Xtensa LX7 emulator.

## 1. Project Overview
The emulator is built to run the `firmware.bin` (ESP32-S3) in a browser/CLI environment to verify boot logic and visual effects without physical hardware.

- **CPU Architecture**: Xtensa LX7 (32-bit, Windowed Register ABI).
- **Host Platform**: TypeScript / Node.js (CLI) / React (Web).

## 2. Core Architecture

### 2.1 Instruction Decoder (`cpu.ts`)
The decoder handles 24-bit and 16-bit (Narrow) Xtensa instructions.
- **RRI8 Format**: Critical mapping fix applied. `t` field is in Byte 0 (bits 4-7), `s` field is in Byte 1 (bits 0-3).
- **RRR Format**: Standard 3-register mapping.
- **Windowed Registers**: Implements `WindowBase` and `WindowStart` to manage the physical register file (64 registers) mapped to logical windows (`a0`-`a15`).

### 2.2 Memory & HAL (`hal.ts`)
- **Flash-to-IRAM Alias**: The emulator applies an offset of `0x1774` (experimentally derived) to map binary segments to the correct execution address.
- **ROM Stubs**: Since the ESP32-S3 internal ROM is not available, calls to `0x4000xxxx` are stubbed. 
  - The stub currently logic: Logs the call, simulates the Window return, and jumps to the address in `a0`.

## 3. Current Debugging Status (Phase 40+)

### 3.1 The "NULL SLIDE" Issue
The emulator currently execution reaches app initialization but halts at `0x403B083B`.
- **Symptom**: Execution of `00 00 00` (ADD a0, a0, a0) sequence in uninitialized IRAM.
- **Root Cause**: A `CALL4` instruction at `0x40374709` targets `0x403B082C`. This address is within SRAM0 (Instruction Bus) but is not part of any LOAD segments in the `firmware.bin`.
- **Observation**: `_iram_text_end` in `firmware.map` is `0x4038511F`. The jump to `0x403B082C` is ~177KB past the end of linked static code.

### 3.2 Implemented Instruction Extensions
- **Hardware Loops**: `LOOP`, `LOOPNEZ` and related SRs (`LBEG`, `LEND`, `LCOUNT`) are now operational.
- **Advanced Branches**: `BNONE`, `BANY`, `BBCI`, `BBSI` variants implemented.
- **Bitwise/Shifts**: `EXTUI`, `SRAI`, `SLLI`, `SRLI` verified.
- **Density Bypass**: Instructions starting with `0x3F` (Custom TIE) are logged and bypassed to avoid immediate halt.

### 3.3 Memory Map (ESP32-S3)
- **IRAM (SRAM0)**: `0x40370000 - 0x403DFFFF`.
- **DRAM (SRAM1)**: `0x3FC88000 - 0x3FCE7FFF`.
- **IROM (Flash)**: `0x42000000+`.

## 4. Key Lessons & Patterns
- **Header Offset**: ESP32-S3 `firmware.bin` uses a 24-byte extended header. Segment data starts after this header.
- **Jump Targets**: `CALL` and `J` instructions use `(PC & ~3) + 4 + (offset << 2)`. Correctness of `offset` (sign-extension) is critical.
- **HAL MMIO**: Early boot requires stubs for `GPIO`, `RTC`, and `Cache` controllers to avoid infinite wait loops.

## 5. Ongoing Tasks
- [ ] Determine if `0x403B082C` is a dynamic jump table or a missing Flash-to-IRAM mapping.
- [ ] Implement `RUR` / `WUR` (Read/Write User Register) if encountered.
- [ ] Support `VECBASE` modification to handle Window Exceptions correctly.
- [ ] Add `S32I` / `L32I` for `0x6000xxxx` (Peripheral) range with detailed logging.
- [ ] Enable UART interrupt simulation to capture framework-level log output.

## 6. Test Harness & Framebuffer Mapping (NEW)

- Added an emulator-only, memory-mapped framebuffer so firmware or test code can drive an LED matrix directly.
  - Framebuffer base: `0x3FF00000` (emulator-reserved).
  - Size: `64 * 32 * 4` bytes (RGBA per pixel).
  - Writes to this region are routed to the CPU's `frameBuffer` and trigger `onDisplayUpdate(frameBuffer)`.

- Runtime changes made:
  - `XtensaCPU.frameBuffer` (Uint8Array) added.
  - `resolveAddress()` maps `0x3FF00000..0x3FF00000+size` to `frameBuffer`.
  - `write8()` / `write32()` now call `onDisplayUpdate()` when the write targets the framebuffer.
  - `EmulatorEngine` periodically pushes the current `frameBuffer` to the UI via a 30 FPS interval.

- Test runner and build:
  - `tsconfig.node-run.json` added: compiles `src/emulator` to ESM under `scripts_dist/`.
  - `npm run build-node` compiles node-run artifacts.
  - `scripts/test-cpu-run.mjs` (ESM) runs either an actual `firmware.bin` simulation (if present) or a synthetic LED stress test.
    - Output frames are saved as PPM files in `emulator/scripts/output/frame-XXXXX.ppm`.
  - Convenience script: `npm run test-cpu` (build + run).

- How to run locally:
  1. Build node modules: `npm run build-node`
  2. Run test harness: `node ./scripts/test-cpu-run.mjs`
  3. To run indefinitely: set environment variable `INFINITE=1` before running.

- Observations from running the harness:
  - Synthetic stress test continuously generated frames and saved PPM files, confirming `onDisplayUpdate` is invoked for framebuffer writes.
  - HAL UART writes are captured and printed by the test harness via `ESP32HAL`'s UART FIFO stub.
  - When running real firmware, the emulator will attempt to execute code; if execution reaches unmapped IRAM or a strict halt, the CPU logs registers and trace for debugging.

- Completed items during this work:
  - Mapped framebuffer and hooked CPU writes -> display updates.
  - Added test harness (`scripts/test-cpu-run.mjs`) and CommonJS fallback runner for other environments.
  - Added `npm run build-node` and `npm run test-cpu` scripts.
  - Verified synthetic test run and produced thousands of PPM frames (see `emulator/scripts/output`).

## 7. Notes & Next Steps

- The `NULL SLIDE` symptom still indicates missing code segments or dynamic jump tables in some firmware images; when encountered, use the generated `trace_audit.txt` and the PPM frames to correlate boot progress with visual output.
- If you want real-time visual inspection instead of PPM snapshots, add a small HTTP server that serves the latest frame as PNG or a WebSocket feed into the React UI.

