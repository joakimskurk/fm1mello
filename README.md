# SkurkyMello for M-VAVE FM-1

Native FM-1 port of **SkurkyMello**, developed parity-first from the browser version and built on the Dinghy FM-1 platform.

## Continue development

For a new ChatGPT/work session, start with:

`START_HERE.md`

It identifies the active work package, required project files, source-reconstruction workflow, tests and stop condition. Do not reconstruct the current project direction from old chat history when the repository handoff files are available.

## Build status

This repository deliberately does **not** vendor a floating Dinghy checkout. CI reconstructs and verifies the exact upstream baseline before applying the Mello overlay:

- Dinghy repository: `zvenson/dinghy`
- pinned commit: `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- pinned tree: `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- upstream status at lock: `0.3`
- SkurkyMello package identity: `FM-1_970`

The CI order is intentionally strict:

1. fetch the pinned Dinghy commit and verify commit + tree;
2. build/run **unchanged Dinghy host**;
3. build **unchanged Dinghy target**;
4. copy Dinghy to a separate release tree;
5. apply only the SkurkyMello application overlay, flash write-range patch and release-identity patch;
6. run the overlaid app in Dinghy host mode;
7. build the real JieLi target `.fwsc`;
8. inspect image/RAM/pool budgets and package identity;
9. run the current Mello host/sanitizer tests;
10. upload/attest the package via the shared `zvenson/fm1-build` workflow.

No firmware is installed automatically.

## Local host tests

```sh
bash ci/test_host.sh
```

## Target build

The target build needs the JieLi toolchain and AC79 SDK. GitHub Actions obtains those through the shared FM-1 build workflow, so a push to `main` runs the full baseline + Mello target pipeline.

```sh
bash ci/build_target.sh
```

## Hardware status

Host and source validation are mature, but **real FM-1 hardware validation is still required** before a public release. See `docs/HARDWARE_VALIDATION_CHECKLIST.md`.

## Licensing

GPL-3.0-only. SkurkyMello's FM-1 shell derives from Dinghy/Felucca/SLOOP platform work; upstream copyright and licensing obligations are retained. See `LICENSING.md`.
