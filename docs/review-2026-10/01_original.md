# 01 — Original instrument analysis (SkurkyMello HTML)

Source: `review/original.html` (4960 lines; React 18 + Tone.js **v14** from unpkg, L20-23).
All line numbers (L…) refer to that file. Computed values are derived from the code's own mapping functions.

---

## 1. The core loop: the first 30 seconds

### What is on screen after load
- The boot screen hides after about 480 ms (L4947-4956). **Defaults are always hard-coded.** `loadSnapshot()` is **not** called on mount; it runs only from the LOAD button, from patch import or from slot recall (L2633, L2913, L3331). Every fresh load therefore gives the same known-good state. Only save slots, the slot sequencer and sample buffers persist (L431-462, L875-893).
- The header (L3083-3210) shows the big title, root `C`, `minor`, `16 steps`, `16p / 1.00bars`, a LOAD meter, the BPM readout (80, with -/+, TAP, slider 20-250, and 1/16, 1/8, 1/4 clock-div buttons), ROOT/OCT/SCALE selects, MIDI and TAB-sync toggles, STYLE, ⚙, RESET ALL, and a glowing green **▶ PLAY** button.
- The main body is one long page. Almost every panel starts **open** (L583-587): the pitch sequencer (always open), LOOP CYCLE SEQ (open), SYNTH VOICE (open), SAMPLE LAYER (open), SUB BASS VOICE (open), DOT MATRIX (always open), and TURING MACHINE (open). Only SAVE SLOTS starts collapsed.
- Pitch grid: 16 tall columns (`COL_H=236`, L2999). Each column shows its bar height, its **note name**, the step number, its pulse count with -/+ buttons, a gate glyph and three flag dots (accent, slide, sub) (L3531-3681). A STEP 1 DETAIL editor sits under the grid (L3684-3831).

### What plays by default (exact values)
| Item | Default | Line |
|---|---|---|
| Pattern | `pitch = i%7` (scale degrees 0..6 repeating): C D D# F G G# A# C D D# F G G# A# C D, an ascending C-minor run | L163-177 |
| Step defaults | pulses 1, ratchet 1, gate `single`, octave 0, slide/skip/accent/sub false, samp 1, prob 1.0 | L165-175 |
| Length | 16 steps, 1 section (A), repeats 1, `fwd`/`linear`, masterLen 0 | L411, L468-473, L596 |
| Tempo | 80 BPM, clockDiv 1 (1/16). One 16th = 0.1875 s, so the loop is 3.0 s (1 bar) | L416-417 |
| Key | root MIDI 60 (C4), scale `minor` [0,2,3,5,7,8,10], global oct 0, pitch span 2 oct | L485-487, L412 |
| Lead osc | `sawtooth`, mono `Tone.Synth` | L495, L1260 |
| Lead filter | cutoff 0.7 gives `80*200^0.7` = **3264 Hz**; res 0.2 gives Q = 0.2*15 = **3** (Web Audio dB-Q, linear ≈1.41 per stage), lowpass, **-24 dB/oct** (2 cascaded biquads) | L489-490, L1125, L1282-1285 |
| Lead env | A 5 ms, D 180 ms, S 0.4, R 120 ms | L491-494 |
| Slide | 50 ms portamento, applied only on slide steps | L488, L1594 |
| Sub-osc layer | ON, sine, -1 oct, level 0.22 | L501-504 |
| Sub bass voice | ON, mode `step`, but **no step has `sub=true`**, so it is silent until RANDOM or a sub flag is set | L525-538, L172 |
| Reverb | send 0.24, decay 6.0 s, pre-delay 12 ms, tone 0.62 (4201 Hz LPF), level 1.15, "hall" | L505-511 |
| Delay | send 0.12, sync ON, `8n` (0.375 s at 80 BPM), feedback 0.32, tone 0.72 (4699 Hz), level 0.75 | L512-518 |
| Distortion | amount 0 (off), mix 1, character 0.35, tone 0.82 (6318 Hz), level 0.9 | L519-523 |
| Master | 0.75, which is -2.5 dB on Tone destination; Limiter at -1 dB | L524, L1317, L1368 |
| LFO | OFF (2 Hz, depth 0.35, target filter) | L497-500 |
| TM / Loop-cycle / Dot matrix / Samples / X-SUB | all OFF or empty | L569-576, L562, L280-291, L541, L475 |

