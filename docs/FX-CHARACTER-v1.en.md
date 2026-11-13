# DRIVE and ORBIT · 0.6.2-dev

[Русский](FX-CHARACTER-v1.md) · **English**

The first implementation after the [Elektron investigation](ELEKTRON-FIRMWARE-ANALYSIS.en.md) adds a separate DRIVE and replaces ORBIT in experimental **MOSS-GRANULAR**. These are original algorithms, not copied firmware or a specific device emulation. POND, CRUSH and REEL remain available. Stable 0.5.2 remains a separate application.

## Controls

Open SPACE on a track or MIX → MASTER FX. All five effects appear in the list. A edits, X enables/bypasses, Y+↑/↓ reorders. Up to **two effects** run per track and two on the master.

Inside DRIVE/ORBIT, **L1/R1** selects CORE / CHARACTER. Fields follow **Y / B / A / X**: hold the field button with ←/→ for fine changes or ↑/↓ for coarse changes. B returns to the chain. L1+←/→ selects the adjacent effect without returning to the list. Windows uses Z/X/C/V for the fields and Q/E for L1/R1.

In PATTERN → FX LOCKS, **L1+←/→ selects the effect**, **L1+↑/↓ selects its bank**. The heading shows its name and 1/2 or 2/2. L1/R1 still selects STEP / VOICE LOCKS / FX LOCKS / CYCLE. All eight parameters accept locks; reordering retains their effect identity. CLEAR CURRENT LOCK BANK clears only the four visible fields. Locks without a new note remain a separate, unimplemented feature.

## DRIVE

| Bank | Y | B | A | X |
|---|---|---|---|---|
| CORE | DRIVE 0–100% | TONE 0–100% | OUTPUT 0–150% | MIX 0–100% |
| CHARACTER | CURVE: ROUND / BITE / EDGE | BIAS −100…100% | BODY 0–100% | DYNAMICS 0–100% |

ROUND enters saturation gently; BITE and EDGE sharpen the limiting. BIAS makes positive and negative half-cycles asymmetric. BODY changes bass **before** distortion; 50 is neutral. TONE changes treble **after** it; 50 is neutral. DYNAMICS reduces input drive on strong peaks using a shared stereo envelope. OUTPUT is explicit, not automatic loudness matching; match levels manually when comparing curves.

The shapes are odd polynomials bounded beyond ±1. First-order antiderivative antialiasing (ADAA) reduces aliasing; nearly identical consecutive samples use a midpoint fallback. Caching the previous integral avoids duplicate work. Curve changes crossfade over 20 ms; continuous controls smooth over 8 ms. DC removal uses a 5 Hz corner: a zero-drive 20 Hz test loses less than 0.5 dB.

