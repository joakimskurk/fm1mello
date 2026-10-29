# 03 — Firmware engineering review (mello C port, FM-1 / AC791N)

Reviewer role: firmware engineering. Scope: `review/mello/*.c|h` + `review/overlay/dinghy.c`.
No source file in the review directory was modified. All proofs are host programs in
`scratchpad/rt03/` (full path:
`/private/tmp/claude-501/-Users-jhf-Library-Application-Support-Claude-scratch-workspaces-a37f9ad9-4170-457e-b227-8b39d2114a23-bd7bac0f-9add-4657-8ce6-3c03fe53d0f9-scratch-2026-10-28-4d5f5b/a1e19db5-dd27-43c1-802a-4a442d5baf0a/scratchpad/rt03/`),
built with `cc -std=c11 -O2 -I review/mello review/mello/<needed>.c rt03/<test>.c`. Where a test needed
instrumentation (counting `q15mul` overflows, counting `mello_sample_decode_block` calls), it uses a
*copy* of the file in `rt03/inst/`, `rt03/ex/` or `rt03/smp/`, with one function body changed.

Line numbers refer to `review/mello/*`. Note that `mello_audio.c:969-1049` is a single "line"
holding most of `mello_audio_render`, so several findings share line 981/1020/1045.

Known issues I was told about (reverb-return DC blocker dead band at about -150, `q15mul` flooring) are
**confirmed**: after one note on the default patch the output sits at L/R = -148/-147 indefinitely
(`t_silent`). They are not re-reported, but finding P1-1 below is a *consequence* of that DC that
had not been reported yet.

## Summary table