### Actions needed to reach a good-sounding loop
1. **PLAY**, or Space or `S` (L2157). You hear a 16th-note C-minor saw run with a 6 s reverb tail and 8th-note echoes straight away. **1 action.**
2. **⚡ RANDOM** (L3332). This makes a new in-key melody with rests, holds, a few ratchets, accents, octave jumps, slides and about 22% sub-bass hits (L2357-2374). The transport keeps running and the change is heard on the next tick (seqDirty rebuild, L984, L1765). **2 actions to a "real" groove.** Pressing it again gives another variation.
3. Optional fine-tuning, each change a single click while playing:
   - click a column height to set a note (L3002-3008, L3560);
   - right-click to skip a step;
   - drag CUTOFF;
   - pick a SCALE or ROOT, and the whole melody re-keys instantly because pitches are stored as scale degrees.

There is no menu diving or mode switching, and nothing has to be enabled before sound comes out.

---

## 2. Feature inventory

### 2.1 Main step sequencer
- **Length:** buttons 4,5,6,7,8,10,12,16,20,24,28,32 (L3295). The state always holds 32 steps (L410), and RANDOM fills all 32 (L2360). Lengthening the pattern therefore exposes already-randomized steps.
- **Pitch span:** 1, 2 or 3 octaves of scale degrees per column (L3306). Degree range is 0..`scaleLen*span-1`; minor with 2 oct gives 0..13.
- **Global OCT:** -2..+2 (L3316).
- **Per-step fields** (L163-177), edited in the column (L3531-3681) and in the STEP DETAIL panel (L3684-3831):
  - `pitch`: scale degree. Set by clicking the column (vertical position, quantized to degree, L3002-3008), by note-name buttons, or with ↑/↓ keys (L2166-2177).
  - `octave` -2..+2; `pulses` 1-8 (step length in 16ths; -/+ on the column); `ratchet` 1-8.
  - `gate`: single `—`, hold `▬`, rest `○`, multi `≡`. Clicking the glyph cycles it (L3644-3654).
  - `slide`, `accent`, `sub`, `skip` (right-click on the column, L3561); `prob` 0-1 in 0.05 steps (L3804); `samp` OFF/A-D.
- **Pitch math** (L1874-1882): `midi = root + scale[deg%len] + floor(deg/len)*12 + (global+step+loopcycle+TM)*12 + loopcycle.pitch`, clamped 12..108.
- **Gate/timing semantics** (L1582-1669):
  - `pulseDur = 60/(bpm*4)*clockDiv`.
  - single & ratchet 1: one note, length `totalDur*0.82` (clamped 0.02-4 s). The note lasts 82% of the whole step, including pulses.
  - ratchet >1: the pulse is split into N, each gate `subLen*0.72` (0.01-0.25 s); applies to every pulse.
  - multi & ratchet 1: re-gate on every pulse, length `pulseDur*0.4`.
  - hold: attack on pulse 0, release at last pulse + `0.88*pulseDur` (legato).
  - rest: silent.
  - Velocity is **0.72** normal and **0.95** accent (L1591). Accent changes only amplitude (+2.4 dB), not the filter.
- **Probability:** `Math.random() <= prob`, otherwise the step rests (L1821-1822). A TM gate override wins over this (L1812, L1822).
- **Sections A-F** (L3356-3512, L1715-1732, L2006-2017): segCount 1-6, split points (defaults [8,10,12,14,15]), repeats ×1-4, DIR FWD/REV, MODE LIN/P-P/RND (random reshuffles at each rebuild, L253), SKIP section. Each section has a color, and a ◉ marks the playing section.
- **RESET AFTER (masterLen):** off, 1, 2, 4, 8 or 16 bars (0/16/32/64/128/256 ticks), which force-resets to step 1 (L3491, L2002).
- **Loop info:** `N pulses = X bars` (L3507-3511).
- **Keys:** Space/S play/stop; ←/→ select step; ↑/↓ degree (L2154-2181).
- **No swing, no humanize, no micro-timing** anywhere. The clock is Tone Transport `scheduleRepeat(...,"16n")` (L1757, L2026); the 1/8 and 1/4 clock-divs skip ticks (L1762-1763).

