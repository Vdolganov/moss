# Granular engine: first stage

[Русский](GRANULAR-LAB-v1.md) · **English**

Verified on 13 November 2026. Experimental branch: `experiment/granular-engine`, based on MOSS 0.5.2. This is an isolated DSP test rig, not a new application release. The stable binary on the console, sequences, SONGs and sample library were not modified.

**Outcome:** eight grains per track is the selected budget. The final four-track series, with two effects per track and two on the master, completed 13 minutes without audio deadline misses, late callbacks, slow UI frames or write errors. Maximum buffer time was 26.601 ms against a 32 ms budget. The console returned to Garlic, its previous CPU policy was restored and the stable MOSS checksum matched.

## Implemented behavior

One `GranularVoice` reads one prepared PCM16 asset from the existing `SampleBuffer`: up to 1 MiB, mono/stereo, 8–96 kHz. Scan speed and pitch are independent. Negative speed moves the grain birth position backwards; individual grains still read forwards. FREEZE stops scanning while grains continue to spawn. Size, pitch and position are latched at birth; parameter edits do not move existing grains' read cursors.

The engine provides a table-based Hann window, linear interpolation, position jitter, smoothly varying density irregularity, whole-cloud stereo balance, a four-pole low-pass and ASR. The envelope follows the filter. Fixed −18 dB attenuation provides headroom; there is no density-dependent automatic normalization. This initial gain setting still needs musical listening tests.