| # | Sev | Area | One-liner |
|---|-----|------|-----------|
| P0-1 | **P0** | Int overflow / DSP | `q15mul` int32 overflow in the ladder filter and the LFO→AMP multiply, with legal settings; the ladder then **latches at a permanent -21903 DC (about -3.5 dBFS) until reboot** |
| P1-1 | P1 | Silence detect → storage | After any note with reverb on (factory default), `silent` never becomes true again, so **autosave and explicit SAVE never run** and sample upload is refused |
| P1-2 | P1 | Storage | Older firmware autosave **erases a newer-format project record** (and the user's older project) after a load returns UNSUPPORTED |
| P1-3 | P1 | Storage | **Chord-machine settings are not persisted** at all (no chord section in the payload) |
| P1-4 | P1 | RT safety | Sample decode in the ISR is unbounded per block: short loops or high pitch cause **up to 84 block decodes (21k IMA steps) per 256-frame block** |
| P2-1 | P2 | Concurrency | Config seqlock and SPSC queue have no compiler barrier between payload stores and the volatile publish |
| P2-2 | P2 | Queue | Lane scheme can reject an event while 120/124 slots are free; drops may be NOTE_OFFs (stuck notes); `queue_overflow` is never read or cleared |
| P2-3 | P2 | Clicks | Sample retrigger/ratchet hard-resets the play position (measured step 6248 LSB, 19 % FS); trim-end cut, chord voice removal/kill, matrix-env reset |
| P2-4 | P2 | Zipper | No parameter smoothing; cutoff quantized to 101 table steps even for per-sample LFO; delay-time jumps (incl. MIDI-slave BPM jitter in sync mode) |
| P2-5 | P2 | Aliasing | Naive saw/square/pulse, 256-entry non-interpolated sine, sample pitch-up up to 16× with linear interpolation and no anti-aliasing |
| P2-6 | P2 | Timing | No scheduling lookahead: note onsets jitter 0–4.8 ms (measured), quantized to the block boundary |
| P2-7 | P2 | RT cost | Two variable-divisor `%` per sample (delay); 64-bit divisions in ISR paths, contrary to the engine's own "no 64-bit division on target" rule |
| P2-8 | P2 | Storage | Every save erases the full 40 KiB bank (10 sectors) for a payload of about 0.8 KB: wear and a UI freeze of up to several seconds |
| P2-9 | P2 | MIDI | Sample-slot CLEAR is plain CC113 on channel 16: any controller can erase a sample slot while stopped |
| P2-10 | P2 | Events | The all-notes-off flush also discards events pushed after STOP but before the next ISR (fast stop→start loses the first notes) |
| P2-11 | P2 | DSP | Residual dead bands in other one-pole filters; stale reverb tail replays after the network is parked; reverb reconfigure moves taps under live content |
| P2-12 | P2 | Code health | Minified one-liners, `mello_ui.c.orig`, write-only fields, Makefile `tests/` missing, no extreme-range or UBSan audio tests, storage load result ignored |

---

## P0-1 — `q15mul` int32 overflow: ladder filter latches at full-scale DC until reboot

**Where**
- `mello_audio.c:454`: `static int32_t q15mul(int32_t a,int32_t b){return (a*b)>>15;}`
- `mello_audio.c:981`: `ampmod=32767+(lfo*lfo_depth/1000)`, which reaches **65534** at depth 1000, then
  `lead=q15mul(lead,ampmod)` with `lead` up to about 65529 (osc + follower sub).
- `mello_audio.c:1020`: ladder `a->filterN += q15mul(x - a->filterN, al)`. `inp` is clamped to ±65535 but
  the states are not, so `diff` can reach about 131070 (and more once states wrap). With `al=30871`
  (cut ≥ 890), anything with |diff| > 69564 overflows.

**Failure scenario (all values are inside `sanitize_*` ranges)**
- Lead SQUARE or SAW (or PULSE) + follower sub SQUARE or SAW at level 1000, cutoff ≥ 850, any resonance, velocity 121.
- *Or* the default patch (follower sine at 220) + LFO→AMP depth ≥ 750 (≥ 950 without the follower).

When a wrapped product appears, all four ladder states jump to about ±139k. From then on
`diff*al` overflows on every sample and the wrapped arithmetic has a **stable fixed point**: the
filter outputs a constant -21903 forever, with notes off and transport stopped. Only `mello_audio_init`
(a reboot) clears `filter1..4`. The output is a large DC into the codec and amplifier. Because the
output never falls below ±8, `tail_guard_frames` keeps re-arming and `silent` stays false. That blocks
autosave (see P1-1), so the user's only remedy (power-cycle) also loses unsaved edits.

**Proof**
```
$ ./t_ub 0/1 (UBSan, -fsanitize=undefined)
mello_audio.c:454:53: runtime error: signed integer overflow: 61057 * 35979   <- called from :981 (ampmod)
mello_audio.c:454:53: runtime error: signed integer overflow: 75995 * 30871   <- called from :1020 (ladder)

$ ./t_latch      (-fwrapv to model the target's 32-bit wrap; one 0.5 s note, then 60 s of silence)
t=  0.50s lead_env=3193036 stage=4 filter1..4=-139127 -139127 -139127 -139127  out L min/max=-21903/-21903 silent=0
t=  1.16s lead_env=0 stage=0 filter1..4=-139127 ...  out L min/max=-21903/-21903 silent=0
t= 58.05s lead_env=0 stage=0 filter1..4=-139127 ...  out L min/max=-21903/-21903 silent=0
--- exact (int64 q15mul) reference, same input:
t= 58.05s lead_env=0 stage=0 filter1..4=0 0 0 0  out L min/max=-135/-135 silent=0

$ ./t_sweep  (instrumented copy; counts q15mul results that differ from exact int64)
lead=SQR follower=SQR cut=850 res=0 : overflow from follower_level>=1000 (... |out|>=20000 in tail after release=20480/20480)
lead=SAW follower=SAW cut=1000 res=0 : overflow from follower_level>=1000 (... 20480/20480)
... 24 failing (wave, follower, cut, res) combinations
$ ./t_sweep2
lead=SQR follower(default sine 220)=on cut=700: LFO->AMP overflow from depth>=750
lead=TRI ... depth>=750 ; lead=SIN ... depth>=850 ; lead=SAW follower off cut=1000 ... depth>=1000
```
Over a 2.3 s held note, the wrapped build (scenario 1) clips 99191/102400 samples; the exact build clips 0.

**Fix**
1. Ladder: compute `(int64_t)diff*al >> 15`, or clamp each state to ±65535 after the update (one compare
   per stage). Stage states should never leave the `inp` range anyway.
2. LFO→AMP: make it attenuating only, e.g. `ampmod = 32767 - depth*(32767 - lfo)/2000` (0..32767),
   or saturate `lead` to ±32767 before the multiply.
3. Give `q15mul` an explicit contract (|a|,|b| ≤ 32768) and, in debug builds, assert it, or
   provide a `q15mul_wide()` (64-bit or `smull`) for the 4 call sites that exceed it.
4. Add the `t_sweep` corner configs to the sanitize test set.
5. Defence in depth: on all-notes-off, or when `silent` has been asserted for N seconds, zero the
   filter, dist, delay-lp and reverb-HP states.

Related, fragile but currently safe: `soft_drive` (`mello_audio.c:946`) has `gain` capped at 65535, and
32767·65535 = 2 147 418 112 leaves only 65 535 of headroom below INT32_MAX.

---

## P1-1 — After any note with reverb on, `silent` never comes back: autosave, SAVE and sample upload are blocked

**Where**: `mello_audio.c:1045` (tail-guard re-arm `if(|mix|>8 && (delay_wet||reverb_wet)) tail_guard=…`) and
the `silent` computation. Consumers: `dinghy.c:254`
(`mello_storage_poll(..., !mt.running && mello_audio_is_silent(&audio))`) and `dinghy.c:232` (sample
transfer `safe`).

**Scenario**: factory default FX (`reverb_wet=240`). Play one note, stop the transport. The known reverb
DC (about -148) is above the ±8 threshold forever, so `tail_guard_frames` is re-armed every sample.
`mello_storage_poll` then returns DEFERRED forever, for autosave *and* explicit SAVE
(`request_save` also requires `safe_to_write`). Any edit made after the first note is lost on power-off.
Sample START/CLEAR/END are refused with an error ACK.

**Proof** (`t_silent`, unmodified sources):
```
default patch, no note    reverb_wet= 240 delay_wet= 120 : first silent at 0.12 s
default patch, one note   reverb_wet= 240 delay_wet= 120 : first silent at NEVER; after 120 s out L/R=-148/-147 tail_guard=264600
one note, reverb off      reverb_wet=   0 delay_wet= 120 : first silent at 1.42 s
```
**Fix**: fix the DC blocker (the known issue). Independently, (a) base the tail guard on AC energy
(e.g. |x − dc_estimate| or a peak-hold with a higher threshold such as 64), (b) cap the guard at
`reverb_decay`, re-armed only by *input* activity (send > threshold), not by output. Also consider
letting SAVE proceed when stopped by muting/fading the output for the duration of the flash operation.
That is the real requirement behind "safe".

---

## P1-2 — Downgrade: older firmware destroys a newer-format record

**Where**: `mello_storage.c:260-261` returns `LOAD_UNSUPPORTED` but leaves `active_bank=-1`.
`mello_storage_save_now` (`:277`) then picks `bank = active_bank==0 ? 1 : 0`, i.e. bank 0, and
the next save alternates. `dinghy.c:172` ignores the load result and sets `storage_ready=true`.

**Scenario**: the user installs firmware N+1 (format v4), saves, then rolls back to N (v3). Boot says
UNSUPPORTED and the engine runs on defaults. The first edit autosaves into bank 0 (destroying the user's
last v3 project), and the second autosave erases bank 1 (the v4 record). The comment "Never silently
roll back over data from a newer format" is enforced on read only.

**Proof** (`t_storage`, RAM-backed NOR model):
```
save#2 -> 2 bank 1 rev 2
boot older fw: load=4 (4=UNSUPPORTED) active_bank=-1
  autosave 0 -> result 2, wrote bank 0
  autosave 1 -> result 2, wrote bank 1
bank1 version field now = 3 (was 4) => NEWER-FORMAT RECORD DESTROYED
```
**Fix**: on UNSUPPORTED (or CORRUPT with a present header), set a `write_protected` flag that `poll` and
`save_now` honour until an explicit user "factory reset storage" action. At minimum, never erase a bank
whose header magic is valid with `version > FORMAT_VERSION`. Surface `last_load_result` in the UI.

---

## P1-3 — Chord machine configuration is never saved

**Where**: `mello_storage.c:166-186`. `encode_snapshot` and `encode_payload` emit META/WORKING/FX/SLOTS/SLOT_SEQ;
`MelloPatternSlot` has no chord member and there is no `SEC_CHORD`. `MelloEngine.chord` exists only in RAM.

**Proof** (`t_storage`):
```
load: 1 ; chord.on before=1 after=0, mode 2->0, level 900->320, release 3000->420  => CHORD SETTINGS LOST
```
**Fix**: add `SEC_CHORD` (section ver 1) for the working chord config, ideally also per slot. The
section framework already skips unknown ids, so older readers stay compatible. Keep the
sanitizer (`sanitize_chord_config`) on import.

---

## P1-4 — Unbounded sample decode in the audio ISR (CPU spike / underrun)

**Where**: `mello_audio.c:573-585` (`sample_decode_for`, loop wrap at `:584` forces
`decoded_ready=false` on **every** wrap); `mello_sample.c:59` decodes all 256 samples of a block per
call.

**Scenarios**
- `loop=on` with a short span (trim_end=10 ‰ of a short sample, or any loop shorter than 256 source samples):
  every wrap re-decodes the same block, i.e. a full 256-sample IMA decode every few output samples.
- Pitch up: `SAMPLE_SEMI_Q16` clamps at +60 semitones, so the step is 16 source samples per output
  sample. A new 256-sample block is needed every 16 output frames, and 15/16 of the decoded data is
  never read.

**Proof** (`t_sdec`, instrumented `decode_block` counter, 4 voices):
```
4 voices, unity pitch, no loop (baseline)          calls/block max    4  => IMA steps   1020 ; host render  57 us
4 voices, +24 coarse, note 96 (+60 st clamp)       calls/block max   64  => IMA steps  16320 ; host render 117 us
4 voices, loop, trim_end=10, short (1000-smp)      calls/block avg 51.2  => IMA steps  13260 ; host render 101 us
4 voices, loop trim 10 + coarse +24 note 96        calls/block avg 80.0  => IMA steps  21420 ; host render 132 us
```
That is 2.3× the baseline render cost on host, sustained, from sample decode alone. With the documented
80 MHz realtime margin of about 2.2× (`WP_E2_CLOSEOUT.md`), this is likely to overrun on target.

**Fix**: (1) do not invalidate the decode cache on wrap when the wrapped index is in the same block
(`decoded_block==block` is already checked, so drop the `decoded_ready=false` at `:584`);
(2) enforce a minimum loop span (≥ 1 block, or ≥ 64 samples); (3) bound the per-voice pitch step (e.g. ≤ 4×)
or decode incrementally (keep the IMA predictor/index state and decode only the samples actually
crossed). Then step through the block without re-decoding the prefix. (4) Add a per-callback decode
budget, plus an ISR cycle counter in the hardware checklist.

---

## P2-1 — Missing compiler barriers in the config seqlock and the SPSC queue (latent)

**Where**: `config_update_begin/end` (`mello_audio.c:817-832`) store a `volatile` generation, but the payload
(`pending_*`, `pending_sub_dist_curve`) is plain memory (`:895-901`). `mello_audio_push_engine_event` writes
`queue[slot]` (non-volatile) and then `q_tail` (volatile) (`:926`).

C only orders volatile accesses relative to each other. The compiler may sink the payload `memcpy`
below the "even" generation store, or the queue entry store below `q_tail`. On a single-core part
the seqlock logic is otherwise correct: the ISR cannot preempt itself, and the odd check is right.
If the stores are reordered, the ISR can apply a half-written config or pop a stale event (a previous
lap's entry, so wrong note or type). This is compiler-dependent and not reproducible on host.

**Fix**: `__asm__ volatile("" ::: "memory")` (or `atomic_signal_fence(memory_order_seq_cst)`) after
the odd store, before the even store, before the `q_tail`/`q_head` publish, and at the top of
`apply_pending`/`audio_pop_due` after reading the generation or tail.

## P2-2 — Event lanes: head-of-line rejection, drops may be NOTE_OFF, flag never surfaced

**Where**: `mello_audio.c:911-927`. A lane accepts only `due >= q_last_due[lane]`, and a lane frees only when
time passes its *last* due. **Proof** (`t_lanehol`):
```
push due=2000000/1500000/1000000/800000 us -> ok
push due= 125000 us -> REJECTED (4 lanes each hold 1 of 31 entries)
queue_overflow flag=1 (never cleared; no reader in firmware)
```
Mitigating evidence: 300 random legal patterns through the real engine+transport (`t_lanefuzz`,
bpm 60–300, div 1/2/4, all gates/ratchets, sub/chord/sample on) dropped **0** of 138 812 events,
because the engine emits per-pulse events tick-sorted. So this is a robustness gap, not an
observed bug. **Fix**: if all lanes reject, drop the *newest NOTE_ON* rather than a NOTE_OFF.
Also have `apply_event` tolerate a missing OFF (e.g. auto-release sample voices at trim end, which
already happens), and expose and clear `queue_overflow` in a debug view.

## P2-3 — Clicks: no de-click on retrigger and hard voice cuts

- **Sample retrigger** (`mello_audio.c:569`): `memset(v,0)` then `pos=start`, with `env` carried over
  from `prev_env`. A ratchet or retrigger jumps the waveform mid-cycle. **Proof** (`t_retrig`, filter
  open): `6678 6678 6678 430 -3555 -5386`, a **6248 LSB (19.1 % FS) step** at the retrigger.
  Fix: use a 2nd "fade-out" voice slot per sample voice (or a 1–2 ms crossfade), or ramp the env to 0
  over 32–64 samples before resetting `pos`.
- **Sample trim end without loop** (`:584`): `active=false; env=0` cuts mid-waveform. Fix: short release.
- **Chord**: `chord_note_off` removes one voice instantly (`active[i]=0`) and changes the `norm[]` gain
  (e.g. 13200→18500) on the others; `chord_kill` (STOP, chord off) zeroes the env. The 3.5 kHz LP
  softens it (`t_chordcut`, sine: 897 LSB step vs 560 steady-state max), but it is audible in
  PATTERN/ARP with overlapping gates. Fix: per-voice short release, smoothed norm.
- **Matrix filter env** (`:598`): every PULSE resets `matrix_env_q15=0`, an instantaneous cutoff drop of
  up to 240 ‰.

## P2-4 — Zipper and parameter discontinuities

- All config is applied per block with no smoothing (`apply_pending`): cutoff, levels, `dist_mix`,
  `master_level`, `reverb_send`.
- Cutoff resolution: `MAIN_FILTER_ALPHA_Q15[cut/10]` has 101 steps, and adjacent entries differ by about
  4–5 %. The LFO→filter path computes `cut` per sample (`:981`) but still indexes `cut/10`,
  so the sweep is a staircase, clearly zippery at high resonance. Fix: interpolate between table entries
  (`cut%10`), or keep a Q15 alpha and slew it.
- Delay time (`:973`, `:1026`): `delay_samples` jumps, so the read pointer jumps and clicks. In
  `delay_sync` it is recomputed **every block from the BPM**, and in MIDI-slave mode
  `measured_bpm` is re-estimated every 8–24 clocks (integer, jittery ±1), so the delay tail clicks
  or warbles while slaved. Fix: slew the read position (e.g. ≤ 1 sample/sample, tape-style) or crossfade,
  plus hysteresis on `measured_bpm`.
- Lead SUSTAIN level does not follow the sustain knob while held (no recompute in SUSTAIN), unlike Tone.js.

## P2-5 — Aliasing and oscillator quality

- `osc_sample` (`:442-451`): naive saw/square/pulse. At MIDI 96 (2093 Hz), harmonic 11 folds to
  21.1 kHz at -20.8 dB and harmonic 21 folds to 147 Hz at -26.4 dB (inharmonic, well inside the audible
  band). The browser (Tone.js) oscillators are band-limited, so this is a parity regression on
  high notes, chords (up to MIDI 108) and pitch-LFO.
  Fix: polyBLEP (2 multiplies + branch per discontinuity; cheap on fixed point).
- Sine: 256-entry table, 8-bit index, no interpolation (THD about -48 dB), used for lead, sub, follower,
  chord "soft" and the LFO. Fix: linear interpolation with the next 8 phase bits.
- Sample pitch-up: up to 16× with 2-point linear interpolation and no pre-filter, which aliases heavily.
  Bound the ratio or add a cheap half-band pre-filter for ratios > 2.

## P2-6 — No lookahead: onset jitter up to one block

The transport processes a pulse when `now >= due` (`mello_transport.c:300`), so every event reaches the ISR
already due and starts at the next block boundary. **Proof** (`t_jitter`, 120 BPM, 1 ms main loop,
256-frame callback):
```
NOTE_ON onsets: 160, mean onset error 2.05 ms, max 4.76 ms (uniform 0-5 ms)
```
Ratchets within a pulse are sample-accurate relative to each other; the pulse onset is not.
Fix: run the transport `LOOKAHEAD = 1 block + main-loop period` (about 8 ms) ahead and add the same constant
to the audio clock. The lanes already support future timestamps.

## P2-7 — Avoidable per-sample divisions; 64-bit division in the ISR

- `:1026`: `(delay_pos+cap-delay_samples)%cap` and `delay_pos=(delay_pos+1)%cap` are two variable-divisor
  modulos per sample. `cap` is 65536; use conditional wrap or a mask.
- `/1000` by constants compile to multiplies (fine), but around 25 of them per sample could be folded into
  per-block Q15 gains (`mix*580/1000`, `fx_in*840/1000`, level/1000 chains).
- `sub_attack` (`:518`, `uint64_t/…`, per SUB event) and `reverb_configure → reverb_feedback_q15`
  (`:645-648`, 24 × uint64 divisions per reverb knob change) run in the ISR.
  `mello_engine.c:191` states 64-bit division is "target-forbidden". The cost is small, so this is
  mostly an inconsistency, but it pulls `__udivdi3` into the ISR.
- `tone_distortion_point` (257 × uint64 divisions) runs inside the config-publish window, so it
  lengthens the window in which the ISR skips config updates.

## P2-8 — Storage write amplification and blocking

`erase_bank` (`mello_storage.c:275`) always erases `bank_size/sector` = **10 sectors** per save. The real
payload measured 798 B (`t_storage`: "erases=10 sectors for payload 798 bytes"). That is 10× the
needed wear, plus 10 × (45–400 ms typical/max NOR 4 K erase) ≈ 0.5–4 s of main-loop stall per save
(UI, MIDI and transport frozen). Fix: erase `ceil((24+size+8)/4096)` sectors. Optionally journal
several records per bank before erasing.

Also: the CRC is bitwise (8 iterations/byte); with up to 36 KB × 3 passes per save it is fine but a
16-entry nibble table is cheap.

Positive: the A/B banks, header CRC, payload CRC, commit footer written last, read-back verify, section framing
with length checks, and full `sanitize_*` on every imported field are good. I checked that every
`TABLE[x/10u]` index and `SAMPLE_FINE_Q15[fine+100]` is protected by the load-time sanitizers, so
malformed but CRC-valid records cannot index out of range.

## P2-9 — Destructive sample commands on generic MIDI CCs

`mello_sample.c:72`: CTRL = status `0xBF` (CC on channel 16) with CC110–114. `CLEAR` (CC113, value =
slot) erases the slot in flash whenever the device is stopped and silent. A controller knob or
MIDI-learn mapping on ch16/CC113 wipes samples. Fix: SysEx with a manufacturer/device ID and a
START→CLEAR handshake.

## P2-10 — All-notes-off flush is deferred and over-broad

`dinghy.c:247` sets `all_notes_off_pending`; the *next* ISR (`mello_audio.c:969`) sets
`q_head=q_tail` for all lanes. Events pushed between STOP and that ISR (≤ 5.8 ms), such as an
immediate restart (MIDI Stop+Start from a DAW, slot change), are discarded, so the first notes are missing.
Fix: publish a "flush up to sequence number N" marker instead of a boolean, or flush in the main
thread by letting the ISR drop events whose `due` precedes a published `flush_us`.

## P2-11 — Residual DSP issues (minor)

- Other one-pole filters with flooring `q15mul` also stick slightly negative after silence:
  `filter4=-1, dist_lp=-2, delay_lp=-1` (`t_silent`). At `filter_cut=0` (alpha 847) the ladder's bound is
  about 38 LSB per stage. Fix: round (`+16384`) or use a sign-symmetric multiply.
- The reverb network is parked when `reverb_wet==0 && tail_guard==0`, but its buffer is not cleared, so
  raising wet later replays a frozen tail fragment. Clear (or slowly zero) on park.
- `reverb_configure` changes line/allpass delays under live buffer content (pitch-glitch bursts while
  turning SIZE). Crossfade or mute the send briefly.

## P2-12 — Code health

- Readability: 15 lines in `mello_audio.c` are over 300 chars; the longest is 2596 chars.
  `mello_audio_render` (≈ 80 statements) and every voice function are minified one-liners, which makes
  review, diffs and line-level debugging or profiling impractical. `clang-format` it; split render into
  `render_lead/sub/chord/samples/fx`.
- Patch archaeology: `review/mello/mello_ui.c.orig` (1925 lines) ships in the source dir.
  `final4/ci/patch_*.py` + `final4/patches/*.b64.part-NN` mean the source of truth is "sources + 30
  patch scripts". Commit the result and delete `.orig`.
- Write-only or dead state in `MelloAudio`: `reverb_lp` ("retained compatibility state"), `reverb_damp`,
  `reverb_delay_a/b`, `last_lfo`, `rendered_frames`, `lead.gate`, `sub.gate`. `reverb_feedback_q15` is
  used only by test tools. `queue_overflow` is only read by tests.
- `dinghy.c:172` and `:185` discard the `mello_storage_load` and `mello_sample_store_load_all` results; the UI
  never learns about CORRUPT, UNSUPPORTED or IO_ERROR.
- Tests: `final4/Makefile` builds `tests/test_mello_{storage,audio,sample,...}.c`, but `final4/tests/` is
  **absent** from this snapshot, so `make test sanitize` cannot run here and storage/sample-store/transfer
  coverage could not be verified. The 10 audio/chord harnesses in `final4/tools` build and pass
  (rc=0), but none drives the parameter extremes. The existing `-fsanitize=undefined` target would have
  caught P0-1 with the `t_ub` configs.
- Feature flag: `-DMELLO_FEATURE_CHORD_MACHINE=0` builds warning-free, and the unity build (`dinghy.c`
  style `#include` of all .c files) compiles. `-Wconversion` yields only 2 sign-conversion warnings
  (`mello_ui.c:1509`). Good.

---

## Reproduction index (`scratchpad/rt03/`)

| Test | Proves |
|------|--------|
| `t_ub.c` (+ `-fsanitize=undefined`) | P0-1 UB sites :981, :1020 |
| `t_latch.c` (`-fwrapv` vs exact `ex/`) | P0-1 permanent -21903 DC latch |
| `t_sweep.c`, `t_sweep2.c` (instrumented `inst/`) | P0-1 reachable config space |
| `t_silent.c` | P1-1 never silent → autosave blocked; residual one-pole DC |
| `t_storage.c` | P1-2 downgrade destroys v4 record; P1-3 chord lost; P2-8 10 erases/save |
| `t_sdec.c` (instrumented `smp/`) | P1-4 decode calls/block and render time |
| `t_lanehol.c`, `t_lanefuzz.c` | P2-2 HOL rejection; 0 drops with real engine |
| `t_retrig.c`, `t_chordcut.c` | P2-3 retrigger and voice-cut steps |
| `t_jitter.c` | P2-6 onset jitter |