### 2.2 Randomizer
- **⚡ RANDOM** (L2357-2374) applies these per-field distributions to all 32 steps:
  - pitch: uniform 0..pMax;
  - pulses: 60% 1, otherwise uniform 1-3;
  - ratchet: 78% 1, otherwise 60% 2 or uniform 2-5;
  - gate: 10% rest, 18% hold, 72% single (**never multi**);
  - octave: [0,0,0,+1,-1];
  - accent 12%, sub 22%, slide 8%, skip 5%;
  - prob: 90% 1.0, otherwise 0.40-0.90;
  - samp: 15% off, otherwise A-D.
- **Mask chips** PITCH PUL RAT GATE OCT ACC SUB SLD SKIP PROB SAMP, plus **ALL** and **NONE** (L3337-3354). All are on by default (L597-600). Example: NONE then PITCH re-rolls only the melody and keeps the rhythm.
- **◻ CLEAR** resets steps to the `i%7` defaults and resets the loop-cycle slots (L2375-2382).
- Other per-module randomizers: dot matrix **RND** per grid at 35% density (L4674-4681); TM **⚡ REROLL** (L4789-4795).

### 2.3 Sub-osc layer (part of the lead voice)
- A second `Tone.Synth` that shares the lead ADSR (L1264-1267) and fires on every lead note.
- Settings: wave sine/tri/sqr/saw; octave -2, -1 or UNI; level 0-1 (default 0.22) (L4102-4134).
- **Gain quirk:** level is applied twice, once as velocity `vel*0.22` (L1592) and once as gain `0.22` (L1272). The layer is therefore only about 5% of lead amplitude (≈ -26 dB): a faint body, not a second voice.

### 2.4 SUB BASS VOICE (separate voice) and X-SUB
- **Chain:**
  - `Tone.Synth` (sine; env A 3 ms, D 0.16, **S 0.24**, R 0.12; portamento 0.02)
  - → LPF -24 at `40*55^cut` (default 0.45 = **243 Hz**, Q 0.8)
  - → Distortion (drive 0, so wet 0), 2× oversampling
  - → Gain 0.42
  - → into the **same** `mix` as the lead, so it also passes through the main filter and the FX (L1268-1279, L1344-1347).
- **Trigger:** only at step start (`pulseIdx===0`), velocity 0.9. Duration `stepDur*pulses*subGate` (0.9) (L1671-1678, L1910).
- **Pitch:** `root + scaleTone + (global+loopOct+subOct(-1)+xOct)*12`, clamped 12..96 (L1939-1942). With defaults this is C3. It follows the key, scale and loop-cycle.
- **TRIG MODE** (L4476):
  - STEP: per-step SUB flag.
  - EUCLID: pulses 1-64 (default 6), rotation 0-64, over the whole loop length (L368-373, L1914).
  - MATRIX: SUB dot matrix.
  - TM GATE: Turing bool bit.
- **Params:** Octave -2..+1, Gate 0.15-1.2, Glide 0-200 ms, Decay 30-700 ms, Release 30-1200 ms, Level, LPF, Drive (L4523-4572).
- **X SUB SEQUENCER** (hidden behind an `X` button, L4574-4652): 2, 3 or 5 steps (default 5; only step 1 ON). Each step has ON/OFF, tone R/3/5 and oct -1/0/+1, plus a reset-on-wrap checkbox (default true). When on, it **overrides** the trigger mode and advances once per main step (L1922-1934). The active X step glows gold.
- Loop-cycle `subTone` R, 2-7 or 8 per slot is a scale-relative interval (L214-222).

### 2.5 LOOP CYCLE SEQ ("planetary")
- Settings (L3964-4076, L1961-1974):
  - 2-8 slots (default 4), OFF by default;
  - SYNC/FREE (default SYNC); RESET POS.
