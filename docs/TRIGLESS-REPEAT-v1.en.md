# LOCK ONLY / REPEAT · 0.6.5-dev verification

[Русский](TRIGLESS-REPEAT-v1.md) · **English** · November 15, 2026

Installed separately as **MOSS GRANULAR** on the original RG35XX / GarlicOS 1.4.9. Existing three sequences, 25 prepared samples and stable MOSS were preserved and backed up. Added **LOCK STUDY**, using the existing original DRIFT SOURCE. [Controls and persistence](TRIGLESS-LOCKS-PLAN.en.md).

## Hardware load

**14 runs / 15 minutes / 28143 callbacks.** Zero audio-budget overruns, late callback intervals, stream underruns or slow UI frames in these stress runs. Maximum **26.490 ms** within the 32 ms budget; worst p99 upper bound **24.25 ms**.

Four tracks, maximum 512-tick timelines at 1/64 and 240 BPM, 32 MiB PCM / 32 assets with 96 kHz stereo sources. Eight simultaneous 8-second grains per track, or four sampler slots per track, plus mixed synths and SONG. Every track and master has two enabled effects; all ten legal pairs are covered. Dense lock-only events, all repeat divisions, live UI and repeated SD resampling. Stock 1.008 GHz, 32000 Hz / 1024 frames. Output is muted after full DSP; these measured scenarios are not a listening assessment or proof of every possible input.

| Profile | FX | Seconds | Mean ms | p99 ≤ ms | Max ms |
|---|---|---:|---:|---:|---:|
| 4 GRANULAR / held notes | ORBIT+DRIVE | 30 | 21.107 | 23.25 | 23.805 |
| 4 GRANULAR / held notes | POND+DRIVE | 30 | 21.406 | 23.25 | 24.140 |
| 4 GRANULAR / held notes | REEL+DRIVE | 30 | 19.375 | 21.50 | 22.107 |
| 4 GRANULAR / held notes | ORBIT+POND | 30 | 21.537 | 24.25 | 26.490 |
| 4 GRANULAR / held notes | POND+REEL | 30 | 19.224 | 21.25 | 22.156 |
| 4 GRANULAR / held notes | ORBIT+CRUSH | 30 | 17.888 | 20.00 | 20.525 |
| 4 GRANULAR / held notes | ORBIT+REEL | 30 | 19.080 | 21.25 | 22.316 |
| 4 GRANULAR / held notes | POND+CRUSH | 30 | 18.923 | 21.00 | 21.625 |
| 4 GRANULAR / held notes | CRUSH+REEL | 30 | 16.847 | 19.00 | 19.539 |
| 4 GRANULAR / held notes | CRUSH+DRIVE | 30 | 18.660 | 20.75 | 20.992 |
| 4 GRANULAR / held notes | POND+DRIVE | 300 | 21.974 | 23.25 | 24.472 |
| 4 SAMPLER / 16 slots | POND+DRIVE | 120 | 16.334 | 18.25 | 19.800 |
| GRANULAR + 3 synths | POND+DRIVE | 120 | 20.255 | 21.50 | 23.547 |
| SONG / transitions | ALL | 60 | 15.182 | 20.75 | 24.802 |

## Held-note and UI checks

All long-grain reports reached [8,8,8,8] voices. Their note-trigger counters exactly equal the expected number of 512-tick cycle starts; LOCK ONLY counts equal the remaining timeline events. Functional ARM/host tests additionally compare gate deadlines, read heads, grain ages, envelopes and sampler slots, ensuring that locks and repeat never call noteOn. The final ARM suite includes physical A/X gestures and a parameter change on a delay tail after the sample voice has finished.

13 host suites passed, plus targeted UBSan and the ARM suite. Tests include all repeat divisions at 30/70/240 BPM, stereo loop periods, block invariance, probability, per-parameter RESET, 15/16 polyrhythms, SONG, stop/restart, MS10 migration and MS11 roundtrip. Native renderer captures cover STEP, TIMING, RESET, menu and sampler states.

The separate normal hardware session recorded 0 budget overruns, 1 late intervals, 0 stream underruns and 2 slow UI frames (max 81.221 ms). It includes loading/saving, so its counters are kept separate from the stress runs. MENU exited with code 0; the governor returned to interactive. Original projects and the restored demo remained byte-identical.

[Exact binary hash and raw reports](verification/trigless-repeat-v1/manifest.json).
