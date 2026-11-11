# The iceberg: SkurkyMello's control layout (proposal, 2026-10-29)

The goal: very easy to make something good and cool, and you can dig deeper, but it is never impossible or
unintuitive to find your way. Built on the review's PERFORM HOME ([02_ux.md §5](../review-2026-10/02_ux.md)).

## Three depths

| Depth | Where | What lives there | How you get there |
|---|---|---|---|
| **Surface** | HOME | Play, random, undo, the four sound macros, tempo, steps on/off, transpose | Always: HOME, from anywhere |
| **One down** | A domain page per button | Everything about one thing: SOUND, FX, MOD, SEQ, BASS, KEY, GLOBAL | One button press |
| **Two down** | Layers within that page | The detail: envelope, LFO, delay config... | SELECT, or press the same button again |

Never deeper than that. A page button always opens its first layer, so the screen never depends on history. The
header always shows `PAGE / LAYER`, and the readout (done) shows what you turn.

## The surface: HOME

```
┌────────────────────────────────────────┐
│ C MIN · 120 BPM · 16 STEPS · SLOT 03 →04│  header: key, tempo, length, pattern (queued)
├────────────────────────────────────────┤
│   ▁▃▅▂▇▃▁▆  ▄▂▅▃▆▁▃▅   (skyline)       │  steps: height = pitch, dim = off,
│   ▲ playhead                            │  bright = playing now
├────────────────────────────────────────┤
│ RND  UNDO  MUTATE  FILL  ...  (black-key strip, drawn where the keys are)
│ CUTOFF   DECAY    SPACE    DRIVE       │  K1-K4: always-live macros
│  ◔ 62     ◑ 45     ◔ 30     ○ 0        │
└────────────────────────────────────────┘
```