- Each slot has:
  - STEPS hold (1-128 ticks, presets 4/8/12/16/32; default 16 = 1 bar);
  - SUB TONE;
  - Key (FOLLOW or C..B);
  - Scale (FOLLOW or one of the 12);
  - Root ±12 semitones; Octave ±2; Pitch ±12 semitones.
- The current slot is **always applied**, even when OFF; `planActive` only gates auto-advance (L1781). Clicking a slot selects it live.
- Feedback: header dots show the active slot glowing gold, and the active card is outlined.

### 2.6 Dot Matrix (L4657-4764, L1824-1868)
- Four 4×4 grids: ACCENT (coral), FILTER (blue), RATCHET (purple), SUB (gold). All start empty.
- Per grid:
  - mode horizontal, vertical, snake or random (random reshuffles each cycle);
  - steps 1-16 (default 16);
  - RESET or NO RESET (reset gives position `tick % seqLen`);
  - FOLLOW or FREE (default FREE = advance per pulse tick);
  - depth 0-1 (default 1.0, used as a probability);
  - RND.
- Effects:
  - ACC: forces accent.
  - FLT: a "pluck" on the cutoff. Ramp to `base*(1+amt*2.6)` in **20 ms**, then exponential back to base by **160 ms** (L1897-1902).
  - RAT: +1 ratchet.
  - SUB: sub trigger, only in MATRIX mode.
- The active cell glows in the grid color whether or not the cell is lit (L4743-4757).

### 2.7 Turing Machine (L4766-4916, L349-366, L1795-1819)
- Defaults: OFF ("STATIC"/"EVOLVE"), LOCK, ⚡REROLL.
- Parameters:
  - MUTATION PROB 0-50% (default **10%**);
  - LOOP SIZE 3-16, 20, 24, 28, 32 (default **8**);
  - RATE ½×, 1× or 2×; STEP OFFSET ±31;
  - ROUTES GATE (default on), FILTER, OCTAVE, DECAY, each with a depth (default 0.35).
- Mutation per tick: with probability p, one random bit is set randomly; each float cell drifts ±0.3 with probability p (L352-361, L1798-1799).
- Routes:
  - Gate: sub trigger in TM mode. Also, with chance = depth, it overrides the gate to `multi` (bit=1) or `single` (L1811-1812).
  - Filter: cutoff × (1 + (f-0.5)*0.5*depth*4).
  - Octave: `round((f-0.5)*4*depth)` in ±2.
  - Decay: ±0.1*depth s on lead decay (L1813-1815, L1884).
- REGISTER VIEW shows a float bar and a clickable bool pad per cell. The active cell glows cyan (L4878-4908).

### 2.8 Save slots and slot sequencer (L3834-3962, L2884-2962)
- 8 visible slots (2-64 selectable). **W** writes; double-clicking W renames.
- **R** loads at once when stopped. While playing it **queues** until the next pattern wrap, and the slot blinks (L2937-2946, L1953-1960).
- Slots exclude FX and BPM (L2873-2882, L2903), so switching patterns keeps the sound and the tempo.
- SLOT SEQUENCER: 2-64 steps; each step holds a slot (or OFF) for 1/16 .. 4 bars (L227-230, L1975-1996). The loop-cycle config is preserved across slot-seq loads (L2896).

### 2.9 Sample layer (L4338-4452, L1399-1579)
- Four slots A-D. Each slot can IMPORT a file (decoded, then **normalized to 0.95 peak**, L191-206) or REC from the mic. The buffer persists in IndexedDB.
- Per slot:
  - PITCH TRACK/FIXED (root MIDI 60), LOOP;
  - Level 0-1.5 (0.7), Dist 0-1;
  - Trim start/end with a waveform canvas;
  - Coarse ±24 st, Fine ±100 ct;
  - ADSR (5 ms / 120 ms / 0.7 / 150 ms).
- The source is created per trigger, so SLIDE glides `playbackRate` (L1553-1561). The sample fires at step start through env → dist → gain → **mix**, the same filter and FX path as the synth.
- SEQ/FIX mode picks the slot per step or a fixed slot. The layer is OFF by default; importing a sample switches it ON.