This is original C++ code, without copied Mangl source. Cloud organization was checked against [Engine_MGlut](https://github.com/justmat/mangl/blob/master/lib/Engine_MGlut.sc); window and parameter-latching semantics against [GrainBuf](https://doc.sccode.org/Classes/GrainBuf.html). Neither bit-identical nor timbrally identical Norns behavior is claimed.

| Planned control bank | Four parameters |
|---|---|
| PLAY | SPEED, PITCH, SIZE, DENSITY |
| SCAN | POSITION, START, END, JITTER |
| TEXTURE | SPREAD, IRREGULARITY, CUTOFF, FADE |

These banks currently describe the DSP contract. Screens, locks, sample assignment, FREEZE control and persistence still need integration with MOSS interaction patterns.

DSP ranges: SPEED −3…3; PITCH ±24 semitones; SIZE 1–500 ms; base DENSITY 0–512 Hz; POSITION/START/END 0…1; JITTER 0–500 ms; SPREAD/IRREGULARITY 0…1; CUTOFF from 20 Hz to 0.4375 of output rate; FADE 0–9000 ms with at least 2 ms of smoothing. Notes additionally transpose the cloud; the sum is clamped to ±48 semitones. Irregularity changes birth rate within 0.5…2 times the base; the live-grain cap remains fixed. These are tested kernel boundaries, not finalized screen control ranges.

Behavioral boundaries:

- START/END restrict the **grain birth scan**. Grains may read beyond that interval, wrapping within the complete asset. This is not PCM trimming; TAPE handles that.
- The pool limit is fixed at initialization. The conservative integration budget is **8 grains per track**, up to 32 across four tracks. Each grain reads both stereo channels. Larger pools up to 32 per voice remain available for research only; the limit never increases automatically.
- A full pool skips a new birth and lets existing grains finish their windows. `skipped` counts these events, not missed audio buffers. For example, 512 births/s with 500 ms grains greatly exceeds a pool of 16.
- Audio processing performs no allocation, file access or locking. Tables and PCM are prepared beforehand. The owner must keep the sample alive and immutable until the voice stops. UI-to-audio parameter handoff is not integrated yet.
- Linear interpolation does not eliminate aliasing during upward transposition. Arbitrary source loop boundaries may be audible. Timing tests cannot establish sound quality.

On ARM, one voice occupies 1320 bytes and shared tables occupy 5128 bytes, excluding PCM. The structure still reserves the research maximum of 32 slots even with an active limit of eight. Counters belong to the audio thread; a UI must read a snapshot rather than accessing them concurrently. Initialization and source replacement happen with the voice stopped; the audio-state owner applies parameter changes and noteOn/noteOff. `stop()` resets immediately; musical release uses `noteOff()`.

## Correctness tests

Windows and ARM run the same checks with assertions enabled in optimized builds:

- identical output for block sizes 1, 257 and 1024, including parameter edits, seek, release and retrigger;
- freeze, reverse scan, independent pitch and measured frequency doubling from 250 to 500 Hz;
- 8/16/32 hard caps, pool saturation, release and no rendering work after stopping;
- stereo separation, coherent mono output and independent random streams per voice;
- per-grain parameter latching and release duration across output sample rates;
- assets from one frame to 1 MiB, 8–96 kHz, extreme transposition, narrow scan ranges, NaN/Inf controls and zero density.

## RG35XX measurements

GarlicOS 1.4.9, stock 1.008 GHz, performance governor, ARMv7/softfp/uClibc. Output: 32 kHz stereo, 1024 frames, **32 ms** buffer deadline. GarlicOS 1.3.9 was not tested in this stage.

A short series compared 8/16/32 grains: one and four dry tracks, all six effect pairs on four tracks and master, short grains and rapid edits. At 32 grains, deadline misses appeared: ORBIT + POND — 13, POND + REEL — 56, rapid edits with POND + REEL — 89 of 94 measured buffers. That setting was rejected. The initial series had no deadline misses at 8 or 16.

The next test used **four stereo sources at 96 kHz with total transposition +48 semitones**: reading advances 48 source frames per output frame, increasing cache pressure. At 16 grains with POND + REEL, the maximum rose to 33.956 ms, with 139 of 157 buffers missing their deadline. Twelve grains produced no misses, but another effect pair reached 31.998 ms, leaving almost no integration headroom. Eight grains passed all six pairs; the highest maximum in that short series was 26.070 ms. This is why the budget is eight, despite the earlier successful ten-minute run at 16.

The sustained rig uses the real SDL audio backend, a 640×480 screen refreshed at approximately 30 Hz and repeated background eight-second WAV writes through the application's `CaptureStore`. Synthetic noise passes through the full DSP and output limiter, then is muted before device output. This tests processing deadlines, not audible quality.

All sustained cases use four tracks, two effects per track and two on the master. All voices and effects receive edits every 500 frames, equivalent to 1/64 steps at 240 BPM. The rig loads 32 MiB of PCM and touches additional memory the size of `SongBank`. It **does not execute** SONG transitions or the actual editor.

Initial 16-grain series (alternating 44.1/96 kHz sources, −24/0/+24 transposition; the retrigger scenario also changes notes):

| Scenario | Duration | Mean / p99 / maximum buffer | Deadline misses / late intervals | Saves |
|---|---:|---:|---:|---:|
| Dense cloud, POND + REEL | 600 s | 21.535 / ≤22.750 / 24.684 ms | 0 / 0 | 38 |
| Dense cloud, ORBIT + POND | 60 s | 21.072 / ≤22.500 / 23.594 ms | 0 / 0 | 4 |
| 1 ms grains, POND + REEL | 60 s | 10.759 / ≤11.750 / 12.516 ms | 0 / 0 | 4 |
| Changing size, freeze, seek, retrigger | 60 s | 22.014 / ≤23.250 / 25.221 ms | 0 / 0 | 4 |

p99 is the upper bound of a 0.25 ms histogram bucket. A late interval means callback start times more than 48 ms apart; this is a separate indicator, not a driver underrun counter. All eight sustained runs had zero save errors, audio-priority failures and UI frames over 33.333 ms. Static reservation was 68,889,420 bytes; observed RSS was approximately 56 MiB. Rig reservation is not the future instrument's actual memory consumption.

Final series for the selected **eight-grain** limit:

| Scenario | Duration | Mean / p99 / maximum buffer | Deadline misses / late intervals | Saves |
|---|---:|---:|---:|---:|
| 96 kHz, +48 semitones, POND + REEL | 600 s | 22.853 / ≤24.250 / 26.601 ms | 0 / 0 | 38 |
| 96 kHz, +48 semitones, ORBIT + POND | 60 s | 22.117 / ≤24.000 / 25.617 ms | 0 / 0 | 4 |
| 1 ms grains, POND + REEL | 60 s | 10.895 / ≤12.000 / 12.713 ms | 0 / 0 | 4 |
| Changing size, freeze, seek, retrigger | 60 s | 16.185 / ≤17.250 / 18.905 ms | 0 / 0 | 4 |

The final series contains 24,379 measured audio buffers and 50 successful saves. The recorded WAV structure was checked (32 kHz, stereo PCM16, 256,000 frames); raw JSON and SHA-256 values are stored beside this report. This supports proceeding to integration; it is not a promise about the full application before that application is tested.

## Files and reproduction

- [DSP](../native/prototype/src/granular.h), [correctness tests](../native/prototype/tests/granular_test.cpp), [load generator](../native/prototype/tests/granular_harness.h), [SDL rig](../native/prototype/tests/granular_device.cpp).
- [Raw results and checksums](verification/granular-v1/manifest.json).
- Build from repository root: `node native/prototype/scripts/granular.cjs`. Requires the previously configured Zig and Miyoo SDK; paths follow the main `toolchain.cjs`.
- Builds Windows/ARM checks, headless benchmarks, the ARM SDL rig and three WAV examples in `native/prototype/build/granular/audio/`. Examples use only the bundled demo sources, not user recordings.
- Headless CLI: `benchmark-arm TRACKS CAPACITY FX_MASK MODE SECONDS`. FX bits: ORBIT=1, POND=2, CRUSH=4, REEL=8; at most two. Modes: 0 — long grains; 1 — 1 ms grains; 2 — rapid changes; 3 — all sources at 96 kHz and total transposition +48.
- SDL CLI: `device-arm CAPACITY FX_MASK MODE SECONDS REPORT.json`. The [launcher](../native/prototype/scripts/granular-device.sh) accepts those four numeric arguments or runs the complete series without arguments. Reports and the test WAV are written only to `/mnt/mmc/MOSS-Granular-Lab`.
- The [headless series](../native/prototype/scripts/granular-headless.sh) deliberately includes the unsuitable 32-grain setting. Its nonzero exit status is expected when it detects overruns; each case has its own data.

Launch the SDL rig through Garlic's normal one-shot command mechanism with MOSS closed. Do not run it beside the full-screen menu or replace someone else's pending `command.sh`. It uses the existing `packaging/cpu-performance.sh`, which restores the CPU policy on exit. Do not overwrite a running test executable. Report checksums distinguish the initial rig, the expanded mode-3 build and the final build with a default budget of eight. The granular algorithm is unchanged at an explicitly specified capacity; a separate repeat verifies the output checksum.

## Next stage

Integrate the chosen cap with the instrument: TAPE → single-sample assignment → three VOICE banks → sequencer gate/velocity/locks → persistence. Then repeat sustained tests inside the application: four granular tracks, mixed MONO/ORGANISM/FM/SAMPLER projects, device input, SONG and resampling. Until then, isolated rig results do not guarantee glitch-free operation throughout the application.
