# Ambient LENS · 0.6.4-dev

[Русский](AMBIENT-LENS-v1.md) · **English** · November 14, 2026

Installed separately as **MOSS-GRANULAR** on the original RG35XX / GarlicOS 1.4.9. Stable MOSS and all existing project/library files were preserved and backed up. The independent **LENS DRIFT** sequence uses a new original 6.4-second synthetic source, a long held note, SIZE 3200 ms, JUMP 75% and SCATTER 40%.

SIZE reaches 8 seconds with evenly staggered births within the unchanged eight-grain pool. JUMP moves the scan head randomly while SPEED continues; FREEZE holds both. The circular waveform dims excluded samples; stable arcs and read squares come from audio snapshots. Four banks and physical controls are unchanged. See the [instrument guide](GRANULAR-INSTRUMENT.en.md) for controls and MS10 migration.

## Hardware load

**15 runs, 19 minutes, 35641 callbacks:** zero audio-budget overruns, late callback intervals, stream underruns or slow UI frames in these stress runs. Maximum **24.409 ms** within a 32 ms budget; worst p99 upper bound **22.75 ms**.

Four tracks, maximum 512-tick timelines at 1/64 and 240 BPM, 32 MiB PCM, 96 kHz stereo granular sources, two active effects on every track and two on the master. All ten legal FX pairs are covered; dense locks, repeated SD resampling and live UI are included. Every long-grain profile reached eight voices on all four tracks. Stock 1.008 GHz, 32000 Hz / 1024 frames. Diagnostic output is muted after full DSP; these are measured scenarios, not a listening assessment or proof of every possible input.

| Scenario | FX | Seconds | Mean ms | p99 ≤ ms | Max ms |
|---|---|---:|---:|---:|---:|
| 8 s grains / live LENS | ORBIT+DRIVE | 30 | 19.843 | 21.50 | 22.565 |
| 8 s grains / live LENS | POND+DRIVE | 30 | 20.759 | 22.75 | 23.701 |
| 8 s grains / live LENS | REEL+DRIVE | 30 | 18.396 | 20.25 | 20.338 |
| 8 s grains / live LENS | ORBIT+POND | 30 | 20.038 | 22.00 | 23.396 |
| 8 s grains / live LENS | POND+REEL | 30 | 18.679 | 20.75 | 21.437 |
| 8 s grains / live LENS | ORBIT+CRUSH | 30 | 17.126 | 19.00 | 19.822 |
| 8 s grains / live LENS | ORBIT+REEL | 30 | 17.671 | 19.50 | 20.271 |
| 8 s grains / live LENS | POND+CRUSH | 30 | 18.090 | 20.00 | 21.125 |
| 8 s grains / live LENS | CRUSH+REEL | 30 | 15.690 | 17.75 | 18.337 |
| 8 s grains / live LENS | CRUSH+DRIVE | 30 | 17.764 | 19.50 | 20.039 |
| 8 s grains / live LENS | POND+DRIVE | 600 | 21.225 | 22.50 | 24.409 |
| 1 ms / 8 s changes and FREEZE | POND+DRIVE | 60 | 18.984 | 20.50 | 21.503 |
| 1 ms grains | POND+DRIVE | 60 | 13.935 | 15.00 | 15.255 |
| GRANULAR + three synths | POND+DRIVE | 60 | 20.103 | 21.50 | 22.381 |
| SONG / all FX pairs | ALL | 60 | 17.696 | 22.00 | 24.142 |

## Functional checks and limits

Twelve host suites passed, along with ARM granular unit, ambient and integration tests. UBSan passed for granular unit/ambient tests; seven browser tests passed. Coverage includes sustained constant-source pads without burst gaps, independent jump randomness, FREEZE, block invariance, latched grain bounds/lanes, snapshot positions, physical JUMP edits and locks, long SIZE/MS10 roundtrip and old-format migration. Normal hardware UI covered demo playback, JUMP changes/restoration, FREEZE, partial-range shading, lock navigation and MENU exit 0. Existing files and the demo match their pre-session hashes; the CPU governor returned to `interactive`.

The separate normal session recorded 1 long callback interval and 2 slow UI frames (maximum 691.543 ms), with no audio processing overruns or stream underruns. It included project loading and saving; the log does not identify the exact transition responsible. These counters are preserved separately and are not included in the zero counts above.

The browser study remains silent/synthetic. Source PCM remains limited to 1 MiB; longer grains wrap their selected range. Source-boundary crossfading and anti-aliasing for large upward transpositions were not added. Head JUMP does not alter active grain read pointers.

[Reports and exact binary SHA256](verification/ambient-lens-v1/manifest.json).