### 2.10 Sound / filter / envelope / LFO (L4078-4336)
- Waveform: OFF, SAW, SQR, TRI, SIN, PLS. OFF mutes the synth but keeps samples and MIDI (L1645). PWM width 0.05-0.95.
- Filter cutoff 0-1 maps to 80 Hz-16 kHz (log). Resonance 0-1 maps to Q 0-15 dB per stage.
- ADSR ranges: A 1-500 ms, D 5-1000 ms, S 0-1, R 5-2000 ms. Slide 0-300 ms.
- LFO: ON/OFF; targets FLT, PIT, AMP, PWM. Rate is log 0.008-20 Hz, with presets 120/60/30/15 s. Depth 0-1.
  - Filter LFO is computed in a `requestAnimationFrame` loop (about 60 Hz). Its span is `base*(0.08+0.92d)`, with a larger upward swing if a matrix filter hit is active (L1373-1394).
  - Pitch LFO: ±(80+620d) cents, which is ±297 cents at the default depth.
  - Amp LFO: tremolo ±0.58d around the mix gain of 0.58.
  - PWM LFO: ±min(0.42d, headroom) (L1203-1233).
- All parameter changes use short `rampTo` ramps (50-100 ms) and are click-free (L987-1123).

### 2.11 FX (L4215-4333, L1286-1367)
- **Distortion:** `Tone.Distortion` with 4× oversampling. Drive = `amt^1.9*(0.28+0.52*char)`, clamped to 0.8 (max 0.462 at char 0.35) (L1138-1142). Followed by a post LPF -24 at `320*38^tone` and a level of 0.2-1.4.
- **Delay:** `Tone.TapeDelay||Tone.FeedbackDelay`. TapeDelay does not exist in v14, so this is a **FeedbackDelay** (L1290).
  - Sync divisions: 1m, 2n, 2t, 4n, 4t, 8n, 8t, 16n, 16t, 32n. Free time 30-1500 ms.
  - Feedback 0-0.95. Send 0-1.
  - Tone LPF -24 at `450*26^tone`, placed **after** the delay, outside the feedback loop, so repeats do not darken progressively.
  - Return 0-1.5. The time is re-synced 150 ms after a BPM change (L1079-1088).
- **Reverb:** `Tone.Reverb` exists in v14, so this is a **convolution reverb with a generated noise-decay IR** (decay 0.4-60 s, pre-delay 0-120 ms). Freeverb is only a fallback (L1303-1306).
  - The "Room size" slider only affects the Freeverb branch, so **it is a no-op in practice** (L1008-1011). Type is fixed to HALL.
  - Send curve: `0.03 + wet^1.2*0.95` (L1129-1134). The default 0.24 gives **0.201**.
  - Return path: LPF -24 at `700*18^tone`, HPF 24 Hz -24, return `level*1.05`. The default 1.15 gives 1.21.

