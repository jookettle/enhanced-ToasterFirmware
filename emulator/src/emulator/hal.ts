export class ESP32HAL {
  private onLog: (msg: string) => void;
  private uartBuffer = "";
   private writeLogState = new Map<number, { count: number; last: number }>();

  constructor(onLog: (msg: string) => void) {
    this.onLog = onLog;
  }

   private maybeLogWrite(addr: number, val: number) {
      const key = addr & ~0x3; // coarse-grain by word
      const now = Date.now();
      const s = this.writeLogState.get(key) || { count: 0, last: 0 };
      if (s.count < 8 || now - s.last > 250) {
         this.onLog(`[HAL] W 0x${addr.toString(16)} = 0x${val.toString(16)}`);
         s.count++;
         s.last = now;
         this.writeLogState.set(key, s);
      }
   }

  handleRead(addr: number): number | null {
    addr >>>= 0;
    
    // UART0 Status Stub
    if (addr === 0x6000000C) return 0x00000000;
    
    // SPI0 / SPI1 (Flash Controller) Status Stubs
    // Bootloaders often wait for SPI_MEM_CTRL_REG or SPI_MEM_CMD_REG bits to clear.
    if (addr >= 0x60002000 && addr <= 0x60002FFF) {
       return 0; // Everything is IDLE and READY
    }
    
    // Timer / WDT Stubs
    if (addr >= 0x6001F000 && addr <= 0x6001F0FF) return 0;
    
    // GPIO / IO MUX Stubs
    if (addr === 0x6000403C) return 0x00000000; // GPIO_IN_REG (All low)
    if (addr === 0x60004044) return 0x00000000; // GPIO_IN_REG_1 (All low)
    
    // RTC Controller
    if (addr >= 0x60008000 && addr <= 0x60008FFF) return 0x0;
    
    // Cache Controller (S3)
    if (addr >= 0x61801000 && addr <= 0x61801FFF) {
       return 0x00000001; // Cache enabled/ready
    }
    
    return null;
  }

  handleWrite(addr: number, val: number): boolean {
    addr >>>= 0;
    
    // UART0 FIFO Write (Log Output)
    if (addr === 0x60000000) {
       const char = String.fromCharCode(val & 0xFF);
       if (char === '\n') {
          this.onLog(`[UART0] ${this.uartBuffer}`);
          this.uartBuffer = "";
       } else {
          this.uartBuffer += (val & 0xFF) >= 32 ? char : `[0x${(val & 0xFF).toString(16)}]`;
       }
       return true;
    }

      // SPI / Flash Controller Writes
      if (addr >= 0x60002000 && addr <= 0x60002FFF) {
         this.maybeLogWrite(addr, val);
         return true;
      }
    
   // Ignore WDT / Timer Writes for stability
   if (addr >= 0x6001F000 && addr <= 0x6001F0FF) { this.maybeLogWrite(addr, val); return true; } 

   // GPIO / MUX Writes
   if (addr >= 0x60004000 && addr <= 0x60004FFF) { this.maybeLogWrite(addr, val); return true; }
    
   // Cache Controller Writes
   if (addr >= 0x61801000 && addr <= 0x61801FFF) { this.maybeLogWrite(addr, val); return true; }
    
   // RTC Controller Writes
   if (addr >= 0x60008000 && addr <= 0x60008FFF) { this.maybeLogWrite(addr, val); return true; }

    return false;
  }
}
