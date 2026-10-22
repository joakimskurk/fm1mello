#!/usr/bin/env python3
import hashlib, json, os, sys, time
from pathlib import Path
from playwright.sync_api import sync_playwright

root = Path(__file__).resolve().parents[1]
pkg = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'build/skurkymello-0.1-dev.fwsc').resolve()
out = root / 'work/frontend-e2e'
out.mkdir(parents=True, exist_ok=True)
url = os.environ.get('MELLO_E2E_URL', 'http://127.0.0.1:8765/?mhz=100&latency=interactive')

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True, args=['--autoplay-policy=no-user-gesture-required'])
    page = browser.new_page(viewport={'width': 1400, 'height': 1000})
    console=[]
    errors=[]
    page.on('console', lambda m: console.append(f'{m.type}: {m.text}'))
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.goto(url, wait_until='networkidle')
    page.locator('#open').set_input_files(str(pkg))
    # Opening a local package before audio starts should leave it ready for Start.
    page.locator('#start').click()
    page.wait_for_timeout(3500)
    canvas=page.locator('#panel')
    before=canvas.screenshot(path=str(out/'before.png'))
    before_hash=hashlib.sha256(before).hexdigest()
    status_before=page.locator('#status').inner_text()

    # Real browser keyboard route in web/main.js: 0 = SEQ on the FM-1.
    page.keyboard.press('0')
    page.wait_for_timeout(600)
    after=canvas.screenshot(path=str(out/'after_seq.png'))
    after_hash=hashlib.sha256(after).hexdigest()
    status_after=page.locator('#status').inner_text()

    # HOME (7) should return to the original page; then exercise a note key ('a').
    page.keyboard.press('7')
    page.wait_for_timeout(400)
    home=canvas.screenshot(path=str(out/'after_home.png'))
    home_hash=hashlib.sha256(home).hexdigest()
    page.keyboard.down('a'); page.wait_for_timeout(120); page.keyboard.up('a')
    page.wait_for_timeout(300)

    report={
      'url':url,
      'statusBefore':status_before,
      'statusAfter':status_after,
      'beforeSha256':before_hash,
      'afterSeqSha256':after_hash,
      'afterHomeSha256':home_hash,
      'seqChangedCanvas':before_hash != after_hash,
      'homeReturnedCanvas':before_hash == home_hash,
      'console':console,
      'pageErrors':errors,
    }
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
    if errors:
        raise SystemExit('FAIL: frontend page errors: '+ '; '.join(errors))
    if before_hash == after_hash:
        raise SystemExit('FAIL: real browser key 0/SEQ did not change the rendered FM-1 canvas')
    print('PASS: real Chromium frontend key route changed Mello UI')
    browser.close()
