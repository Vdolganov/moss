# Native FM 0.12 · documentation audit

19 November 2026. Established-world extension; documentation follows the final source and supplied captures. Written boundary: `design/current/native-fm-v012.md` and this report. Global `DESIGN.md` and `.impeccable/design.json` are preserved.

## Evidence checked

- Product and system: `PRODUCT.md`, `DESIGN.md`, `.impeccable/design.json`, and the pre-implementation FM surface direction.
- Final implementation: `app_fm_ui.h`, `fm_view.h`, `fm_dsp.h`, `fm_defs.h`, `synth_defs.h`, `app_ui.h`; sampled shared field, synth-header, menu, navigation and preset application code.
- Form seed: `native/prototype/build/mono.bmp` and `app_mono_ui.h`. Visual reference: supplied Fors Opal Gem screen at `work/fm/research/gem-screen.png`; amplitude/modulation separation is the relevant influence.
- Captures inspected at native size: `bank0.png` through `bank4.png`, `presets.png`, `locks.png`, `held.png`, `release.png`, `idle.png`, `lockedratio.png`, `accent.png`, `device-fm.png`, `device-hit.png`.
- Final `review.md`: the initial live-ratio and BEND findings are resolved; disposition is ship **at scored-fix scope**. This report does not broaden that verdict.

## Recorded result

The final surface contract records all twenty controls in five banks, PAIRS/STACK, eight presets and their five-row picker, base versus effective ratio, velocity-scaled BEND, active/release/idle states, physical editing gestures and existing nested lock-bank navigation. It corrects the earlier direction's blanket bounded-selector claim: short VOICE L1/R1 wraps, while nested lock banks and preset selection clamp to their ends.

The phase portrait is documented as actual modulator/carrier samples, with amplitude-scaled geometry, rather than final audio output. MOD remains separate from AMP. Direct noise/click output, tone filtering and post-voice effects are explicitly outside its measurement. No new motion, color or typography system is inferred.

## System continuity

| Area | Evidence and result |
| --- | --- |
| Palette | Shared BG/PANEL/LINE/TEXT/DIM values match DESIGN.md; Y/B/A/X retains slot-4/slot-2/slot-1/slot-3. FM reuses ORGANISM's existing neutral portrait shade; see drift below. |
| Type | Existing bitmap 5×7 at scales 2 and 3; shared long-value fallback. No new font or ramp. |
| Geometry | Existing 640×480 frame, 18 px margin, four 142×89 fields at y=346 and footer at y=445. MONO provides the left visual/right telemetry form. |
| Named rules | Neutral Focus, Physical Button, Persistent Unit, Flat Surface and Fine and Coarse continue to apply. The local Live Operator Rule records only FM telemetry semantics. |
| Interaction | Existing A audition, L1/R1 bank, L2/R2 tab, L1+R1 options and L1+arrows nested-bank grammar; no extra modal for numeric editing. |

## Drift not canonized or repaired

The global files still carry native 0.5.2 descriptions and older CORE/CHARACTER language, while later surface contracts describe five-bank instruments. Their historical appendices and version wording were already present and are outside this FM documentation boundary. The sidecar narrative also retains older native/FX scope. These are reported, not silently rewritten.

The literal neutral `#96999F` used by FM is inherited from `app_organism_ui.h` but is absent from DESIGN.md's primitive colors. FM therefore uses the existing palette and rendering vocabulary, with this pre-existing un-tokenized shade; a claim that every rendered color is a declared token would be inaccurate. No token was added to legitimize it. Existing bitmap typography remains an incumbent native choice rather than a newly proposed type system.

No new craft-floor violation is made into a reusable rule by these documents. Device captures establish rendered states only. Physical readability, temporal feel and full-load audio performance remain outside the scored-fix visual verdict and this documentation pass.

## Preserved global files

SHA-256 baselines, verified unchanged after writing:

- `DESIGN.md`: `ED4E0430FDDA7C8237C76DFD686BF300560F02941598A0E10FAD4D93F7BEF945`
- `.impeccable/design.json`: `BA6C0BEF288E8EDB2BDCCF527AA80812748EF7C3094AA8486A0A7E0F356F4D81`
