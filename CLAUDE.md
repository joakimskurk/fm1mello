# Working on SkurkyMello

- Bare-metal FM-1 firmware on Dinghy 0.3. Read README.md and ROADMAP.md first; the current phase is the first
  unchecked one in ROADMAP.md.
- `reference/skurkymello.html` is the reference for sound and feel. When the C code and the HTML disagree, the HTML wins
  unless docs/mello/DECISIONS.md says otherwise.
- After every change: `make test`, then `make format`. Keep a change small enough to bisect: one fix per commit.
- Only edit Dinghy's own files (`platform.c`, `gfx.c`, `hal/`, `loader/`, `tools/`) when the task needs it, and say so
  in the commit message; they stay close to upstream.
- Audio callback (`mello_audio_render`): no flash, no unbounded loops, no 64-bit division; fixed point stays in range
  (a `q15mul` of two full-scale values overflows).
- Never install a package on the FM-1, push, or tag a release without asking.