### 2.12 Key / scale
- 12 scales: chromatic, minor, dorian, phrygian, lydian, mixolydian, pentatonic, min penta, diminished, whole tone, augmented, major (L98-111).
- Root C..B and octave Oct1-5 in the header (L3155-3173).
- Degrees are clamped when the scale shrinks (L2384-2396). Note names show sharps (D#, not Eb).

### 2.13 MIDI and sync
- **MIDI button** cycles OFF, SLAVE, MASTER (L3177):
  - SLAVE reads 0xF8 clock with trimmed-mean BPM detection and deadband, and follows Start/Continue/Stop (L2271-2315).
  - MASTER sends Start/Stop and a 24-ppqn clock, plus optional note out (lead ch 1, sub ch 2, length STEP or FIXED 20-2000 ms) (L2137-2147, L1613-1626, L3236-3277).
- **TAB SYNC:** BroadcastChannel master/slave with phase correction above 60 ms. Also listens to `skurkybeats-sync` (L2183-2253).
- **TAP** tempo uses the average of up to 8 taps within 3 s, range 20-250 (L2399-2410).
- **MIDI file export:** 3-track SMF at 96 PPQ, unrolled over loop-cycle slots (L2412-2591).

### 2.14 Patch I/O
- PATCH EXPORT writes a JSON file `{app, kind:"patch", version:1, snapshot}`. PATCH IMPORT accepts either a wrapped or a raw snapshot (L2605-2639).
- SAVE/LOAD use a localStorage snapshot. RESET ALL clears storage and reloads (L2641-2664).
- `buildSnap` lists every field (L2848-2871). `loadSnapshot` clamps every field and contains the ranges table used above (L2666-2846).

### 2.15 Visual feedback while playing
UI refresh runs every 20 ms (50 Hz) via `setInterval` (L2033-2087). While playing:
- the active column glows in its section color, with a notch at its pitch height (L3565-3615);
- the note name turns colored, and the pulse count lights on sub-pulses (L3634);
- the melody contour strip highlights the current step (L3516-3529);
- the section ◉ indicator, loop-cycle slot dots, TM active cell, X-SUB active step, dot-matrix active cells and the blinking queued slot all update;
- the LOAD % meter has a sparkline, and the header blinks "◉ PLAYING".
Every value has a numeric readout next to its slider (Hz, ms, %).

---

## 3. "Feel" factors

### 3.1 One-click actions
Each of these is a single click (or key):
- Play/stop.
- RANDOM; mask chip; ALL/NONE; CLEAR.
- Step length (12 buttons); pitch span; global octave; clock div; BPM ±1; TAP.
- Set a step pitch (one click at the desired height); skip (right-click); cycle gate; toggle accent/slide/sub dots; pulses ±.
- Section on/off; repeats; dir; mode.
- Waveform; LFO target; every toggle.
- Queue slot.
- Dot-matrix cell; TM bit; TM reroll.

There are no modal pages: every control is a direct button. Sliders change continuously while audio plays.

### 3.2 Everything visible at once
- The entire 16-step melody is always on screen as heights with note names. You *see* the contour (staircase, peaks).
- Section colors, the flags of all steps, the filter Hz in the header of the voice panel, and the sub mode in the header of the sub panel are visible together.
- Six large panels are open simultaneously. The user can move between "melody", "sound" and "generative" layers by scrolling, without navigating.

### 3.3 Immediate audible feedback
- Edits mark `seqDirtyRef` and the pulse list rebuilds on the **next 16th** (L984, L1765). There is no restart, no "apply" and no wait for the loop end, except slot loads, which queue on purpose.
- Filter, FX and env changes ramp in 50-100 ms.
- Tone's lookahead scheduling gives sample-accurate note timing that is independent of UI load.

### 3.4 Why it sounds musical out of the box
1. **Degree-based pitch:** you cannot hit a wrong note. RANDOM draws degrees, not semitones (L2362), and the scale-degree grid is the column itself.
2. **The random distribution is tuned for music, not uniformity.**
   - Mostly single 16ths (pulses 1 at 60%, ratchet 1 at 78%).
   - Few rests (10%) and some legato holds (18%); multi gates never.
   - Octave changes at 40% (±1 only).
   - Sparse accents (12%) and slides (8%).
   - About 22% sub-bass hits on step starts, which gives a bassline pulse.
   - 10% of steps have probability 0.4-0.9, adding subtle variation each loop.
   - The result breathes and loops, but varies slightly.
3. **The sub follows the key** (root/third/fifth via scale index), triggers only on step starts, has short decay with low sustain (0.24) and is lowpassed at 243 Hz. It is a clean, punchy bass that never clashes.
4. **FX defaults are "wet but clear".**
   - Reverb: a 6 s convolution tail at send ≈0.20 and return ≈1.21, darkened to 4.2 kHz with lows above 24 Hz kept.
   - Delay: 8th-note echoes at 12% send, feedback 0.32, darkened to 4.7 kHz.
   - Both are in parallel with a dry path at 0.84, so the dry signal stays punchy while space fills in behind it.
5. **Lead tone:**
   - band-limited Web Audio saw;
   - 24 dB LPF at 3.26 kHz with a mild resonance bump (about +7 dB across two stages at Q=3 dB);
   - plucky ADSR 5/180/0.4/120;
   - an 82% gate at 80 BPM, which is about 154 ms per 16th.
   This gives an acid-adjacent but soft pluck. 80 BPM with 16ths is a relaxed, "mello" tempo.
6. **Clean gain staging, plenty of headroom.**
   - Lead velocity 0.72 → mix 0.58 → filter → Distortion stage → level 0.9 → dry 0.84 → limiter -1 dB → master -2.5 dB.
   - Tone v14's Distortion curve is `(3+k)x·20°/(π+k|x|)` with k = drive×100 (Tone.js library behavior, not in this file). At drive 0 with wet 1 it acts as a **linear ×1/3 gain** (≈ -9.5 dB). At high drive it saturates near ±0.35.
   - So the distortion is effectively **loudness-compensated**: turning it up changes the color, not the volume.
   - The dry peak is therefore roughly 0.72·0.58·0.33·0.9·0.84 ≈ 0.10. The limiter almost never pumps.

### 3.5 Timing and groove details
- **No swing or humanize.** The groove comes entirely from structure:
  - pulses (variable step lengths, which create odd phrase lengths; loop length is shown in pulses and bars);
  - ratchets (even subdivisions of one 16th, 72% gates);
  - gate modes and per-step probability;
  - sections with repeats, reverse, ping-pong and random;
  - Euclid sub over the whole loop;
  - TM gate flips.
- Gate lengths: single 82% of the step, ratchet 72% of the sub-division, multi 40% per pulse, hold legato to 88% of the last pulse. Sub bass 90% of the step.
- The mono lead re-triggers its envelope each note. Portamento applies only to slide steps (set per note, L1594).

### 3.6 Exact Tone.js node graph (L1246-1369)
```
lead  Tone.Synth{osc: saw|square|triangle|sine|pulse(width), env ADSR, portamento per-step}
subOsc Tone.Synth{osc: sine.., same ADSR} -> Gain(subOscLevel)
sub   Tone.Synth{osc sine.., env A.003 D.16 S.24 R.12, port .02} -> Filter LP -24 Q.8 (40*55^c)
        -> Distortion(drive, wet 0|1, 2x) -> Gain(.42)
sampleSlot[i]: ToneBufferSource -> AmplitudeEnvelope -> Distortion(2x) -> Gain(level)
all four  ──────────────────────────────────────────────────────────> mix Gain(0.58)
mix -> Filter LP -24 (80*200^cut, Q=res*15) -> Distortion(drive, wet=distMix, 4x)
    -> Filter LP -24 Q.2 (320*38^tone) -> Gain(distLevel .2-1.4) =: P
P -> dry Gain(0.84) ───────────────────────────────────────────────┐
P -> dlySend Gain(wet) -> FeedbackDelay(time, fb, wet1) -> LP -24 Q.2 (450*26^t) -> Gain(lvl) ─┤
P -> revSend Gain(.03+w^1.2*.95) -> Reverb(conv, decay, preDelay, wet1) -> LP -24 Q.2 (700*18^t)
     -> HP -24 Q.35 24Hz -> Gain(level*1.05) ────────────────────────┤
                                                    fxMix Gain(1) -> Limiter(-1 dB) -> Destination(vol = gainToDb(master))
LFO Tone.LFO sine -> (synth.detune | mix.gain | osc.width); filter target via rAF
```

---

## 4. Top 10 things the hardware port MUST replicate (ranked)

1. **Scale-degree pitch storage with forced quantization, and one-gesture entry.**
   - Steps hold degrees 0..scaleLen×span-1, not notes. Changing root or scale re-keys the whole pattern. Out-of-key notes are impossible (L1874-1882).
   - On the FM-1, the 27 keys should map to **degrees** (white keys = consecutive scale degrees) or to "select step + set degree" in one press.
   - Why: this is the main reason "anything you do sounds good".
2. **One-button RANDOM with the exact probability table and the field mask (ALL/NONE).**
   - Apply it to all 32 steps, live, without stopping the transport (L2357-2374).
   - Why: the 2-action path to a groove depends on these specific, musical distributions (60/78/10/18/12/22/8/5/90%). A naive uniform randomizer sounds chaotic.
3. **Instant, sounding defaults on boot.**
   - The `i%7` C-minor run at 80 BPM, saw, LPF 3264 Hz / Q 3 dB ×2 stages, ADSR 5/180/0.4/120, reverb and delay at the defaults above. Play is a single dedicated button.
   - Do not auto-restore a stale patch over these defaults; the HTML does not (no mount-time `loadSnapshot`).
4. **The FX topology and gain staging, not just the effect types.**
   - Parallel dry 0.84 plus delay return plus reverb return, then a -1 dB limiter (L1352-1367).
   - The reverb send curve `0.03+w^1.2*0.95`; a dense, long (6 s) smooth tail; LPF 4.2 kHz plus HPF 24 Hz on the return; return ×1.21.
   - Delay 8n synced to BPM, fb 0.32, LPF 4.7 kHz post-delay.
   - The ×1/3 linear Distortion stage at zero drive, plus soft saturation that tops out around ±0.35, so dist on/off stays level-matched.
   - A cheap Schroeder/Freeverb with metallic ringing, or a missing dry/wet balance, will lose the "mello" space.
5. **The lead voice character.**
   - Band-limited (PolyBLEP-class) saw; mono with retrigger.
   - **Two cascaded biquad LPs** whose Q is in Web-Audio dB semantics (linear Q = 10^(Qdb/20) per stage).
   - Velocity 0.72 / 0.95; per-step-only portamento of 50 ms.
   - The faint sub-osc layer (effective about 0.048 amplitude because of the double 0.22).
6. **Timing semantics of pulses, ratchets and gates, and sample-accurate scheduling.**
   - 82% / 72% / 40% / 88% gate rules and pulses that lengthen steps (L1582-1669).
   - Notes must be scheduled ahead (Tone uses a 100 ms lookahead) so timing never jitters with UI or encoder work.
   - Edits apply on the next 16th (dirty-flag rebuild), not at loop end.
7. **The SUB BASS voice as its own filtered, key-following voice.**
   - Sine, A 3 ms / D 160 ms / S 0.24 / R 120 ms, LPF 243 Hz -24, gain 0.42, gate 90% of the step, triggered only on step starts. Pitch = root + scale-interval + octave -1.
   - The STEP, EUCLID (6 pulses over the loop), MATRIX and TM modes, and X-SUB R/3/5 steps.
   - It also passes through the main filter and the FX.
8. **Whole-pattern visibility and playhead feedback.**
   - The HTML always shows 16 bars with note names, flags and a glowing playhead (50 Hz refresh). The 240×240 screen should give a persistent 16-column contour view with the playhead, the selected step's note name, and flag dots on the main page.
   - Parameter pages should overlay briefly while turning an encoder (name + value + unit), then return to the pattern.
   - Why: without seeing the melody, editing becomes guesswork.
9. **Generative layers that stay in key and default to subtle settings.**
   - TM: 10% mutation, length 8, GATE route at 35% depth.
   - Dot-matrix filter "pluck": ×(1+2.6·amt), 20 ms up, 160 ms exponential down.
   - Per-step probability; loop-cycle key/scale/octave shifts each N ticks with SYNC.
   - Each must be one toggle away and must modulate *within the scale*, so evolution never breaks the music.
10. **Smoothness and live performance safety.**
    - All parameter changes ramp over 50-100 ms; there are no clicks on filter or FX moves.
    - Slot recall while playing queues to the pattern wrap; slots exclude FX and BPM.
    - Tap tempo. Space-style instant stop that releases held notes (L2108-2151).
    - These make it feel safe to twist knobs and switch patterns mid-performance.

### Quirks the port should copy (or knowingly fix)
- The Reverb "Room size" control is inert with Tone v14 (convolution branch, L1008-1011, L1303-1306). Decay is the real size control.
- Delay tone is outside the feedback loop (L1356-1359).
- The sub-osc level is applied twice (L1272, L1592).
- The current loop-cycle slot is applied even when Loop Cycle is OFF (L1781).
- A TM gate override bypasses step probability and rests (L1812, L1822).
- RANDOM never produces `multi` gates. Clock-div works by skipping 16th ticks.
- The filter LFO updates at display rate (rAF, about 60 Hz), not audio rate, and only while the tab is visible.
