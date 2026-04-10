import { BinaryParser } from './binary-parser';
import { XtensaCPU } from './cpu';
import { ESP32HAL } from './hal';
export class EmulatorEngine {
    constructor() {
        this.image = null;
        this.displayCallback = null;
        this.frameUpdateInterval = null;
        this.overlayInterval = null;
        this.overlayState = false;
        this.cpu = new XtensaCPU();
        this.hal = new ESP32HAL((msg) => {
            this.cpu.onLog(msg);
        });
        this.cpu.hal = this.hal;
    }
    async loadBinary(buffer, onLog) {
        try {
            onLog(`[ENGINE] Binary received: ${buffer.byteLength} bytes`);
            this.image = BinaryParser.parse(buffer, onLog);
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
        // Overlay heartbeat: draw a small blinking box so UI shows activity
        this.overlayState = false;
        this.overlayInterval = setInterval(() => {
            try {
                const buf = this.cpu.frameBuffer;
                const w = 64, h = 32;
                const boxW = 8, boxH = 8;
                const startX = Math.max(0, Math.floor((w - boxW) / 2));
                const startY = Math.max(0, Math.floor((h - boxH) / 2));
                if (!this.overlayState) {
                    // draw green box
                    for (let y = 0; y < boxH; y++) {
                        for (let x = 0; x < boxW; x++) {
                            const idx = ((startY + y) * w + (startX + x)) * 4;
                            buf[idx + 0] = 0; // R
                            buf[idx + 1] = 220; // G
                            buf[idx + 2] = 0; // B
                            buf[idx + 3] = 255; // A
                        }
                    }
                }
                else {
                    // clear box to black
                    for (let y = 0; y < boxH; y++) {
                        for (let x = 0; x < boxW; x++) {
                            const idx = ((startY + y) * w + (startX + x)) * 4;
                            buf[idx + 0] = 0;
                            buf[idx + 1] = 0;
                            buf[idx + 2] = 0;
                            buf[idx + 3] = 255;
                        }
                    }
                }
                this.overlayState = !this.overlayState;
            }
            catch (e) {
                // ignore overlay errors
            }
        }, 500);
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
