import React, { useState, useEffect, useRef } from 'react';
import { 
  Play, 
  Square, 
  RotateCcw, 
  Terminal, 
  Cpu, 
  Layers, 
  Download,
  Activity,
  Settings
} from 'lucide-react';
import { EmulatorEngine } from './emulator/engine';
import './App.css';


function App() {
  const [isRunning, setIsRunning] = useState(false);
  const [logs, setLogs] = useState<{ time: string; level: string; msg: string }[]>([]);
  const [binLoaded, setBinLoaded] = useState(false);
  const [fps, setFps] = useState(0);
  const terminalRef = useRef<HTMLDivElement>(null);
  const engineRef = useRef<EmulatorEngine>(new EmulatorEngine());
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const fpsCounterRef = useRef({ frames: 0, lastTime: Date.now() });
  const overlayTickRef = useRef(false);
  const overlayIntervalRef = useRef<number | null>(null);

  // Auto-scroll terminal
  useEffect(() => {
    if (terminalRef.current) {
      terminalRef.current.scrollTop = terminalRef.current.scrollHeight;
    }
  }, [logs]);

  // FPS counter
  useEffect(() => {
    if (!isRunning) return;
    
    const interval = setInterval(() => {
      const now = Date.now();
      const elapsed = now - fpsCounterRef.current.lastTime;
      if (elapsed >= 1000) {
        setFps(Math.round((fpsCounterRef.current.frames * 1000) / elapsed));
        fpsCounterRef.current.frames = 0;
        fpsCounterRef.current.lastTime = now;
      }
    }, 100);

    return () => clearInterval(interval);
  }, [isRunning]);

  // Subscribe to external frame stream (WebSocket) if available
  useEffect(() => {
    const ports = [8081, 8082];
    let ws: WebSocket | null = null;
    let idx = 0;

    const connect = () => {
      if (idx >= ports.length) return;
      const port = ports[idx++];
      try {
        ws = new WebSocket(`ws://localhost:${port}`);
        (ws as any).binaryType = 'arraybuffer';
        ws.onopen = () => setLogs(prev => [...prev, { time: new Date().toLocaleTimeString(), level: 'INFO', msg: `Connected to frame websocket:${port}` }]);
        ws.onmessage = (ev: MessageEvent) => {
          try {
            const data = ev.data as ArrayBuffer;
            const bin = new Uint8Array(data);
            fpsCounterRef.current.frames++;
            if (canvasRef.current) {
              const ctx = canvasRef.current.getContext('2d');
              if (ctx && bin.length >= 64 * 32 * 4) {
                const imgData = ctx.createImageData(64, 32);
                imgData.data.set(bin.slice(0, 64 * 32 * 4));
                ctx.putImageData(imgData, 0, 0);
              }
            }
          } catch (e) {
            // ignore malformed frames
          }
        };
        ws.onerror = () => {
          setLogs(prev => [...prev, { time: new Date().toLocaleTimeString(), level: 'WARN', msg: `Frame websocket error (port ${port})` }]);
          try { ws?.close(); } catch (e) {}
        };
        ws.onclose = () => {
          setLogs(prev => [...prev, { time: new Date().toLocaleTimeString(), level: 'INFO', msg: `Frame websocket closed (port ${port})` }]);
          // try next port
          setTimeout(connect, 200);
        };
      } catch (e) {
        setTimeout(connect, 200);
      }
    };

    connect();
    return () => { try { ws?.close(); } catch (e) {} };
  }, []);

  const addLog = (msg: string, level: string = 'INFO') => {
    const time = new Date().toLocaleTimeString([], { hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit' });
    setLogs(prev => [...prev, { time, level, msg }]);
  };

  const handleStart = () => {
    if (!binLoaded) {
      addLog("Please load a firmware binary first.", "WARN");
      return;
    }
    setIsRunning(true);
    addLog("Emulation started.");
    fpsCounterRef.current.frames = 0;
    fpsCounterRef.current.lastTime = Date.now();
    
    // Start overlay heartbeat for blank-frame feedback
    overlayIntervalRef.current = window.setInterval(() => { overlayTickRef.current = !overlayTickRef.current; }, 500);

    engineRef.current.start(
      (m) => addLog(m),
      (buffer: Uint8Array) => {
        fpsCounterRef.current.frames++;
        if (!canvasRef.current) return;
        const ctx = canvasRef.current.getContext('2d');
        if (!ctx) return;
        if (buffer.length >= 64 * 32 * 4) {
          const imgData = ctx.createImageData(64, 32);
          imgData.data.set(buffer.slice(0, 64 * 32 * 4));
          ctx.putImageData(imgData, 0, 0);
          // If the frame is essentially blank (all black), draw a visible overlay
          let nonZero = false;
          for (let i = 0; i < 64 * 32 * 4; i += 4) {
            if (imgData.data[i] !== 0 || imgData.data[i+1] !== 0 || imgData.data[i+2] !== 0) { nonZero = true; break; }
          }
          if (!nonZero) {
            const w = 64, h = 32;
            const boxW = 8, boxH = 8;
            const startX = Math.max(0, Math.floor((w - boxW) / 2));
            const startY = Math.max(0, Math.floor((h - boxH) / 2));
            ctx.fillStyle = overlayTickRef.current ? '#00DC00' : '#004400';
            ctx.fillRect(startX, startY, boxW, boxH);
          }
        }
      }
    );
  };

  const handleStop = () => {
    setIsRunning(false);
    engineRef.current.stop();
    if (overlayIntervalRef.current) { clearInterval(overlayIntervalRef.current); overlayIntervalRef.current = null; }
    addLog("Emulation stopped.");
  };

  const handleReset = () => {
    handleStop();
    setTimeout(() => {
      // Optionally reload binary and restart
      if (binLoaded) {
        handleStart();
      }
    }, 100);
    addLog("Emulation reset.");
  };

  const onDrop = async (e: React.DragEvent) => {
    e.preventDefault();
    const files = Array.from(e.dataTransfer.files);
    const file = files.find(f => f.name.endsWith('.bin'));
    if (file) {
      try {
        const buffer = await file.arrayBuffer();
        const success = await engineRef.current.loadBinary(buffer, (m) => addLog(m));
        if (success) {
          setBinLoaded(true);
          addLog(`Loaded binary: ${file.name}`);
        } else {
          addLog("Failed to parse binary header.", "ERROR");
        }
      } catch (error) {
        addLog(`Failed to read file: ${error}`, "ERROR");
      }
    } else {
      addLog("Invalid file type. Please upload a .bin file.", "ERROR");
    }
  };

  return (
    <div className="dashboard" onDragOver={e => e.preventDefault()} onDrop={onDrop}>
      {/* Header */}
      <header className="glass-panel">
        <div className="logo-section">
          <Cpu className="icon accent" />
          <h1>Toaster<span>Emulator</span></h1>
        </div>
        <div className="status-badge">
          <div className={`dot ${isRunning ? 'pulse' : ''}`}></div>
          <span>{isRunning ? 'Running' : 'Ready'}</span>
          {fps > 0 && <span className="fps-indicator">{fps} FPS</span>}
        </div>
        <div className="header-actions">
          <button className="btn-icon"><Settings size={18} /></button>
        </div>
      </header>

      <main>
        {/* Left Sidebar - Controls */}
        <aside className="controls-panel glass-panel">
          <section>
            <h2>Controls</h2>
            <div className="button-grid">
              <button 
                onClick={isRunning ? handleStop : handleStart} 
                className={`btn-primary ${isRunning ? 'stop' : 'start'}`}
              >
                {isRunning ? <Square size={20} /> : <Play size={20} />}
                {isRunning ? 'Stop' : 'Start'}
              </button>
              <button onClick={handleReset} className="btn-secondary">
                <RotateCcw size={20} />
                Reset
              </button>
            </div>
          </section>

          <section>
            <h2>System Stats</h2>
            <div className="stats-list">
              <div className="stat-item">
                <Activity size={16} />
                <span>FPS: {fps}</span>
              </div>
              <div className="stat-item">
                <Layers size={16} />
                <span>Cores: 2</span>
              </div>
            </div>
          </section>

          <section className="upload-section">
            <input 
              type="file" 
              id="bin-file" 
              accept=".bin" 
              style={{ display: 'none' }}
              onChange={(e) => {
                const file = e.target.files?.[0];
                if (file) {
                  file.arrayBuffer().then(buffer => {
                    engineRef.current.loadBinary(buffer, (m) => addLog(m)).then(success => {
                      if (success) {
                        setBinLoaded(true);
                        addLog(`Loaded binary: ${file.name}`);
                      } else {
                        addLog("Failed to parse binary header.", "ERROR");
                      }
                    });
                  });
                }
              }}
            />
            <label htmlFor="bin-file" style={{ cursor: 'pointer', width: '100%' }}>
              <div className={`drop-zone ${binLoaded ? 'success' : ''}`}>
                <Download size={32} />
                <p>{binLoaded ? "✓ Binary Loaded" : "📁 Drop or Click"}</p>
                <small style={{ fontSize: '0.75rem', opacity: 0.7 }}>firmware.bin</small>
              </div>
            </label>
          </section>
        </aside>

        {/* Center - Matrix Display */}
        <section className="display-area">
          <div className="matrix-container glass-panel">
            <div className="matrix-wrapper">
               <div className="canvas-overlay"></div>
               <canvas 
                 ref={canvasRef}
                 width="64" 
                 height="32"
                 className="led-canvas"
               ></canvas>
            </div>
            <div className="display-label">Hub75 LED Matrix Simulation (64x32)</div>
          </div>
          
          <div className="hardware-view glass-panel">
            <div className="esp32-s3-chip">
              <div className="chip-label">ESP32-S3</div>
              <div className="chip-antenna"></div>
              <div className="chip-pads"></div>
            </div>
            <div className="display-label">Virtual ESP32-S3 Core</div>
          </div>
        </section>

        {/* Right Sidebar - Serial Terminal */}
        <aside className="terminal-panel glass-panel">
          <div className="terminal-header">
            <Terminal size={18} />
            <h2>Serial Output</h2>
          </div>
          <div className="terminal-body mono" ref={terminalRef}>
            {logs.length === 0 && <div className="terminal-placeholder">Waiting for serial logs...</div>}
            {logs.map((log, i) => (
              <div key={i} className={`log-entry ${log.level.toLowerCase()}`}>
                <span className="log-time">[{log.time}]</span>
                <span className="log-level">{log.level}</span>
                <span className="log-msg">{log.msg}</span>
              </div>
            ))}
          </div>
        </aside>
      </main>
    </div>
  );
}

export default App;
