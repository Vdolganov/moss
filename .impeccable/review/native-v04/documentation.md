# Native 0.4.0 · documentation audit

Date: 2026-11-10. Scope: the SAMPLER / MONO / ORGANISM / FM extension only. This does not mark the whole release or project/track management work complete.

## Outcome

Added `design/current/native-synths-0.4.md` as a narrow implementation note. No durable visual-system change was found; `DESIGN.md`, `.impeccable/design.json`, browser files, source files, and README were left untouched. No new token export is needed.

## Evidence checked

- `PRODUCT.md`, `DESIGN.md`, `.impeccable/design.json`, and `native/prototype/README.md` establish the incumbent contract and browser/native boundary.
- `hardware lab/src/lab_ui.h`, `native/prototype/src/app.h`, `app_synth_ui.h`, `synth_defs.h`, and `model.h` supply the actual colors, geometry, bitmap type, labels, parameter ranges, picker behavior, note range, independent banks, and preset semantics.
- Native Windows captures `mono-window.png`, `organism-window.png`, and `fm-window.png` were visually inspected for the unchanged frame, neutral diagrams, fixed slot labels, legible values, units, and MODEL wording. The captures are evidence of the Windows surface, not device performance.
- `native/prototype/tests/synth_test.cpp` was read for SELECT press/release cancellation of both picker modes, unchanged project data, retained VOICE page, B cancellation, independent banks, and preserved sampler maps. No tests were rerun for these documentation-only additions.
- `review.md` reports the picker fix as resolved and `disposition: ship` for that scored change, with actual before/after Tab captures and a passing recorded synth verification run. This audit does not broaden that verdict to later work.

## Token comparison

| Source role | DESIGN.md / sidecar | Native source | Result |
|---|---|---|---|
| Background | `#191A1C` | `BG = 0x191a1c` | Match |
| Panel | `#222326` | `PANEL = 0x222326` | Match |
| Lines | `#414348` | `LINE = 0x414348` | Match |
| Text | `#E8E8E6` | `TEXT = 0xe8e8e6` | Match |
| Secondary text | `#A9ABB0` | `DIM = 0xa9abb0` | Match |
| Slots 1–4 | `#A7BA91`, `#91AEBB`, `#ACA0BA`, `#C69B83` | Same four `slots[]` values | Match |

Palette comparison is exact; no token repair was necessary. The synth model renderer uses existing neutral colors. Its labels use the shared four-slot renderer, so the instrument choice does not introduce another accent system.

## Existing scope drift, left unchanged

1. `PRODUCT.md` still declares platform `web` and contains historical prototype statements about no synthesis/audio. These remain true for the browser reference; they do not describe the native engine. The new surface note states the boundary without rewriting product history.
2. Root DESIGN.md and its sidecar describe browser Consolas/Courier typography and CSS geometry. Native rendering has inherited 5×7 bitmap glyphs at integer scales, a 49 px header region, and its own OPTIONS geometry. `lab_ui.h` already marks final native typography as a separate task. These observed prototype details were not promoted into global tokens or a new visual rule.
3. Root browser guidance uses a MONO model axis up to 20 kHz and discusses a moving ORGANISM model cursor. Native 0.4.0 uses the implemented 20 Hz–12 kHz cutoff range and static synth diagrams. The surface note records this implementation-specific adaptation without changing browser guidance.

## System summary

Palette: inherited graphite and the exact four fixed positional slot colors.

Type: inherited native bitmap labels at scale 2 and synth titles/values at scale 3; no new typography token.

Layout: existing 640×480 frame, four lower parameter fields, neutral OPTIONS picker.

Behavior: shoulder navigation and held-button edits remain; B/SELECT cancel pickers, and audition notes stay separate from step notes.

Model rule: static labeled technical diagrams describe base settings, never measured audio or live step-lock values.

Not canonized or repaired: pre-existing browser/native type, geometry, model-timing, and audio-scope differences; they are outside this narrow extension and are explicitly separated above.

## Workflow documentation follow-up · 2026-11-11

Updated `native/prototype/README.md` and added `design/current/native-workflows-0.4.md` after the project/track management and streaming TAPE implementations landed. The original synth note remains scoped to its surface. No source, browser file, global design token, or sidecar was changed by this documentation pass.

Checked `src/app_management.h`, `app.h`, `stream_reader.h`, `stream_preview.h`, `storage.h`, `main_garlic.cpp`, `tests/workflow_test.cpp`, `tests/synth_test.cpp`, `scripts/build.cjs`, and the resulting `build/tests.json`. The current build record reports music, interaction, features, synth, and workflow passed; its device status remains `pending`. Documentation changes did not warrant rerunning the suites.

Reconciled the stale four-slot/three-track/MOS3 language with up to 32 named projects, 0–8 named tracks, empty creation, instrument choice on add, rename/copy/confirmed deletion, track Undo, and one recoverable deleted project. MOS4 replaces older native projects only after the separately authorized backup/reset process; WAV and the sample library remain intact.

Reconciled the old oversized-preview refusal with the sliding 1 MiB PCM16 selection limit and unrestricted-by-that-budget full-source streaming. The new note records the exact 65,536-stereo-frame / 256 KiB ring, UI-side file reads, source-rate/channel duration calculation, selected/full audition controls, true preview position, stop-on-edit/leave, and remaining RIFF/long-file constraints.

README now describes eight trigger counters and stream underruns in periodic device reports, all five test suites, numerical synth checks, and the desktop 60-second eight-synth/FX/capture test. It explicitly leaves real RG35XX audio, SD streaming, load, and overall release verification open. Browser gaps are limited to the pertinent unimplemented key/scale metadata, JSON transfer, scenes, and pattern banks; no browser redesign or token export was performed.
