# MONO 0.9 design documentation record

Date: 2026-11-16. Scope: the finished native SDL MONO extension at 640×480. The independent visual disposition is **SHIP**, recorded in [review.md](review.md). Subsequent device verification is complete and recorded [separately](../../../docs/verification/mono-v09/README.md); it does not expand the scope of the original visual review.

## Evidence checked

- Read `PRODUCT.md`, `DESIGN.md`, `.impeccable/design.json`, `design/current/native-mono-v09.md`, and the document workflow reference.
- Opened all eight current captures at their native size: `0.png` CORE, `1.png` COLOR, `2.png` OSC, `3.png` ENV, `4.png` MOTION, `idle.png`, `release.png`, and `locks.png`. Compared the incumbent `.impeccable/review/native-v051/mono.png` and `monochar.png` directly.
- Checked `native/prototype/src/app_mono_ui.h`, `app_synth_ui.h`, and the parameter, pattern-bank and footer sections of `app_ui.h`; traced bank names and units through `synth_defs.h`, field geometry and button color mapping through `parameter_ui.h`, and navigation through `app.h`.
- Checked `mono_dsp.h`, `mono_view.h`, the engine mailbox publication sites, and `tests/preview.cpp` to distinguish measured voice samples and modulation values from illustrative graphs. This source trace establishes the UI's data origin; it is not a separate DSP accuracy or performance qualification.

## Incumbent system and token decision

The finished surface conforms to the approved flat graphite system. The source still uses the existing BG / PANEL / LINE / TEXT / DIM constants and four `slots` colors from `hardware lab/src/lab_ui.h`; these agree with the normative `DESIGN.md` palette. Physical Y / B / A / X retains `slot-4 / slot-2 / slot-1 / slot-3`, matching the old MONO captures. The shared 5×7 bitmap glyphs, text/value scales, square forms, neutral selection, 18 px edges and four 142×89 px parameter fields are retained.

The measured waveform, VCF NOW, two envelope bars and LFO marker reuse neutral text and line colors. Five visible names and the selected underline reuse existing navigation treatment. No new token, type family, shape, elevation rule or asset style is needed. The extension-specific layout and behavior are recorded in `design/current/native-mono-v09.md`; root `DESIGN.md` and `.impeccable/design.json` were intentionally not regenerated.

Preserved file SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `DESIGN.md` | `938D77B6C40A4498244C89D6F401D1BFFAA251DE9B734C7C22E42B1F324829A8` |
| `.impeccable/design.json` | `7F95AF240B1C04EA369D0BA994A9AAD2F027F4BE5E7D90CC90468E3FD3167614` |

## Actual behavior documented

The direction record now identifies the actual pre-FX waveform and cutoff/envelope/LFO telemetry, the GATE / RELEASE / IDLE distinction and idle fallback. It lists all twenty fields across CORE / COLOR / OSC / ENV / MOTION in physical Y / B / A / X order, with units. It distinguishes short L1/R1 bank selection on VOICE from L1+arrows for the nested bank inside VOICE LOCKS, where short L1/R1 retains the outer pattern banks. It also records the unchanged shared field geometry, main navigation and ORGANISM/FM diagrams.

## Existing drift observed, left intact

The root documents are versioned around native 0.5.2 with later local extensions. Their native 0.5.2 component paragraph still describes MONO as CORE / CHARACTER; the MONO 0.9 direction record now supplies the applicable local replacement without rewriting the earlier contract. Separately, the existing `DESIGN.md` Ambient LENS 0.6.4 note says four banks, whereas the already-established `granular_defs.h` has five: PLAY / SCAN / TEXTURE / ENVELOPE / CLOUD. This unrelated documentation coverage gap predates the MONO work and was not repaired. No palette or typography token discrepancy was found in the sampled native source.

## Capture provenance

The eight PNGs are QA output from the native renderer and the `mono-v09-*` fixtures in `native/prototype/tests/preview.cpp`, not generated artwork or physical-device photographs. The fixture initializes MONO STUDY / DEEP SIGNAL with the SOFT pulse preset, renders real engine audio for the active state, then selects each bank. Its idle variant stops the engine; its release variant releases the voice and renders another block; its lock variant selects MOTION on a LOCK ONLY step with an explicit LFO RATE lock. Thus the consistent active waveform across bank captures is deliberate. No new raster is shipped as part of the runtime UI.