| Control | On HOME |
|---|---|
| White keys W1-W16 | **Steps.** Tap = step on/off. **Hold + turn K1-K4 = that step's pitch / pulses / ratchet / gate** (the footer shows them while held). LEDs: lit = step on, dim = off, moving = playhead. |
| K1-K4 | **CUTOFF · DECAY · SPACE · DRIVE**, always live, while playing |
| SELECT | **TEMPO** |
| ALGORITHM | **RANDOM INTENSITY** (low / mid / high) |
| PRESETS | **PATTERN**: picks a slot, switches on the next bar (`→04` in the header) |
| Black keys | **Fixed places on every page** (R3, G1-G4): B0 ON (the page's part) · B1-B4 the page's own (HOME: transpose −/+, RESTART, CLEAR) · B5 the page's · B6 MUTATE · B7 the page's · B8 UNDO · B9 RNDMZR · B10 PERF (the PERFORM page: the white keys are FILL, ARP, REVERSE, ACCENT, REPEAT, the sweeps, THROW, WASH and the mutes on the beat, on every part) (2026-11-04) |
| PLAY | Start/stop; the LED blinks on the beat |
| SAVE | Saves the project, with "SAVED" on screen. Hold = pattern slots |
| EDIT | **KEYS**: the 27 keys become a keyboard. Tap = transpose the sequence; while stopped, play the lead |
| REC | In KEYS: step-record (each key writes the next step) |

From HOME: start, randomize, undo, shape the sound, set the tempo, mute steps and transpose, each **one action**.

## One down: the pages (one button each)

Since 2026-10-31 (DECISIONS.md "One thread", R1) the printed button *is* the section: pressed again (or ALGORITHM
turned) it goes to the section's next page, SELECT is the layer, and the header reads BUTTON > PAGE > LAYER.

| Button | Pages (press again, or ALGORITHM) | Layers (SELECT) |
|---|---|---|
| SEQ | **STEP** · SEGMENT · LOOP CYCLE · SLOT SEQ | STEP: TRIG · NOTE · FLAGS · SEQ; LOOP: STEP · HARM · LOOP |
| ENV | **SOUND** · SAMPLE | SOUND: FILTER · OSC · OSC 2 · CHARACTER · GRIT · MOD ENV · TRACKING · AMP ENV · FOLLOWER; SAMPLE: MAIN · PITCH · TRIM · ENVELOPE |
| LFO | **LFO** · DOT · EVOLVE | DOT: the five matrices (and a MOD lane's SHAPE); EVOLVE: MAIN · ROUTES · DEPTH · SOUND |
| FX | **DISTORTION** · DELAY · REVERB | DELAY: SEND · CONFIG · LOOP; REVERB: ROOM · RETURNS · OLIVERB |
| ARP | **BASS** · DRUMS · CHORD | BASS: PLAY · SEQ · STEP · VOICE · TONE · FILTER; DRUMS: VOICE · STEP · VARY · KIT · DIST; CHORD: PLAY · CHORD · VOICING · GRID · PATTERN · TONE · FILTER · MOTION · AMP |
| SEL | **KEY / SCALE** | — |
| GLO | **MIX** · GLOBAL | GLOBAL: CLOCK · MIDI · GROOVE |
| hold RNDMZR | **RANDOM** | MASK · LINKS · EDIT |
| hold SAVE | **SLOTS** | LOAD · RENAME |

CHORD opens first under ARP because the arpeggiator is its ARP UP / DOWN / PATTERN modes. No layer is hidden: GRID
and PATTERN are always there, and the title says GRID OFF / PTN OFF when they are not in use.

The keys (R3, R4, R6): a white key's tap toggles what sits on it and picks it; SEL held + a white key only picks;
EDIT turns the page's edit mode on (KEYS on HOME, pitch entry on STEP, amounts on RANDOM, the lock on EVOLVE, per
step on SAMPLE). The black keys: B0 ON, B1-B4 the page's own, B10 PERF (PERFORM) and B6 MUTATE on every
page, B9 UNDO, B10 RNDMZR.

## The look: the HTML's

From screenshots of the HTML (`node tools/tone-ref/shoot.mjs OUT`, full page, stopped and playing). The FM-1's palette
already uses the HTML's colours; what changes is how they are used.

- **Ground**: very dark green (HTML #04140B), not black; panels #0D2418 / #061810, lines #2A5C3A, muted text #4D9966.
- **One colour per domain, as the HTML colours its sections**:

| Domain | Colour | HTML |
|---|---|---|
| Steps, sequence, HOME | green #00DD88 | step bars, STEPS |
| OSC / FILTER | turquoise #00FFEE | CUTOFF 3264Hz |
| Envelope | A yellow · D coral · S turquoise · R purple | the ADSR letters |
| LFO, MOD, RANDOM | purple #CC55FF | LFO buttons, ⚡ RANDOM |
| Sub, follower, key/scale selection | yellow #FFCC00 | SUB OSC, OCT, selected |
| Distortion, DRIVE | coral #FF7755 | DISTORTION |
| Delay | turquoise #00FFEE | DELAY values |
| Reverb, SPACE | green #00DD88 | REVERB values |
| Stop, clear, destructive | red/pink #FF3377 | STOP, RESET ALL |

  Where the HTML gives no reason (DOT's four targets, the segments A-F), the FM-1 keeps its own colours.
- **Steps on HOME** as the HTML's lanes: tall bars, the note name in the bar, the playing step brighter.
- **Labels** small, uppercase, letter-spaced; values in the domain's colour.

## Rules everywhere

- **No on/off on a rotary.** Booleans go on black keys or EDIT, with their state visible.
- **Acceleration**: turn fast, move far. Every knob honours how far it turned.
- **Music units**: note names (C4), BPM, bars, %, ms, Hz.
- **What the screen says is true**: hints match the keys, and nothing is called "B5".
- **Destructive = hold 1 s**, and random has undo.

## Build order (each step its own commit, checked on the emulator)

1. HOME: macros on K1-K4; hold a step + turn edits it; the step LEDs with playhead.
2. HOME: RANDOM / UNDO on black keys, tempo / intensity / pattern on the three encoders.
3. The black-key strip drawn on screen; the hints fixed; SAVE with "SAVED".
4. Pages to the table above: page buttons open layer 1; MOD and BASS as layers; sample to GLOBAL.
5. Toggles off the rotaries; acceleration; music units; readout with full names and units.
6. KEYS mode (EDIT): transpose and play; REC step-record.

Each step keeps `make test` green and is checked as screens in the emulator before the next.
