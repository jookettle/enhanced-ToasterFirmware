import fs from 'fs';
import path from 'path';
import { XtensaCPU } from '../src/emulator/cpu.js';
import { BinaryParser } from '../src/emulator/binary-parser.js';
import { ESP32HAL } from '../src/emulator/hal.js';

async function runTest() {
  const binPath = path.resolve(process.cwd(), '../.pio/build/adafruit_matrixportal_esp32s3/firmware.bin');
  if (!fs.existsSync(binPath)) {
    console.error(`[TEST] firmware.bin not found at ${binPath}`);
    process.exit(1);
  }

  console.log(`[TEST] Loading ${binPath}...`);
  const binBuffer = fs.readFileSync(binPath).buffer as ArrayBuffer;
  
  const cpu = new XtensaCPU();
  const hal = new ESP32HAL((msg) => cpu.onLog(msg));
  cpu.hal = hal;

  const logs: string[] = [];
  cpu.onLog = (msg) => {
    logs.push(msg);
    if (msg.includes('STRICT HALT') || msg.includes('UART')) {
        console.log(msg);
    }
  };

  const result = BinaryParser.parse(binBuffer);
  
  if (!result) {
    console.error("[TEST] Failed to parse binary.");
    process.exit(1);
  }

  console.log(`[TEST] Extracted ${result.segments.length} segments. Entry: 0x${result.entryAddr.toString(16)}`);
  for (const seg of result.segments) {
    console.log(`  - Seg: 0x${seg.loadAddr.toString(16)} size: ${seg.data.length} bytes`);
  }
  
  cpu.loadImage(result.entryAddr, result.segments);

  console.log("[TEST] Starting high-speed simulation...");
  cpu.isRunning = true;
  
  let steps = 0;
  const MAX_STEPS = 5000000;
  const recentTrace: string[] = [];

  while (cpu.isRunning && steps < MAX_STEPS) {
    const pc = cpu.pc;
    // Peek at next instruction bytes for raw debug
    const b0 = cpu.read8(pc);
    const b1 = cpu.read8(pc + 1);
    const b2 = cpu.read8(pc + 2);
    const rawHex = `${b0.toString(16).padStart(2, '0')} ${b1.toString(16).padStart(2, '0')} ${b2.toString(16).padStart(2, '0')}`;
    
    cpu.step();

    // DETECT ROM TRAP: Entering ROM with a0=0
    if (cpu.pc === 0x40000000 && cpu.getA(0) === 0) {
        cpu.onLog(`[TEST] Detected ROM TRAP at step ${steps}. Halting for analysis.`);
        cpu.isRunning = false;
    }
    
    const lastCpuTrace = cpu.trace[cpu.trace.length - 1] || "???";
    recentTrace.push(`[${rawHex}] ${lastCpuTrace}`);
    
    if (recentTrace.length > 5000) recentTrace.shift();
    steps++;
    
    if (steps % 100000 === 0) process.stdout.write(".");
  }
  console.log("\n");

  const finalReport = `
[TEST] Simulation End
Steps: ${steps}
PC: 0x${cpu.pc.toString(16)}
WindowBase: ${cpu.windowBase}
--------------------------------
${cpu.dumpRegisters()}
--------------------------------
Last Trace (5000 steps):
${recentTrace.join('\n')}
--------------------------------
Logs:
${logs.join('\n')}
`;

  fs.writeFileSync('trace_audit.txt', finalReport);
  console.log(`[TEST] Audit report saved to trace_audit.txt`);
}

runTest().catch(err => {
  console.error("[TEST] Runtime Error:", err);
});