| Capture | SHA-256 |
| --- | --- |
| `0.png` | `B5A32E1908A2A9169649D11495E7E1E62221EC91355EF90C71CD0A5EA7AF3BAB` |
| `1.png` | `74205FA8D9F4B4B96DE26F9485913F15A09616109188719DA16BB34D784340C7` |
| `2.png` | `9EFD4956D74651E04DFB145D83790B032A0FEA82ED3A0D0D1B20AC27907BCA48` |
| `3.png` | `9D07C666BC2482BB12E8DDD901FD943223C2EF5D5E3F8B25BEF764D6260DCE20` |
| `4.png` | `48BDC9A55A0C44D7F3F2822C6F12E1977CC4678CBD6412293EFC29B0EB76A636` |
| `idle.png` | `0F2AF2FFF09ED5032249B56A60DBCE04E9F34236D83C673602FF52B00C10B793` |
| `release.png` | `B61F3C30637FE0E79C9F9F15A51C29E4ECFD985E48600BB0E660D6A02EF5E8FF` |
| `locks.png` | `1C099439950AF0835104F079C7623B0B45CAE8C1E904CF94381DDCB31BE55877` |

## Completion boundary

Visual review and surface documentation are complete. Static captures and source inspection alone do not certify held-button interaction, option cancellation, live animation cadence, physical-screen readability, audio quality, frame pacing or RG35XX performance. The completed [device verification](../../../docs/verification/mono-v09/README.md) supplies separate, bounded evidence for its tested conditions. The original visual SHIP verdict is unchanged; no new visual review was performed for this documentation update.

## Completed device verification follow-up

Read the verification README, `manifest.json`, `deployment.json` and `ui-runtime.json` on 2026-11-16. Six uninterrupted profiles on the original RG35XX with GarlicOS 1.4.9 ran for 105 seconds and 3289 callbacks. Their maximum was 30.078 ms against a 32 ms budget; the reported profiles had zero over-budget callbacks, late intervals and stream underruns. These are short representative stress cases, not a long soak or an exhaustive guarantee.

The separate interactive fixture reached all five banks; physical Y+right changed LFO RATE from 0.18 to 0.19 Hz and Y+up then changed it to 0.29 Hz. STOP flattened the scope, and MENU saved and returned to Garlic. Its report retains one late interval. The interval counter includes the deliberate project-load audio pause, and the session opened a project; absent per-event timestamps, the verification record preserves that gap rather than claiming a clean interactive performance run.

The [deployment record](../../../docs/verification/mono-v09/deployment.json) confirms installation of SHA-256 `65d2f0a28e468eb02e9b4911c159b1b032a845b16b762799c9c47a453bf5907d`, 66 user files unchanged after normal launch, and successful return to Garlic. The verification README remains the authority for installation scope, backups, test limits and reproduction.

Additional `device-*.png` evidence comes from the RG35XX RGB565 framebuffer, as documented in the verification report. These device captures were added after the eight-image independent visual review and are recorded here for provenance, without extending that review or treating them as shipping raster assets:

| Capture | SHA-256 |
| --- | --- |
| `device-atlas.png` | `03F7569433CFFF4E0890409653E660A85F1BDF71E26A8FDB360D37D609489AD5` |
| `device-core.png` | `3F4FA6B03F18C8D00A214236910789CD5CC82777CB0CB6F3D7AF0E8FD984E3EB` |
| `device-motion.png` | `E2E2E8CC14CB68932FF84D61E6BDD429415DD7B0F35F6CD12CFD7F3A462455D0` |
| `device-edit.png` | `D291A04314F2262707852EAFE4410F21D4768A74B54CEC9A1BD372DF0267D6BF` |
| `device-idle.png` | `F0D7D13BFF0E331516DEF3B7BF4D6CC414DA8B3E55F177AE2C3356FFA3F680BC` |

Only `design/current/native-mono-v09.md` and this documentation record were changed during this documentation pass. No application source, root design file, sidecar or unrelated document was edited.
