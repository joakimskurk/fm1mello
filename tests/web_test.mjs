// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 joakimskurk
// The page's player (web/app/): the firmware as WebAssembly, run here as the page's audio thread runs it. It boots
// (the splash, then HOME, its button lit and its steps glowing), is silent until PLAY, plays and stops, a step key
// picks its step and plays it (a press and release in one quantum still count), a section button turns the page and
// lights, SELECT turns the tempo, RNDMZR rolls, SAVE writes the flash, and the project comes back after a restart with
// that flash put back. Writes the screens it saw as PPM (OUT-*.ppm) to look at. After Star Distroyer's
// tests/web_test.mjs.
//   node tests/web_test.mjs build/web/skurkymello.wasm [OUT]
import { readFileSync, writeFileSync } from 'node:fs';

const [wasmPath, out = 'build/web/web_test'] = process.argv.slice(2);
if (!wasmPath) { console.error('usage: node tests/web_test.mjs WASM [OUT]'); process.exit(2); }
const bytes = readFileSync(wasmPath);
const BTN = { OCT_DOWN: 0, OCT_UP: 1, FX: 2, SEL: 3, ENV: 4, LFO: 5, EDIT: 6, GLO: 7, HOME: 8, SAVE: 9, ARP: 10, SEQ: 11, PLAY: 12, REC: 13 };
const ENC = { SELECT: 0, ALGORITHM: 1, K1: 2, K2: 3, K3: 4, K4: 5, PRESETS: 6 };
const WHITE = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26];   // keys 1-16
const RNDMZR = 22;                                                          // B9
let fails = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'}  ${what}`); if (!ok) fails++; };

async function boot(store) {
  const { instance } = await WebAssembly.instantiate(bytes, {});
  const x = instance.exports;
  if (store) new Uint8Array(x.memory.buffer, x.web_store(), x.web_store_size()).set(store);
  x.web_init();
  return x;
}
// n ms of sound (in 128-frame quanta, as a worklet asks): its RMS and peak
function run(x, ms) {
  let sum = 0, peak = 0, n = 0;
  for (let f = 0; f < ms * 44.1; f += 128) {
    x.web_run(128);
    const o = new Int32Array(x.memory.buffer, x.web_out_ptr(), 256);
    for (let i = 0; i < 256; i++) { sum += o[i] * o[i]; peak = Math.max(peak, Math.abs(o[i])); n++; }
  }
  return { rms: Math.sqrt(sum / n), peak };
}
const lcd = (x) => new Uint16Array(x.memory.buffer, x.web_lcd(), 240 * 240).slice();
function ppm(x, name) {
  const p = lcd(x), b = Buffer.alloc(15 + 240 * 240 * 3);
  b.write('P6 240 240 255\n');
  for (let i = 0; i < 57600; i++) {
    const c = p[i];
    b[15 + 3 * i] = ((c >> 11) * 255 / 31) | 0; b[16 + 3 * i] = (((c >> 5) & 63) * 255 / 63) | 0; b[17 + 3 * i] = ((c & 31) * 255 / 31) | 0;
  }
  writeFileSync(`${out}-${name}.ppm`, b);
}
const differ = (a, b) => { let d = 0; for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) d++; return d; };
const tap = (x, id) => { x.web_btn(id, 1); x.web_btn(id, 0); };   // down and up before the next quantum
const led = (x, id) => (x.web_leds(id < 32 ? 0 : 1) >>> (id % 32)) & 1;

const x = await boot();
run(x, 400);
ppm(x, 'splash');
const splash = lcd(x);
check(splash.some((c) => c !== splash[0]), 'the splash draws');
const quiet = run(x, 1800);
const home = lcd(x);
ppm(x, 'home');
check(differ(splash, home) > 5000, `after the splash, HOME (${differ(splash, home)} pixels changed)`);
check(quiet.peak < 200, `silent before PLAY (peak ${quiet.peak})`);
check(led(x, BTN.HOME) === 1 && (x.web_glow(0) | x.web_glow(1)) !== 0, 'HOME lit and its steps glowing from the start');

tap(x, BTN.PLAY);
let lit = 0;
let sum = 0;
for (let i = 0; i < 40; i++) { sum += run(x, 50).rms; lit |= led(x, BTN.PLAY); }
ppm(x, 'playing');
check(sum / 40 > 300, `PLAY plays (RMS ${(sum / 40).toFixed(0)})`);
check(lit === 1, 'the PLAY light on the beat');
tap(x, BTN.PLAY);
run(x, 2500);
const after = run(x, 300);
check(after.peak < 200, `PLAY again stops it (peak ${after.peak} after the tails)`);

const h0 = lcd(x);
x.web_key(WHITE[2], 1); x.web_key(WHITE[2], 0);                      // a press and release in one quantum
run(x, 200);
check(differ(h0, lcd(x)) > 50, `a tap on key 3 picks step 3 (${differ(h0, lcd(x))} pixels)`);
x.web_key(WHITE[2], 1);                                              // held, as a finger holds it
const heard = run(x, 150);
x.web_key(WHITE[2], 0);
check(heard.peak > 300, `held, it plays the step, stopped (peak ${heard.peak})`);
run(x, 2500);

const before = lcd(x);
tap(x, BTN.FX);
run(x, 200);
ppm(x, 'fx');
check(differ(before, lcd(x)) > 2000, 'FX turns the page (DISTORTION)');
check(led(x, BTN.FX) === 1, 'FX lit there');

tap(x, BTN.HOME);
run(x, 300);
const bpm0 = lcd(x);
x.web_enc(ENC.SELECT, 7);                                            // the tempo, 120 -> 127
run(x, 150);
ppm(x, 'tempo');
check(differ(bpm0, lcd(x)) > 500, 'SELECT turns the tempo (its value large over K1-K4)');
run(x, 2500);

const r0 = lcd(x);
x.web_key(RNDMZR, 1); x.web_key(RNDMZR, 0);
run(x, 400);
ppm(x, 'rolled');
check(differ(r0, lcd(x)) > 500, `RNDMZR rolls (${differ(r0, lcd(x))} pixels)`);
run(x, 2500);

x.web_enc(ENC.SELECT, -3);                                           // an edit (the tempo), then SAVE at once
run(x, 50);
const rev0 = x.web_store_rev();
tap(x, BTN.SAVE);
run(x, 1000);
check(x.web_store_rev() !== rev0, `SAVE writes the flash (rev ${rev0} -> ${x.web_store_rev()})`);
run(x, 2500);
ppm(x, 'saved');
const kept = lcd(x);
const store = new Uint8Array(x.memory.buffer, x.web_store(), x.web_store_size()).slice();

const y = await boot(store);                                         // a reload with the flash put back
run(y, 4500);
ppm(y, 'restart');
const back = lcd(y);
const fresh = await boot();
run(fresh, 4500);
check(differ(back, kept) < differ(lcd(fresh), kept) / 4,
  `after a restart the project is back (${differ(back, kept)} pixels off, ${differ(lcd(fresh), kept)} for a fresh one)`);

console.log(fails ? `web_test: ${fails} FAILED` : `web_test: all passed (${bytes.length} B of WebAssembly)`);
process.exit(fails ? 1 : 0);
