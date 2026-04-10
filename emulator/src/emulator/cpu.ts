export class XtensaCPU {
  // Physical Register File (64 registers for windowing)
  public ar = new Uint32Array(64);
  
  public pc = 0;
  public windowBase = 0;
  public windowStart = 1; 
  
  public sar = 0; 
  public ps = 0;  
  
  public lbeg = 0;
  public lend = 0;
  public lcount = 0;
  
  // Sparse Memory Regions
  private sram = new Uint8Array(4 * 1024 * 1024); // 4MB SRAM
  private rom = new Uint8Array(512 * 1024); // 512KB Internal ROM
  private flash = new Uint8Array(16 * 1024 * 1024); // 16MB Flash
  
  // Frame Buffer (64x32 RGBA)
  public frameBuffer = new Uint8Array(64 * 32 * 4);
  
  public isRunning = false;
  public hal: any = null;
  private loadedSegments: { start: number; end: number; data: Uint8Array }[] = [];
  
  // Diagnostic metrics
  public instructionsExecuted = 0;
  public lastStatTime = 0;
  public lastPC = 0;
  public trace: string[] = [];
  private readonly TRACE_SIZE = 100;
  private nullSlideCount = 0;

  constructor() {
    this.rom.fill(0x00);
    // Initialize black frame buffer
    this.frameBuffer.fill(0);
    for (let i = 3; i < this.frameBuffer.length; i += 4) {
      this.frameBuffer[i] = 255; // Alpha channel
    }
  }

  private getRegIdx(aIdx: number): number {
    return (this.windowBase * 4 + aIdx) % 64;
  }

  setA(idx: number, val: number) { this.ar[this.getRegIdx(idx)] = val >>> 0; }
  getA(idx: number): number { return this.ar[this.getRegIdx(idx)] >>> 0; }

  public dumpRegisters(): string {
     let out = "Registers:\n";
     for (let i = 0; i < 16; i++) {
        out += `  a${i.toString().padEnd(2)}: 0x${this.getA(i).toString(16).padStart(8, '0')}${i % 4 === 3 ? '\n' : ''}`;
     }
     out += `  SAR: 0x${this.sar.toString(16)}  WB: ${this.windowBase}  WS: 0x${this.windowStart.toString(16)}\n`;
     return out;
  }

  public read8(addr: number): number {
    const res = this.resolveAddress(addr);
    if (!res) return 0;
    return res.buffer[res.offset];
  }

  private resolveAddress(addr: number): { buffer: Uint8Array; offset: number } | null {
    addr >>>= 0;
    // Map a reserved, emulator-only address range into the frame buffer so
    // firmware (or tests) can write pixel data directly to 0x3FF0_0000.
    // Size = 64 * 32 * 4 (RGBA)
    const FB_BASE = 0x3FF00000 >>> 0;
    if (addr >= FB_BASE && addr < (FB_BASE + this.frameBuffer.length)) {
      return { buffer: this.frameBuffer, offset: addr - FB_BASE };
    }
    for (const seg of this.loadedSegments) {
      if (addr >= seg.start && addr < seg.end) {
        return { buffer: seg.data, offset: addr - seg.start };
      }
    }
    if (addr >= 0x40000000 && addr <= 0x4007FFFF) return { buffer: this.rom, offset: addr - 0x40000000 };
    if (addr >= 0x3FC00000 && addr <= 0x3FCFFFFF) return { buffer: this.sram, offset: addr - 0x3FC00000 };
    if (addr >= 0x40370000 && addr <= 0x403DFFFF) return { buffer: this.sram, offset: addr - 0x40370000 + 0x200000 };
    if (addr >= 0x42000000 && addr <= 0x42FFFFFF) return { buffer: this.flash, offset: addr - 0x42000000 };
    if (addr >= 0x3C000000 && addr <= 0x3CFFFFFF) return { buffer: this.flash, offset: addr - 0x3C000000 };
    return null;
  }

  loadImage(entryAddr: number, segments: { loadAddr: number; data: Uint8Array }[]) {
    this.pc = entryAddr;
    this.loadedSegments = segments.map(s => ({ start: s.loadAddr, end: s.loadAddr + s.data.length, data: s.data }));
    for (const seg of segments) {
        if (seg.loadAddr >= 0x42000000 && seg.loadAddr < 0x42FFFFFF) {
            this.flash.set(seg.data, seg.loadAddr - 0x42000000);
        } else if (seg.loadAddr >= 0x3C000000 && seg.loadAddr < 0x3CFFFFFF) {
            this.flash.set(seg.data, seg.loadAddr - 0x3C000000);
        } else if (seg.loadAddr >= 0x3FC00000 && seg.loadAddr < 0x3FCFFFFF) {
            this.sram.set(seg.data, seg.loadAddr - 0x3FC00000);
        } else if (seg.loadAddr >= 0x40370000 && seg.loadAddr < 0x403DFFFF) {
            this.sram.set(seg.data, seg.loadAddr - 0x40370000 + 0x200000);
        }
    }
  }

  private addTrace(msg: string, b0?: number, b1?: number, b2?: number) {
    let hex = "";
    if (b0 !== undefined) hex = b0.toString(16).padStart(2, '0');
    if (b1 !== undefined) hex += " " + b1.toString(16).padStart(2, '0');
    if (b2 !== undefined) hex += " " + b2.toString(16).padStart(2, '0');
    this.trace.push(`0x${this.pc.toString(16).padStart(8, '0')}: [${hex.padEnd(8)}] ${msg}`);
    if (this.trace.length > this.TRACE_SIZE) this.trace.shift();
  }

  private isPCValid(addr: number): boolean {
    if (addr >= 0x40000000 && addr <= 0x4007FFFF) return true;
    if (addr >= 0x40370000 && addr <= 0x403DFFFF) return true;
    if (addr >= 0x42000000 && addr <= 0x42FFFFFF) return true;
    for (const seg of this.loadedSegments) {
      if (addr >= seg.start && addr < seg.end) return true;
    }
    return false;
  }

  private getImm18(b0: number, b1: number, b2: number): number {
    const v = (b2 << 16) | (b1 << 8) | b0;
    return (v >> 6) << 14 >> 14; 
  }

  step() {
    if (!this.isRunning) return;
    if (!this.isPCValid(this.pc)) {
       this.haltUnknown(0,0,0, `EXECUTION OUTSIDE SEGMENTS (ORPHAN JUMP)`);
       return;
    }

    if (this.pc >= 0x40000000 && this.pc <= 0x4007FFFF) {
       if (this.read8(this.pc) === 0x00) {
          const ra = this.getA(0);
          this.onLog(`[ROM] Stubbed Call at 0x${this.pc.toString(16)} -> Return to 0x${ra.toString(16)} (a2=0x${this.getA(2).toString(16)})`);
          this.windowBase = (this.windowBase - ((ra >>> 30)) + 16) % 16;
          this.pc = (ra & 0x3FFFFFFF) | (this.pc & 0xC0000000);
          return;
       }
    }

    const b0 = this.read8(this.pc);
    const b1 = this.read8(this.pc + 1);
    const b2 = this.read8(this.pc + 2);
    
    const op0 = b0 & 0x0F;
    const is16Bit = (op0 >= 0x8); 
    const instrLen = is16Bit ? 2 : 3;

     if (!is16Bit && b0 === 0 && b1 === 0 && b2 === 0) {
       this.nullSlideCount++;
       if (this.nullSlideCount > 5) {
         // Fallback: try several candidate return addresses derived from `a0`.
         // Some firmwares store return-lowbits or use different top-bit mappings
         // so try a set of likely address reconstructions before halting.
         const ra = this.getA(0) >>> 0;
         if (ra !== 0) {
           const baseLow = ra & 0x3FFFFFFF;
           const candidates = [
             ra,
             baseLow,
             baseLow | 0x40000000,
             baseLow | 0x42000000,
             baseLow | 0x3C000000,
             baseLow | 0x3FC00000,
           ];
           let chosen: number | null = null;
           for (const c of candidates) {
             if (this.isPCValid(c)) { chosen = c; break; }
           }
           if (chosen === null) {
             // as a last-ditch try, preserve original behaviour but log candidates
             this.onLog(`[CPU] NULL SLIDE at 0x${this.pc.toString(16)} - a0=0x${ra.toString(16)} candidates=${candidates.map(x=>"0x"+x.toString(16)).join(',')}`);
             // attempt the original mapping once to see if it helps
             const mapped = (ra & 0x3FFFFFFF) | (this.pc & 0xC0000000);
             if (this.isPCValid(mapped)) {
               chosen = mapped;
             }
           }

           if (chosen !== null) {
             this.onLog(`[CPU] NULL SLIDE recovered: returning to 0x${chosen.toString(16)} (a0=0x${ra.toString(16)})`);
             this.windowBase = (this.windowBase - ((ra >>> 30)) + 16) % 16;
             this.pc = chosen >>> 0;
             this.nullSlideCount = 0;
             return;
           }
         }
         this.haltUnknown(0, 0, 0, `NULL SLIDE DETECTED`);
         return;
       }
     } else {
       this.nullSlideCount = 0;
     }

    try {
      const t_24 = (b0 >> 4) & 0x0F;
      const s_24 = (b1 & 0x0F);
      const r_24 = (b1 >> 4) & 0x0F;

      // Handle Hardware Loops
      if (this.pc === this.lend && this.lcount > 0) {
         this.lcount--;
         this.pc = this.lbeg;
         return;
      }

      switch (op0) {
        case 0x00: // op0=0: QRST Class (RRR and Patterns)
          {
            const op1 = (b0 >> 4) & 0x0F;
            if (op1 === 0) {
              const op2 = (b2 >> 4) & 0x0F;
              const op1_rrr = b2 & 0x0F; 
              if (b1 === 0x08 && b2 === 0x00) { // RETW
                const ra = this.getA(0);
                const windowIncr = (ra >>> 30);
                this.windowBase = (this.windowBase - windowIncr + 16) % 16;
                this.pc = (ra & 0x3FFFFFFF) | (this.pc & 0xC0000000);
                this.addTrace(`RETW (Incr=${windowIncr*4})`, b0, b1, b2); return;
              }
              if (op1_rrr === 0x0) { // Arithmetic
                if (op2 === 0x0) { this.setA(r_24, (this.getA(s_24) + this.getA(t_24)) >>> 0); this.addTrace(`ADD a${r_24}, a${s_24}, a${t_24}`, b0, b1, b2); }
                else if (op2 === 0x1) { this.setA(r_24, (this.getA(s_24) + this.getA(t_24)) >>> 0); this.addTrace(`ADDS a${r_24}, a${s_24}, a${t_24}`, b0, b1, b2); }
                else if (op2 === 0x8) { this.setA(r_24, ((this.getA(s_24) << 1) + this.getA(t_24)) >>> 0); this.addTrace(`ADDX2 a${r_24}, a${s_24}, a${t_24}`, b0, b1, b2); }
                else if (op2 === 0x9) { this.setA(r_24, ((this.getA(s_24) << 2) + this.getA(t_24)) >>> 0); this.addTrace(`ADDX4 a${r_24}, a${s_24}, a${t_24}`, b0, b1, b2); }
                else if (op2 === 0xa) { this.setA(r_24, ((this.getA(s_24) << 3) + this.getA(t_24)) >>> 0); this.addTrace(`ADDX8 a${r_24}, a${s_24}, a${t_24}`, b0, b1, b2); }
                else if (op2 === 0xc) { this.setA(r_24, (this.getA(s_24) - this.getA(t_24)) >>> 0); this.addTrace(`SUB a${r_24}, a${s_24}, a${t_24}`, b0, b1, b2); }
              } else if (op1_rrr === 0x3) { // Logic
                if (op2 === 0x0) { this.setA(r_24, (this.getA(s_24) & this.getA(t_24)) >>> 0); this.addTrace(`AND a${r_24}, a${s_24}, a${t_24}`, b0, b1, b2); }
                else if (op2 === 0x1) { this.setA(r_24, (this.getA(s_24) | this.getA(t_24)) >>> 0); this.addTrace(`OR a${r_24}, a${s_24}, a${t_24}`, b0, b1, b2); }
                else if (op2 === 0x2) { this.setA(r_24, (this.getA(s_24) ^ this.getA(t_24)) >>> 0); this.addTrace(`XOR a${r_24}, a${s_24}, a${t_24}`, b0, b1, b2); }
              } else if (op1_rrr === 0x1) { // Shift
                 if (op2 === 0x3) {
                    const sr = b1; 
                    if (sr === 3) this.sar = this.getA(s_24) & 31;
                    else if (sr === 72) this.windowBase = this.getA(s_24) % 16;
                    else if (sr === 73) this.windowStart = this.getA(s_24);
                    else if (sr === 230) this.ps = this.getA(s_24); // PS
                    else if (sr === 4) this.lbeg = this.getA(s_24);
                    else if (sr === 5) this.lend = this.getA(s_24);
                    else if (sr === 6) this.lcount = this.getA(s_24);
                    this.addTrace(`WSR SR(${sr}), a${s_24}`, b0, b1, b2);
                 } else if (op2 === 0x2) { // RSR
                    const sr = b1;
                    let val = 0;
                    if (sr === 3) val = this.sar;
                    else if (sr === 72) val = this.windowBase;
                    else if (sr === 73) val = this.windowStart;
                    else if (sr === 230) val = this.ps;
                    else if (sr === 4) val = this.lbeg;
                    else if (sr === 5) val = this.lend;
                    else if (sr === 6) val = this.lcount;
                    this.setA(s_24, val); this.addTrace(`RSR a${s_24}, SR(${sr})`, b0, b1, b2);
                 } else {
                    const sa = (b1 >> 4) | ((b0 & 0x10) ? 16 : 0);
                    if (op2 === 0x0) { this.setA(r_24, (this.getA(s_24) << (sa & 31)) >>> 0); this.addTrace(`SLLI a${r_24}, a${s_24}, ${sa}`, b0, b1, b2); }
                    else if (op2 === 0x1) { this.setA(r_24, (this.getA(s_24) >>> (sa & 31))); this.addTrace(`SRLI a${r_24}, a${s_24}, ${sa}`, b0, b1, b2); }
                    else if (op2 === 0x2) { this.setA(r_24, (this.getA(s_24) >> (sa & 31)) >>> 0); this.addTrace(`SRAI a${r_24}, a${s_24}, ${sa}`, b0, b1, b2); }
                 }
              }
            } else {
               const op2 = (b1 >> 4) & 0x0F;
               if (op1 === 0x0 && op2 === 0x0) {
                  const op3 = b2 & 0x0F;
                  if (op3 === 0xD) { // JX
                     this.pc = this.getA(s_24); this.addTrace(`JX a${s_24}`, b0, b1, b2); return;
                  } else if (op3 >= 0xC) { // CALLX
                     const win = (op3 === 0xC ? 1 : (op3 === 0xE ? 2 : 3));
                     const ra = (this.pc + 3) & 0x3FFFFFFF; this.setA(win*4, (win << 30) | ra);
                     this.pc = this.getA(s_24); this.windowBase = (this.windowBase + win) % 16;
                     this.addTrace(`CALLX${win*4} a${s_24}`, b0, b1, b2); return;
                  }
               }
            }
          }
          break;

        case 0x01: // L32R / CALL0 / J (24-bit)
          {
             const sub = (b0 >> 4) & 0x3;
             const imm18 = this.getImm18(b0, b1, b2);
             if (sub === 0) { // L32R
                const imm16 = (b2 << 8) | b1;
                const lptr = (((this.pc + 3) & ~3) + (0xFFFC0000 | (imm16 << 2))) >>> 0;
                this.setA(t_24, this.read32(lptr)); this.addTrace(`L32R a${t_24}, [0x${lptr.toString(16)}]`, b0, b1, b2);
             } else if (sub === 1) { // CALL0
                const target = ((this.pc & ~3) + 4 + (imm18 << 2)) >>> 0;
                this.setA(0, this.pc + 3); this.pc = target; this.addTrace(`CALL0 0x${target.toString(16)}`, b0, b1, b2); return;
             } else if (sub === 2) { // J (Immediate)
                const target = (this.pc + 4 + imm18) >>> 0;
                this.pc = target; this.addTrace(`J.IMM 0x${target.toString(16)}`, b0, b1, b2); return;
             }
          }
          break;

        case 0x02: // ADDI / MOVI
          if (s_24 === 0) { // MOVI
            const imm8 = (b2 << 24) >> 24; this.setA(t_24, imm8); this.addTrace(`MOVI a${t_24}, ${imm8}`, b0, b1, b2);
          } else { // ADDI
            const imm8 = (b2 << 24) >> 24; this.setA(t_24, (this.getA(s_24) + imm8) >>> 0); this.addTrace(`ADDI a${t_24}, a${s_24}, ${imm8}`, b0, b1, b2);
          }
          break;

        case 0x03: // Memory (RRI8)
          {
            const imm8 = b2;
            const addr = (this.getA(s_24) + (imm8 << (r_24 === 0x8 ? 2 : (r_24 === 0x0 || r_24 === 0x4 ? 1 : 0)))) >>> 0;
            if (r_24 === 0x0) { this.setA(t_24, this.read16(addr)); this.addTrace(`L16UI a${t_24}, a${s_24}, ${imm8 << 1}`, b0, b1, b2); }
            else if (r_24 === 0x1) { let val = this.read16(addr); if (val & 0x8000) val |= 0xFFFF0000; this.setA(t_24, val); this.addTrace(`L16SI a${t_24}, a${s_24}, ${imm8 << 1}`, b0, b1, b2); }
            else if (r_24 === 0x2) { this.setA(t_24, this.read8(addr)); this.addTrace(`L8UI a${t_24}, a${s_24}, ${imm8}`, b0, b1, b2); }
            else if (r_24 === 0x4) { this.write16(addr, this.getA(t_24)); this.addTrace(`S16I a${t_24}, a${s_24}, ${imm8 << 1}`, b0, b1, b2); }
            else if (r_24 === 0x6) { this.write8(addr, this.getA(t_24)); this.addTrace(`S8I a${t_24}, a${s_24}, ${imm8}`, b0, b1, b2); }
            else if (r_24 === 0x8) { this.setA(t_24, this.read32(addr)); this.addTrace(`L32I a${t_24}, a${s_24}, ${imm8 << 2}`, b0, b1, b2); }
            else if (r_24 === 0xC) { this.write32(addr, this.getA(t_24)); this.addTrace(`S32I a${t_24}, a${s_24}, ${imm8 << 2}`, b0, b1, b2); }
          }
          break;

        case 0x04: // EXTUI / LOOP
          {
            const op1 = (b0 >> 4) & 0x0F;
            if (op1 === 0x07) { // LOOP class
               const op2 = (b1 >> 4) & 0x0F;
               const imm8 = b2;
               if (op2 === 0x8) { // LOOP
                  this.lbeg = (this.pc + 4) >>> 0;
                  this.lend = (this.pc + 4 + imm8) >>> 0;
                  this.lcount = (this.getA(s_24) - 1) >>> 0;
                  this.addTrace(`LOOP a${s_24}, 0x${this.lend.toString(16)}`, b0, b1, b2);
               } else if (op2 === 0x9) { // LOOPNEZ
                  this.lbeg = (this.pc + 4) >>> 0;
                  this.lend = (this.pc + 4 + imm8) >>> 0;
                  this.lcount = (this.getA(s_24) - 1) >>> 0; // Simplified
                  this.addTrace(`LOOPNEZ a${s_24}, 0x${this.lend.toString(16)}`, b0, b1, b2);
               }
            } else { // EXTUI
               const shift = ((b1 >> 4) & 0xF) | ((b2 & 0x3) << 4);
               const mask = ((b2 >> 2) & 0xF) + 1;
               this.setA(t_24, (this.getA(s_24) >>> shift) & ((1 << mask) - 1));
               this.addTrace(`EXTUI a${t_24}, a${s_24}, ${shift}, ${mask}`, b0, b1, b2);
            }
          }
          break;

        case 0x05: // CALLx (Immediate)
          {
            const imm18 = this.getImm18(b0, b1, b2);
            const target = ((this.pc & ~3) + 4 + (imm18 << 2)) >>> 0;
            const windowCode = (b0 >> 4) & 0x3;
            const raIdx = windowCode * 4;
            const raValue = (windowCode << 30) | ((this.pc + 3) & 0x3FFFFFFF);
            this.setA(raIdx, raValue);
            this.windowBase = (this.windowBase + windowCode) % 16;
            this.pc = target; this.addTrace(`CALL${windowCode * 4} 0x${target.toString(16)}`, b0, b1, b2); return;
          }

        case 0x06: // ENTRY / J
          if (b0 === 0x36) {
             const imm12 = (b1 >> 4) | (b2 << 4);
             this.setA(1, this.getA(1) - imm12); this.addTrace(`ENTRY a1, ${imm12}`, b0, b1, b2);
          } else {
             const imm18 = this.getImm18(b0, b1, b2);
             const target = (this.pc + 4 + imm18) >>> 0;
             this.pc = target; this.addTrace(`J 0x${target.toString(16)}`, b0, b1, b2); return;
          }
          break;

        case 0x07: // BRANCH
          {
            const cond = (b1 >> 4) & 0xF;
            const imm8 = (b2 << 24) >> 24;
            const target = (this.pc + 4 + imm8) >>> 0;
            let taken = false;
            const valS = this.getA(s_24); const valT = this.getA(t_24);
            if (cond === 0x0 || cond === 0x1) taken = (valS === valT);
            else if (cond === 0x8) taken = ((valS & valT) === 0); // BNONE
            else if (cond === 0x9) taken = (valS !== valT);
            else if (cond === 0xa) taken = ((valS & valT) !== 0); // BANY
            else if (cond === 0x2) taken = (valS|0) < (valT|0);
            else if (cond === 0x3) taken = (valS>>>0) < (valT>>>0);
            else if (cond === 0x6) taken = (valS|0) >= (valT|0);
            else if (cond === 0x7) taken = (valS>>>0) >= (valT>>>0);
            else if (cond === 0xC) taken = (this.getA(s_24) & (1 << (b1 >> 4))) === 0; // BBCI
            else if (cond === 0xD) taken = (this.getA(s_24) & (1 << (b1 >> 4))) !== 0; // BBSI
            
            if (taken) { this.pc = target; this.addTrace(`BRANCH -> 0x${target.toString(16)}`, b0, b1, b2); return; }
          }
          break;

        case 0x08: // L32I.N
          {
            const t = (b0 >> 4) & 0xF; const s = b1 & 0xF; const imm4 = b1 >> 4;
            const addr = (this.getA(s) + (imm4 << 2)) >>> 0;
            this.setA(t, this.read32(addr)); this.addTrace(`L32I.N a${t}, [a${s} + ${imm4<<2}]`, b0, b1);
          }
          break;
        case 0x09: // S32I.N
          {
            const t = (b0 >> 4) & 0xF; const s = b1 & 0xF; const imm4 = b1 >> 4;
            const addr = (this.getA(s) + (imm4 << 2)) >>> 0;
            this.write32(addr, this.getA(t)); this.addTrace(`S32I.N a${t}, [a${s} + ${imm4<<2}]`, b0, b1);
          }
          break;
        case 0x0A: // ADD.N / MOV.N
          {
            const t = (b0 >> 4) & 0xF; const s = b1 & 0xF;
            if ((b1 >> 4) === 0) { this.setA(t, this.getA(s)); this.addTrace(`MOV.N a${t}, a${s}`, b0, b1); }
            else { this.setA(t, (this.getA(t) + this.getA(s)) >>> 0); this.addTrace(`ADD.N a${t}, a${s}`, b0, b1); }
          }
          break;
        case 0x0C: // ADDI.N
          {
            const t = (b0 >> 4) & 0xF; const s = b1 & 0xF;
            const imm4 = b1 >> 4; const imm = (imm4 === 0) ? -1 : imm4;
            this.setA(t, (this.getA(s) + imm) >>> 0); this.addTrace(`ADDI.N a${t}, a${s}, ${imm}`, b0, b1);
          }
          break;
        case 0x0D: // RETW.N
          if (b0 === 0x1D && b1 === 0xF0) {
            const ra = this.getA(0);
            const windowIncr = (ra >>> 30);
            this.windowBase = (this.windowBase - windowIncr + 16) % 16;
            this.pc = (ra & 0x3FFFFFFF) | (this.pc & 0xC0000000);
            this.addTrace(`RETW.N (Incr=${windowIncr*4})`, b0, b1); return;
          }
          this.haltUnknown(b0, b1, 0, `Unknown Narrow op0=D`); return;
        case 0x0E: // MOVI.N
          {
             const t = (b0 >> 4) & 0xF;
             const imm = (b1 & 0x7F); 
             this.setA(t, imm); this.addTrace(`MOVI.N a${t}, ${imm}`, b0, b1);
          }
          break;
        case 0x0F: // Density 0xF (Potential Bypass)
          this.onLog(`[CPU] Bypass Density 0xF at 0x${this.pc.toString(16)}: [${b0.toString(16)} ${b1.toString(16)}]`);
          this.addTrace(`BYPASS_0xF`, b0, b1);
          break;

        default: this.haltUnknown(b0, b1, b2); return;
      }
      this.pc = (this.pc + instrLen) >>> 0;
    } catch (e) {
      this.isRunning = false;
      this.onLog(`[CPU] INTERPRETER ERROR: ${e}\nTrace:\n${this.trace.join('\n')}`);
    }
  }

  private haltUnknown(b0: number, b1: number, b2: number, reason?: string) {
    this.isRunning = false;
    const msg = reason || `Unknown Opcode 0x${b0.toString(16)} 0x${b1.toString(16)} 0x${b2.toString(16)}`;
    this.onLog(`[CPU] STRICT HALT: ${msg} at 0x${this.pc.toString(16)}`);
    this.onLog(this.dumpRegisters());
    this.onLog(`[CPU] Trace:\n${this.trace.join('\n')}`);

    // Dump memory around PC to help debug NULL SLIDE / missing segments
    try {
      const before = 32;
      const after = 96;
      const startAddr = this.pc >= before ? this.pc - before : 0;
      const endAddr = this.pc + after;
      const lines: string[] = [];
      for (let addr = startAddr; addr <= endAddr && lines.length < 512; addr++) {
        const a = addr >>> 0;
        const b = this.read8(a) >>> 0;
        lines.push(`0x${a.toString(16).padStart(8, '0')}: ${b.toString(16).padStart(2, '0')}`);
      }
      this.onLog(`[CPU] Memory around PC:\n${lines.join('\n')}`);
    } catch (e) {
      this.onLog(`[CPU] Memory dump failed: ${e}`);
    }

    try {
      const segs = this.loadedSegments.map(s => `0x${s.start.toString(16)}-0x${(s.end - 1).toString(16)}`);
      this.onLog(`[CPU] Loaded segments: ${segs.join(', ')}`);
    } catch (e) {
      // ignore
    }
  }

  private read16(addr: number): number { return (this.read8(addr) | (this.read8(addr + 1) << 8)) >>> 0; }
  private write16(addr: number, val: number) { this.write8(addr, val & 0xFF); this.write8(addr + 1, (val >> 8) & 0xFF); }
  private write8(addr: number, val: number) {
    const res = this.resolveAddress(addr);
    if (res) {
      res.buffer[res.offset] = val & 0xFF;
      if (res.buffer === this.frameBuffer) this.onDisplayUpdate(this.frameBuffer);
    }
  }
  private read32(addr: number): number {
    addr >>>= 0; if (this.hal) { const v = this.hal.handleRead(addr); if (v !== null) return v; }
    return (this.read8(addr) | (this.read8(addr+1) << 8) | (this.read8(addr+2) << 16) | (this.read8(addr+3) << 24)) >>> 0;
  }
  private write32(addr: number, val: number) {
    addr >>>= 0; if (this.hal && this.hal.handleWrite(addr, val)) return;
    const res = this.resolveAddress(addr);
    if (res && res.offset + 4 <= res.buffer.length) {
      res.buffer[res.offset] = val & 0xFF; res.buffer[res.offset+1] = (val >> 8) & 0xFF;
      res.buffer[res.offset+2] = (val >> 16) & 0xFF; res.buffer[res.offset+3] = (val >> 24) & 0xFF;
      if (res.buffer === this.frameBuffer) this.onDisplayUpdate(this.frameBuffer);
    }
  }

  public onLog: (msg: string) => void = () => {};
  public onDisplayUpdate: (buffer: Uint8Array) => void = () => {};

  start() { 
    this.isRunning = true; 
    this.instructionsExecuted = 0; 
    this.lastStatTime = Date.now(); 
    this.runLoop(); 
  }

  stop() {
    this.isRunning = false;
  }

  private runLoop() {
    if (!this.isRunning) return;
    for (let i = 0; i < 50000; i++) { if (!this.isRunning) break; this.step(); this.instructionsExecuted++; }
    if (this.isRunning) setTimeout(() => this.runLoop(), 0);
  }
}
