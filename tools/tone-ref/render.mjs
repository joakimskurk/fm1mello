// Renders every stimulus in stimuli.json through the HTML's sound chain (chain.js, Tone 14.9.17) in headless
// Chromium and writes 32-bit float WAVs to tests/golden/html/. The browser: $CHROME, else a Chrome, Chromium or
// Brave found in the usual places. Run: npm ci && npm run render [-- name ...]
import { readFileSync, writeFileSync, mkdirSync, existsSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import puppeteer from "puppeteer-core";

const here = dirname(fileURLToPath(import.meta.url));
const out = join(here, "..", "..", "tests", "golden", "html");
const spec = JSON.parse(readFileSync(join(here, "stimuli.json"), "utf8"));
const only = process.argv.slice(2);

function findBrowser() {
  const c = [
    process.env.CHROME,
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
    "/Applications/Chromium.app/Contents/MacOS/Chromium",
    "/Applications/Brave Browser.app/Contents/MacOS/Brave Browser",
    "/usr/bin/google-chrome", "/usr/bin/chromium", "/usr/bin/chromium-browser",
  ].filter(Boolean);
  const hit = c.find((p) => existsSync(p));
  if (!hit) throw new Error("no Chromium-family browser found: set CHROME=/path/to/chrome");
  return hit;
}

function wavFloat32(left, right, rate) {
  const n = left.length, data = Buffer.alloc(n * 8), h = Buffer.alloc(44);
  for (let i = 0; i < n; i++) { data.writeFloatLE(left[i], i * 8); data.writeFloatLE(right[i], i * 8 + 4); }
  h.write("RIFF", 0); h.writeUInt32LE(36 + data.length, 4); h.write("WAVE", 8); h.write("fmt ", 12);
  h.writeUInt32LE(16, 16); h.writeUInt16LE(3, 20); h.writeUInt16LE(2, 22); h.writeUInt32LE(rate, 24);
  h.writeUInt32LE(rate * 8, 28); h.writeUInt16LE(8, 32); h.writeUInt16LE(32, 34); h.write("data", 36);
  h.writeUInt32LE(data.length, 40);
  return Buffer.concat([h, data]);
}

const browser = await puppeteer.launch({
  executablePath: findBrowser(),
  headless: true,
  args: ["--no-first-run", "--no-default-browser-check", "--autoplay-policy=no-user-gesture-required"],
});
try {
  let version = "";
  mkdirSync(out, { recursive: true });
  for (const st of spec.stimuli) {
    if (only.length && !only.includes(st.name)) continue;
    const page = await browser.newPage(); /* a fresh Tone per stimulus: its noise buffers come from the seed */
    page.on("console", (m) => { if (m.type() === "error") console.error("page:", m.text()); });
    await page.setContent("<!doctype html><title>tone-ref</title>");
    await page.addScriptTag({ path: join(here, "chain.js") });
    await page.evaluate(() => seedRandom(12345));
    await page.addScriptTag({ path: join(here, "node_modules", "tone", "build", "Tone.js") });
    version = await page.evaluate(() => Tone.version);
    const [l, r] = await page.evaluate((s, sr, bpm) => renderStimulus(s, sr, bpm), st, spec.sample_rate, spec.bpm);
    const f = (b64) => new Float32Array(Uint8Array.from(Buffer.from(b64, "base64")).buffer);
    const L = f(l), R = f(r);
    let peak = 0;
    for (let i = 0; i < L.length; i++) peak = Math.max(peak, Math.abs(L[i]), Math.abs(R[i]));
    writeFileSync(join(out, `${st.name}.wav`), wavFloat32(L, R, spec.sample_rate));
    await page.close();
    console.log(`${st.name.padEnd(18)} ${st.seconds.toFixed(1)} s  peak ${(20 * Math.log10(peak || 1e-9)).toFixed(1)} dBFS`);
  }
  writeFileSync(join(out, "VERSION"), `Tone.js ${version}, ${await browser.version()}\n`);
} finally {
  await browser.close();
}
