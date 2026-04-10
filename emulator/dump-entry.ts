import fs from 'fs';
import path from 'path';
import { BinaryParser } from './src/emulator/binary-parser.js';

const binPath = path.resolve(process.cwd(), '../.pio/build/adafruit_matrixportal_esp32s3/firmware.bin');
const binBuffer = fs.readFileSync(binPath).buffer as ArrayBuffer;
const result = BinaryParser.parse(binBuffer);

if (result) {
  console.log(`Entry: 0x${result.entryAddr.toString(16)}`);
  for (const seg of result.segments) {
    if (result.entryAddr >= seg.loadAddr && result.entryAddr < seg.loadAddr + seg.data.length) {
       console.log(`Found Entry Segment at 0x${seg.loadAddr.toString(16)}`);
       const offset = result.entryAddr - seg.loadAddr;
       const view = seg.data.slice(offset, offset + 32);
       let hex = "";
       view.forEach(b => hex += b.toString(16).padStart(2, '0') + " ");
       console.log(`Hex: ${hex}`);
    }
  }
}
