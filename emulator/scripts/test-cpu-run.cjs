#!/usr/bin/env node
const fs = require('fs');
const path = require('path');
const { WebSocketServer, WebSocket } = require('ws');

// Load compiled emulator modules
const cpuMod = require('../scripts_dist/emulator/cpu.js');
const bpMod = require('../scripts_dist/emulator/binary-parser.js');
const halMod = require('../scripts_dist/emulator/hal.js');

const { XtensaCPU } = cpuMod;
const { BinaryParser } = bpMod;
const { ESP32HAL } = halMod;

const FB_BASE = 0x3FF00000 >>> 0;

const STREAM_PORT = Number(process.env.STREAM_PORT || 8081);
const wss = new WebSocketServer({ port: STREAM_PORT });
wss.on('listening', () => console.log(`[TEST] WebSocket frame stream ws://localhost:${STREAM_PORT}`));

function broadcastFrame(buf) {
  const data = Buffer.from(buf);
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      try { client.send(data); } catch (e) { }
    }
  });
}

function findBinFiles(root, maxDepth = 3) {
  const results = [];
  function walk(dir, depth) {
    if (depth > maxDepth) return;
    let entries;
    try { entries = fs.readdirSync(dir, { withFileTypes: true }); } catch (e) { return; }
    for (const ent of entries) {
      const full = path.join(dir, ent.name);
      if (ent.isFile() && ent.name.toLowerCase().endsWith('.bin')) results.push(full);
      else if (ent.isDirectory()) walk(full, depth + 1);
    }
  }
  walk(root, 0);
  return results;
}

async function run() {
  const binPath = path.resolve(process.cwd(), '.pio/build/adafruit_matrixportal_esp32s3/firmware.bin');
  const cpu = new XtensaCPU();
  const hal = new ESP32HAL((msg) => cpu.onLog(msg));
  cpu.hal = hal;

  let frameIdx = 0;
  cpu.onLog = (m) => { if (m) console.log('[LOG]', m); };
  cpu.onDisplayUpdate = (buf) => {
    broadcastFrame(buf);
    frameIdx++;
  };

  // Attempt to auto-load supplemental binary images (bootloader, ROMs) found nearby
  try {
    const allBins = findBinFiles(process.cwd(), 3);
    for (const bin of allBins) {
      if (path.resolve(bin) === path.resolve(binPath)) continue;
      try {
        const raw = fs.readFileSync(bin);
        const arrayBuffer = raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength);
        const img = BinaryParser.parse(arrayBuffer, (m) => console.log('[PARSER:' + path.basename(bin) + ']', m));
        if (img && img.segments && img.segments.length > 0) {
          console.log('[TEST] Loading supplemental image', bin, `(entry=0x${img.entryAddr.toString(16)}, segments=${img.segments.length})`);
          cpu.loadImage(img.entryAddr, img.segments);
        }
      } catch (e) {
        // ignore non-parsable binary
      }
    }
  } catch (e) {
    // ignore discovery errors
  }

  if (fs.existsSync(binPath)) {
    console.log('[TEST] Found firmware.bin, attempting to run binary simulation.');
    const raw = fs.readFileSync(binPath);
    const arrayBuffer = raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength);
    const img = BinaryParser.parse(arrayBuffer, (m) => console.log('[PARSER]', m));
    console.log(`[TEST] Entry 0x${img.entryAddr.toString(16)}, segments=${img.segments.length}`);
    cpu.loadImage(img.entryAddr, img.segments);
    console.log('[TEST] Starting simulation...');

    cpu.isRunning = true;
    let steps = 0;
    const MAX_STEPS = process.env.INFINITE ? Number.MAX_SAFE_INTEGER : 200000;

    while (cpu.isRunning && steps < MAX_STEPS) {
      cpu.step();
      steps++;
      if (steps % 5000 === 0) process.stdout.write('.');
      // If nothing writes to frame buffer, periodically emit the current buffer
      if (steps % 10000 === 0) cpu.onDisplayUpdate(cpu.frameBuffer);
    }

    console.log('\n[TEST] Simulation ended. Steps:', steps);
    broadcastFrame(cpu.frameBuffer);
    frameIdx++;
  } else {
    console.log('[TEST] firmware.bin not found, running synthetic LED stress test.');
    // Synthetic: animate color wipes by writing directly to mapped memory
    const w = 64, h = 32;
    let cycles = process.env.INFINITE ? Number.MAX_SAFE_INTEGER : 500;
    for (let c = 0; c < cycles; c++) {
      // Fill frame with gradient based on cycle
      for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
          const idx = (y * w + x) * 4;
          const r = (x + c) % 256;
          const g = (y + c * 2) % 256;
          const b = (x + y + c * 3) % 256;
          // write via memory-mapped region (use cpu.write8 to exercise emulator path)
          const base = FB_BASE + idx;
          cpu.write8(base + 0, r);
          cpu.write8(base + 1, g);
          cpu.write8(base + 2, b);
          cpu.write8(base + 3, 255);
        }
      }
      // small delay
      await new Promise(r => setTimeout(r, 10));
      // onDisplayUpdate will be called by write8; but ensure a snapshot
      cpu.onDisplayUpdate(cpu.frameBuffer);
      if ((c + 1) % 10 === 0) process.stdout.write('#');
    }
    console.log('\n[TEST] Synthetic test completed. Frames saved:', frameIdx);
  }
}

run().catch(err => console.error('[TEST] Error:', err));
