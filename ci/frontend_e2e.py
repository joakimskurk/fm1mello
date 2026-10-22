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

    # Real browser keyboard route in main.js: 0 = SEQ, 7 = HOME.
    page.keyboard.press('0'); page.wait_for_timeout(600)
    canvas.screenshot(path=str(out/'after_key_seq.png'))
    key_seq_hash=file_sha(out/'after_key_seq.png')
    status_after=page.locator('#status').inner_text()
    page.keyboard.press('7'); page.wait_for_timeout(500)
    canvas.screenshot(path=str(out/'after_key_home.png'))
    key_home_hash=file_sha(out/'after_key_home.png')

    # Real pointer route on the actual canvas. FM-1 panel geometry is 1280x820.
    # Bottom-row button centres from panel.rs: HOME (931,340), SEQ (1105,340), PLAY (1163,340).
    box=canvas.bounding_box()
    if not box: raise SystemExit('FAIL: panel canvas has no bounding box')
    def panel_click(x,y):
        canvas.click(position={'x': box['width']*x/1280.0, 'y': box['height']*y/820.0})
        page.wait_for_timeout(500)
    panel_click(1105,340)
    canvas.screenshot(path=str(out/'after_pointer_seq.png'))
    ptr_seq_hash=file_sha(out/'after_pointer_seq.png')
    panel_click(931,340)
    canvas.screenshot(path=str(out/'after_pointer_home.png'))
    ptr_home_hash=file_sha(out/'after_pointer_home.png')
    panel_click(1163,340)
    page.wait_for_timeout(700)
    canvas.screenshot(path=str(out/'after_pointer_play.png'))
    ptr_play_hash=file_sha(out/'after_pointer_play.png')

    page.keyboard.down('a'); page.wait_for_timeout(120); page.keyboard.up('a')
    page.wait_for_timeout(200)

    report={
      'url':url,
      'statusBefore':status_before,
      'statusAfter':status_after,
      'beforeSha256':before_hash,
      'keySeqSha256':key_seq_hash,
      'keyHomeSha256':key_home_hash,
      'pointerSeqSha256':ptr_seq_hash,
      'pointerHomeSha256':ptr_home_hash,
      'pointerPlaySha256':ptr_play_hash,
      'keyboardSeqChangedCanvas':before_hash != key_seq_hash,
      'keyboardHomeReturnedCanvas':before_hash == key_home_hash,
      'pointerSeqChangedCanvas':key_home_hash != ptr_seq_hash,
      'pointerHomeReturnedCanvas':key_home_hash == ptr_home_hash,
      'pointerPlayChangedCanvas':ptr_home_hash != ptr_play_hash,
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
        raise SystemExit('FAIL: real canvas click on SEQ did not change Mello canvas')
    if ptr_home_hash == ptr_play_hash:
        raise SystemExit('FAIL: real canvas click on PLAY did not change Mello canvas')
    print('PASS: real Chromium keyboard + pointer frontend routes control Mello')
    browser.close()
