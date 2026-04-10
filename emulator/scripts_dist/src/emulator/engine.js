"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.EmulatorEngine = void 0;
const binary_parser_1 = require("./binary-parser");
const cpu_1 = require("./cpu");
const hal_1 = require("./hal");
class EmulatorEngine {
    constructor() {
        this.image = null;
        this.displayCallback = null;
        this.frameUpdateInterval = null;
        this.cpu = new cpu_1.XtensaCPU();
        this.hal = new hal_1.ESP32HAL((msg) => {
            this.cpu.onLog(msg);
        });
        this.cpu.hal = this.hal;
    }
    async loadBinary(buffer, onLog) {
        try {
            onLog(`[ENGINE] Binary received: ${buffer.byteLength} bytes`);
            this.image = binary_parser_1.BinaryParser.parse(buffer, onLog);
            this.cpu.loadImage(this.image.entryAddr, this.image.segments);
            onLog(`[ENGINE] CPU prepared at entry 0x${this.image.entryAddr.toString(16)}`);
            return true;
        }
        catch (e) {
            onLog(`[ENGINE] FATAL ERROR: ${e.message}`);
            console.error(e);
            return false;
        }
    }
    start(onLog, onDisplay) {
        this.cpu.onLog = onLog;
        this.cpu.onDisplayUpdate = onDisplay;
        this.displayCallback = onDisplay;
        // Update frame buffer every ~33ms (30 FPS)
        this.frameUpdateInterval = setInterval(() => {
            if (this.displayCallback) {
                this.displayCallback(this.cpu.frameBuffer);
            }
        }, 33);
        this.cpu.start();
    }
    stop() {
        if (this.frameUpdateInterval) {
            clearInterval(this.frameUpdateInterval);
            this.frameUpdateInterval = null;
        }
        this.cpu.stop();
    }
}
exports.EmulatorEngine = EmulatorEngine;
