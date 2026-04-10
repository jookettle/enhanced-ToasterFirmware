import fs from 'fs';
import path from 'path';
import WebSocket, { WebSocketServer } from 'ws';

// Import compiled ESM modules
import { XtensaCPU } from '../scripts_dist/emulator/cpu.js';
import { BinaryParser } from '../scripts_dist/emulator/binary-parser.js';
import { ESP32HAL } from '../scripts_dist/emulator/hal.js';

const FB_BASE = 0x3FF00000 >>> 0;

// WebSocket server to stream raw binary frames to the web UI
const STREAM_PORT = Number(process.env.STREAM_PORT || 8081);
const wss = new WebSocketServer({ port: STREAM_PORT });
wss.on('listening', () => console.log(`[TEST] WebSocket frame stream ws://localhost:${STREAM_PORT}`));

function broadcastFrame(buf) {
  const data = Buffer.from(buf);
  for (const client of wss.clients) {
    if (client.readyState === WebSocket.OPEN) {
      try { client.send(data); } catch (e) { }
    }
  }
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
  const cwd = process.cwd();
  const binPath = path.resolve(cwd, '.pio/build/adafruit_matrixportal_esp32s3/firmware.bin');
  const cpu = new XtensaCPU();
  const hal = new ESP32HAL((msg) => cpu.onLog(msg));
  cpu.hal = hal;

  let frameIdx = 0;
  cpu.onLog = (m) => { if (m) console.log('[LOG]', m); };
  cpu.onDisplayUpdate = (buf) => {
    broadcastFrame(buf);
    frameIdx++;
  };

  // Discover nearby .bin files and pick a primary firmware if available.
  let primaryBinCandidate = null;
  try {
    const allBins = findBinFiles(process.cwd(), 3);
    const parsed = [];
    for (const bin of allBins) {
      try {
        const raw = fs.readFileSync(bin);
        const arrayBuffer = raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength);
        const img = BinaryParser.parse(arrayBuffer, (m) => cpu.onLog(`[PARSER:${path.basename(bin)}] ${m}`));
        if (img && img.segments && img.segments.length > 0) {
          parsed.push({ bin, img });
          cpu.onLog(`[TEST] Discovered parsed image ${bin} (entry=0x${img.entryAddr.toString(16)}, segments=${img.segments.length})`);
        }
      } catch (e) {
        // ignore non-parsable binary
      }
    }

    // Prefer the canonical .pio path if present, otherwise prefer common firmware names
    if (fs.existsSync(binPath)) {
      primaryBinCandidate = binPath;
    } else if (parsed.length > 0) {
      const prefer = parsed.find(p => p.bin.includes(path.join('emulator', 'dist')) || p.bin.includes(path.join('emulator', 'public')) || p.bin.endsWith('firmware.bin'));
      primaryBinCandidate = prefer ? prefer.bin : parsed[0].bin;
    }

    // Load parsed images as supplemental except the chosen primary
    for (const p of parsed) {
      if (p.bin === primaryBinCandidate) continue;
      cpu.onLog(`[TEST] Loading supplemental image ${p.bin} (entry=0x${p.img.entryAddr.toString(16)}, segments=${p.img.segments.length})`);
      cpu.loadImage(p.img.entryAddr, p.img.segments);
    }
  } catch (e) {
    // ignore discovery errors
  }

  if (primaryBinCandidate && fs.existsSync(primaryBinCandidate)) {
    console.log('[TEST] Found firmware.bin, attempting to run binary simulation.');
    console.log('[TEST] Using:', primaryBinCandidate);
    const raw = fs.readFileSync(primaryBinCandidate);
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
      if (steps % 10000 === 0) cpu.onDisplayUpdate(cpu.frameBuffer);
    }

    console.log('\n[TEST] Simulation ended. Steps:', steps);
    broadcastFrame(cpu.frameBuffer);
    frameIdx++;
  } else {
    console.log('[TEST] firmware.bin not found, running synthetic LED stress test.');
    const w = 64, h = 32;
    let cycles = process.env.INFINITE ? Number.MAX_SAFE_INTEGER : 500;
    for (let c = 0; c < cycles; c++) {
      for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
          const idx = (y * w + x) * 4;
          const r = (x + c) % 256;
          const g = (y + c * 2) % 256;
          const b = (x + y + c * 3) % 256;
          const base = FB_BASE + idx;
          cpu.write8(base + 0, r);
          cpu.write8(base + 1, g);
          cpu.write8(base + 2, b);
          cpu.write8(base + 3, 255);
        }
      }
      await new Promise(r => setTimeout(r, 10));
      cpu.onDisplayUpdate(cpu.frameBuffer);
      if ((c + 1) % 10 === 0) process.stdout.write('#');
    }
    console.log('\n[TEST] Synthetic test completed.');
  }
}

run().catch(err => console.error('[TEST] Error:', err));
