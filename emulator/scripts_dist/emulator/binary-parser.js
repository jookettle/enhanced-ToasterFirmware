export class BinaryParser {
    static parse(buffer, onLog = () => { }) {
        const view = new DataView(buffer);
        const magic = view.getUint8(0);
        if (magic !== 0xE9) {
            throw new Error(`Invalid magic byte: 0x${magic.toString(16)}. Expected 0xE9.`);
        }
        const segmentCount = view.getUint8(1);
        const entryAddr = view.getUint32(4, true);
        onLog(`[PARSER] Parsing image: magic=0x${magic.toString(16)}, segments=${segmentCount}, entry=0x${entryAddr.toString(16)}`);
        const segments = [];
        let offset = 24; // Header size for ESP32 image
        for (let i = 0; i < segmentCount; i++) {
            if (offset + 8 > buffer.byteLength) {
                onLog(`[PARSER] TRUNCATED: Offset ${offset} exceeds buffer`);
                break;
            }
            const loadAddr = view.getUint32(offset, true);
            const dataLen = view.getUint32(offset + 4, true);
            offset += 8;
            onLog(`[PARSER] Segment ${i}: addr=0x${loadAddr.toString(16)}, len=${dataLen} bytes`);
            if (offset + dataLen > buffer.byteLength) {
                throw new Error(`Segment ${i} data (len ${dataLen}) at offset ${offset} exceeds total buffer length (${buffer.byteLength})`);
            }
            const segmentData = new Uint8Array(buffer, offset, dataLen);
            segments.push({ loadAddr, data: segmentData });
            offset += dataLen;
        }
        onLog(`[PARSER] Successfully parsed ${segments.length} segments.`);
        return {
            magic,
            segmentCount,
            entryAddr,
            segments
        };
    }
}