ADAA is not alias-free: it introduces frequency-dependent attenuation and a fractional delay. In a bounded 1978.515625 Hz sine test at 32 kHz, unwanted energy relative to the fundamental fell by **5.86 / 7.09 / 8.12 dB** for ROUND / BITE / EDGE compared with the same direct waveshaping curves. This does not characterize every signal or replace listening. [Method and limitations](https://www.mdpi.com/2076-3417/10/1/20).

## ORBIT

| Bank | Y | B | A | X |
|---|---|---|---|---|
| CORE | TIME: eight divisions, 1/16…1 BAR | FEEDBACK 0–95% | WIDTH 0–100% | MIX 0–100% |
| CHARACTER | LOW CUT: OFF / 1–2000 Hz | HIGH CUT 500–14000 Hz | DRIVE 0–100% | MOTION 0–100% |

WIDTH blends independent repeats into cross-feedback. Filters color repeats, not the dry signal. LOW CUT defaults to OFF. DRIVE saturates recording and feedback, with normalized small-signal gain so 95% feedback does not turn into uncontrolled self-oscillation. MOTION adds slow time movement up to roughly 0.8 ms. Silence between triggers does not cut the tail.

TIME changes crossfade between two read heads over 40 ms; rapid locks accept the latest target after the current fade finishes. This is not tape acceleration. MOTION deliberately retains slight Doppler movement. Float32 storage and one conversion after the insert chain preserve internal headroom. Logical history invalidation avoids clearing megabytes in the audio callback.

## Performance and verification

**20 completed runs, 22.5 minutes, 42,208 audio callbacks:** zero budget overruns, late callback intervals, stream underruns or slow UI frames. Worst observed callback **26.412 ms** within 32 ms; worst p99 upper bound **23.25 ms**. Original RG35XX, GarlicOS 1.4.9, stock 1.008 GHz performance mode, 32 kHz / 1024 frames.

All profiles use four tracks, 512 ticks at 1/64, 240 BPM, a 32 MiB PCM bank, two enabled effects on every track and two on the master. Granular profiles use eight grains per track and 96 kHz stereo sources. Full wet mixes, dense locks, redraws and repeated SD resampling are included. Output is muted only after full DSP during profiling. These are bounded observed runs, not an exhaustive proof for every possible signal.

| Scenario | FX pair | Seconds | Mean ms | p99 ≤ ms | Max ms |
|---|---|---:|---:|---:|---:|
| 4 GRANULAR | ORBIT+DRIVE | 30 | 20.702 | 22.25 | 23.176 |
| 4 GRANULAR | POND+DRIVE | 30 | 21.101 | 23.00 | 23.561 |
| 4 GRANULAR | REEL+DRIVE | 30 | 19.054 | 20.25 | 21.004 |
| 4 GRANULAR | ORBIT+POND | 30 | 21.067 | 22.75 | 23.967 |
| 4 GRANULAR | POND+REEL | 30 | 19.410 | 20.75 | 21.753 |
| 4 GRANULAR | ORBIT+CRUSH | 30 | 18.220 | 19.75 | 20.007 |
| 4 GRANULAR | ORBIT+REEL | 30 | 18.646 | 20.50 | 21.158 |
| 4 GRANULAR | POND+CRUSH | 30 | 19.006 | 20.50 | 21.073 |
| 4 GRANULAR | CRUSH+REEL | 30 | 16.436 | 17.75 | 18.903 |
| 4 GRANULAR | CRUSH+DRIVE | 30 | 18.502 | 20.25 | 21.076 |
| Rapid CURVE locks | POND+DRIVE | 600 | 22.022 | 23.25 | 26.412 |
| Rapid CURVE locks | ORBIT+DRIVE | 60 | 21.549 | 22.75 | 23.598 |
| 1 ms grains | POND+DRIVE | 60 | 13.867 | 15.00 | 15.764 |
| Changing grains / FREEZE | POND+DRIVE | 60 | 21.170 | 22.50 | 23.767 |
| GRANULAR + synths | POND+DRIVE | 60 | 20.217 | 21.50 | 22.276 |
| 4 SAMPLER / 16 slots | ORBIT+DRIVE | 60 | 16.982 | 18.00 | 18.519 |
| 4 MONO | POND+DRIVE | 30 | 17.625 | 19.00 | 19.463 |
| 4 ORGANISM | POND+DRIVE | 30 | 21.290 | 22.50 | 22.866 |
| 4 FM | POND+DRIVE | 30 | 17.854 | 19.25 | 19.404 |
| SONG / all pairs | ALL | 60 | 18.604 | 22.50 | 25.038 |

[JSON reports / binary SHA-256](verification/fx-character-v1/manifest.json).

The separate normal session confirms startup, track/master banks, CURVE editing, FX LOCKS and normal exit. It had zero DSP budget overruns or stream underruns, but one long callback interval and two slow UI frames, maximum 476.740 ms. That session included sequence loading/saving; the current file-load path deliberately pauses audio. These counters are retained and excluded from the successful load-profile series. Seamless manual project opening is not claimed; SONG transitions were tested separately.


Four tracks and two inserts per rack are retained; voices are never disabled according to CPU load. ARM prefetch of the grains' next PCM data reduced mean block time from 27.79 to 21.36 ms in repeated hardware A/B runs, without changing audio calculations. Float ORBIT storage adds approximately 7.3 MiB across five racks. Main reserved application objects occupy 67,036,048 bytes including the 32 MiB PCM bank, excluding optional SONG objects and OS overhead. A maximum SONG adds approximately 17.1 MiB of sequence data.

Tests cover four sample rates, finite/bounded output, DC, feedback decay, stale-history reset, rapid TIME/CURVE locks, audio invariance with 1/127/1024-frame blocks, controls and migration. Test sources and the six-WAV example generator are in `native/prototype/tests/fx_character_*`; optional NumPy spectral analysis is in `scripts/analyze-fx-character.py`. User listening remains a separate timbre assessment.

## Bass investigation

Reduced bass was reported through both headphones and the speaker, including with all effects bypassed. The complete **sample → track → master → monitor** test on the host and actual ARM deviates by less than 0.001 dB from 20–1000 Hz relative to 1 kHz. A substantial global software high-pass is therefore not supported by this test. It measures digital samples, not analog output. Mixer controls were read without modification; no bass/EQ control was exposed. SDL supplies stereo PCM16 / 32 kHz; ALSA uses PCM32 / 32 kHz. Changing sample word length alone does not imply a bass filter.

`BASS-CHECK.wav` contains 40 / 80 / 160 / 1000 Hz, two seconds each, equal −20 dBFS amplitude and fades. Compare the same file on PC and in the console's TAPE **with the same headphones**, matching the last tone's level. Different frequencies are not expected to sound equally loud. The complaint's cause remains unresolved; no unmeasured global bass boost or hardware gain modification is introduced.

## Data

New saves use **MOS9**. MOS5–MOS8 remain readable; former ORBIT MIX/SPREAD values and locks migrate to MIX/WIDTH without losing values. New controls get defaults. The new algorithm still changes old ORBIT's timbre; identical sound is not promised. Loading alone does not overwrite a project. Stable 0.5.2 and earlier experimental builds cannot read MOS9.

The **DRIVE ORBIT** demo needs no samples: MONO passes through distortion before delay. Open DRIVE's CHARACTER bank, then use L1+←/→ to select ORBIT. Reduce DRIVE/MIX for gentler saturation and HIGH CUT for darker repeats. User sequences are separate from the load-test directory.
