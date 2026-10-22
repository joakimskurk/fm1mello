#!/usr/bin/env python3
import hashlib, json, os, sys
from pathlib import Path
from playwright.sync_api import sync_playwright

root = Path(__file__).resolve().parents[1]
pkg = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'build/skurkymello-0.1-dev.fwsc').resolve()
out = root / 'work/frontend-e2e'
out.mkdir(parents=True, exist_ok=True)
url = os.environ.get('MELLO_E2E_URL', 'http://127.0.0.1:8765/?mhz=100&latency=interactive')

def file_sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True, args=['--autoplay-policy=no-user-gesture-required'])
    page = browser.new_page(viewport={'width': 1400, 'height': 1000})
    console=[]; errors=[]
    page.on('console', lambda m: console.append(f'{m.type}: {m.text}'))
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.goto(url, wait_until='networkidle')
    page.locator('#open').set_input_files(str(pkg))
    page.locator('#start').click()
    page.wait_for_timeout(3500)
    canvas=page.locator('#panel')
    canvas.screenshot(path=str(out/'before.png'))
    before_hash=file_sha(out/'before.png')
    status_before=page.locator('#status').inner_text()

    # Hash an intrinsic panel region directly from the canvas. The FM-1 LCD is at (370,42)
    # and drawn 2x from its 240x240 framebuffer. The first ~30 LCD rows contain Mello's
    # RUN/STOP header, isolated from the animated step/status area below.
    def header_hash():
        return page.evaluate("""() => {
          const c=document.getElementById('panel'), d=c.getContext('2d').getImageData(370,42,480,64).data;
          let h=2166136261>>>0;
          for (let i=0;i<d.length;i++) { h ^= d[i]; h = Math.imul(h,16777619)>>>0; }
          return h>>>0;
        }""")
    stop_header_before = header_hash()

    # Real browser keyboard route in main.js: 0 = SEQ, 7 = HOME. Hold long enough for the
    # emulated 10 kHz matrix scanner to see the state, just like a human press.
    page.keyboard.down('0'); page.wait_for_timeout(120); page.keyboard.up('0'); page.wait_for_timeout(600)
    canvas.screenshot(path=str(out/'after_key_seq.png'))
    key_seq_hash=file_sha(out/'after_key_seq.png')
    status_after=page.locator('#status').inner_text()
    page.keyboard.down('7'); page.wait_for_timeout(120); page.keyboard.up('7'); page.wait_for_timeout(500)
    canvas.screenshot(path=str(out/'after_key_home.png'))
    key_home_hash=file_sha(out/'after_key_home.png')

    # Real pointer route. FM-1 panel intrinsic geometry is 1280x820.
    # Bottom-row button centres: HOME (931,340), SEQ (1105,340), PLAY (1163,340).
    box=canvas.bounding_box()
    if not box: raise SystemExit('FAIL: panel canvas has no bounding box')
    def panel_press(x,y,hold_ms=120,settle_ms=500):
        px=box['x'] + box['width']*x/1280.0
        py=box['y'] + box['height']*y/820.0
        page.mouse.move(px,py)
        page.mouse.down(); page.wait_for_timeout(hold_ms); page.mouse.up(); page.wait_for_timeout(settle_ms)
    panel_press(1105,340)
    canvas.screenshot(path=str(out/'after_pointer_seq.png'))
    ptr_seq_hash=file_sha(out/'after_pointer_seq.png')
    panel_press(931,340)
    canvas.screenshot(path=str(out/'after_pointer_home.png'))
    ptr_home_hash=file_sha(out/'after_pointer_home.png')

    # PLAY must be a true toggle on the physical panel: first press RUN, second press STOP.
    panel_press(1163,340,120,250)
    run_header = header_hash()
    canvas.screenshot(path=str(out/'after_pointer_play_run.png'))
    ptr_play_run_hash=file_sha(out/'after_pointer_play_run.png')
    page.wait_for_timeout(500)
    panel_press(1163,340,120,350)
    stop_header_after = header_hash()
    canvas.screenshot(path=str(out/'after_pointer_play_stop.png'))
    ptr_play_stop_hash=file_sha(out/'after_pointer_play_stop.png')

    page.keyboard.down('a'); page.wait_for_timeout(120); page.keyboard.up('a'); page.wait_for_timeout(200)

    report={
      'url':url,
      'statusBefore':status_before,
      'statusAfter':status_after,
      'beforeSha256':before_hash,
      'keySeqSha256':key_seq_hash,
      'keyHomeSha256':key_home_hash,
      'pointerSeqSha256':ptr_seq_hash,
      'pointerHomeSha256':ptr_home_hash,
      'pointerPlayRunSha256':ptr_play_run_hash,
      'pointerPlayStopSha256':ptr_play_stop_hash,
      'stopHeaderBefore':stop_header_before,
      'runHeader':run_header,
      'stopHeaderAfter':stop_header_after,
      'keyboardSeqChangedCanvas':before_hash != key_seq_hash,
      'keyboardHomeReturnedCanvas':before_hash == key_home_hash,
      'pointerSeqChangedCanvas':key_home_hash != ptr_seq_hash,
      'pointerHomeReturnedCanvas':key_home_hash == ptr_home_hash,
      'pointerPlayStarted':stop_header_before != run_header,
      'pointerPlayStopped':run_header != stop_header_after and stop_header_before == stop_header_after,
      'console':console,
      'pageErrors':errors,
    }
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
    if errors:
        raise SystemExit('FAIL: frontend page errors: '+ '; '.join(errors))
    if before_hash == key_seq_hash:
        raise SystemExit('FAIL: real browser keyboard 0/SEQ did not change Mello canvas')
    if key_home_hash == ptr_seq_hash:
        raise SystemExit('FAIL: real held canvas press on SEQ did not change Mello canvas')
    if stop_header_before == run_header:
        raise SystemExit('FAIL: first PLAY press did not switch STOP header to RUN')
    if run_header == stop_header_after or stop_header_before != stop_header_after:
        raise SystemExit('FAIL: second PLAY press did not restore STOP header')
    print('PASS: real Chromium keyboard + pointer routes, including PLAY start-stop toggle')
    browser.close()
