import WebSocket from 'ws';

const port = process.env.STREAM_PORT || 8082;
const url = `ws://localhost:${port}`;
console.log('Connecting to', url);
let received = 0;
const ws = new WebSocket(url);
ws.on('open', () => console.log('open'));
ws.on('message', (data) => {
  received++;
  console.log('frame', (data && data.length) ? data.length : 'unknown', 'bytes; count=', received);
  if (received >= 3) {
    console.log('received 3 frames — exiting');
    ws.close();
    process.exit(0);
  }
});
ws.on('error', (err) => { console.error('error', err); process.exit(2); });
setTimeout(() => {
  console.error('timeout waiting for frames');
  process.exit(3);
}, 15000);
