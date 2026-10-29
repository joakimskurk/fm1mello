// Screenshots of reference/skurkymello.html (the look the FM-1's screen should borrow): the whole page, stopped and
// playing. node tools/tone-ref/shoot.mjs OUT_DIR
import puppeteer from "puppeteer-core";
import { existsSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const out = process.argv[2] || ".";
const browserPath = [process.env.CHROME, "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
  "/Applications/Chromium.app/Contents/MacOS/Chromium", "/Applications/Brave Browser.app/Contents/MacOS/Brave Browser",
  "/usr/bin/google-chrome", "/usr/bin/chromium"].filter(Boolean).find((p) => existsSync(p));
const browser = await puppeteer.launch({ executablePath: browserPath, headless: true,
  args: ["--no-first-run", "--autoplay-policy=no-user-gesture-required"] });
try {
  const page = await browser.newPage();
  await page.setViewport({ width: 1200, height: 900, deviceScaleFactor: 1 });
  await page.goto(pathToFileURL(join(here, "..", "..", "reference", "skurkymello.html")).href, { waitUntil: "load", timeout: 90000 });
  await page.waitForFunction(() => document.querySelectorAll("button").length > 5, { timeout: 60000 });
  await new Promise((r) => setTimeout(r, 1500));
  await page.screenshot({ path: join(out, "html_stopped.png"), fullPage: true });
  const play = await page.$$eval("button", (bs) => bs.findIndex((b) => /PLAY|▶/.test(b.textContent || "")));
  if (play >= 0) {
    const bs = await page.$$("button");
    await bs[play].click();
    await new Promise((r) => setTimeout(r, 1600));
    await page.screenshot({ path: join(out, "html_playing.png"), fullPage: true });
  }
  const colours = await page.evaluate(() => {
    const seen = {};
    for (const el of document.querySelectorAll("*")) {
      const cs = getComputedStyle(el);
      for (const v of [cs.color, cs.backgroundColor, cs.borderTopColor])
        if (v && v !== "rgba(0, 0, 0, 0)") seen[v] = (seen[v] || 0) + 1;
    }
    return Object.entries(seen).sort((a, b) => b[1] - a[1]).slice(0, 30);
  });
  console.log(JSON.stringify(colours));
} finally {
  await browser.close();
}
