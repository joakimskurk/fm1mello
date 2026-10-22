# Mello hardware control grammar

This contract keeps the FM-1 interface predictable and hardware-first.

## Position belongs to the keys

Where a page represents steps, cells, bits, slots, chain positions or other discrete positions, the white keys select those positions directly. The app is not a performance keyboard; the keyboard surface mirrors the current step/cell domain unless a page explicitly requires manual note entry.

For domains longer than 16 positions, OCT DOWN/UP changes the visible bank. K1-K4 must not be consumed by bank or cursor navigation.

## SELECT belongs to function layers

SELECT changes the current page's functional layer/subpage. Examples include STEP TRIG/NOTE/FLAGS/SEQ, SEGMENT PLAY/STRUCT, LOOP STEP/HARM/LOOP, TURING MAIN/ROUTE/DEPTH and SUB TRIG/5SEQ/STEP/VOICE/TONE.

The selected step/cell must not move when SELECT changes layer.

## K1-K4 stay musical

K1-K4 edit sound, rhythm, harmony, timing or structural parameters. They must not be used to scroll the selected step, selected segment, loop slot, Turing bit, SUB 5-SEQ step, chord-chain step or sampler slot when that selection can be represented directly on the keys.

## Always show the mapping

Dense pages must show a compact control hint plus K1-K4 labels. The user should not need to remember a hidden encoder mode to understand how a page works.

All rendered UI text must fit its assigned text box. `tools/ui_visual_smoke.c` uses the firmware font metrics as a hard clipping gate in addition to the normal 240x240 bounds gate.

## Loop Cycle reference

Loop Cycle uses eight direct key positions and three layers:

- STEP: HOLD, ROOT+, TRANS, OCT
- HARM: KEY, SCALE, SUB, BASS TR
- LOOP: LEN, SYNC/FREE, ON/OFF, RESET NOW

SELECT cycles STEP/HARM/LOOP. The first eight white keys select loop positions 1-8.

When Loop Cycle is OFF, pressing loop keys 1-8 also auditions that slot immediately by making its harmonic context current. This is the manual comparison/performance mode for cycle slots.

When Loop Cycle is ON, keys 1-8 only select the slot being edited. They must not jump the running cycle playback position.

`BASS TR` is a per-cycle-step semitone transpose for the dedicated sub voice only, independent from the cycle's lead/main `TRANS`. Its range is -12..+12 semitones. It is packed into the previously unused high bits of the existing cycle sub byte, so the storage wire size and old snapshots stay compatible; old values decode as bass transpose 0.

RESET NOW resets playback position without changing the selected loop step.

## Regression gate

`tools/control_grammar_smoke.c` is a hard CI gate both with Chord Machine enabled and with `MELLO_FEATURE_CHORD_MACHINE=0`. It verifies Loop Cycle OFF audition, active-cycle edit selection without playback jumps, and independent dedicated-sub bass transpose.
