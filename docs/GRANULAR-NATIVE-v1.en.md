# GRANULAR / LENS — hardware verification, 13 November 2026

[Русский](GRANULAR-NATIVE-v1.md) · **English**

Experimental 0.6.0-dev is installed separately as **APPS/MOSS-GRANULAR** on the original RG35XX running GarlicOS 1.4.9. This console has copies of the user's library/sequences and a new LENS STUDY demo. Stable MOSS 0.5.2 and original projects are preserved: the five original project-file hashes and stable binary still match. All 57 installation files were verified after transfer. User data is not published in the repository.

## Engine and controls

One source of up to 1 MiB and eight grains per track; four tracks, two effects per track and two on the master. Twelve controls, Freeze, note transposition, LENGTH/FADE, parameter locks, assignment and named import, SONG and resampling use the real engine. See [controls and limits](GRANULAR-INSTRUMENT.en.md).

The eight previous Windows suites and granular integration passed, as did UBSan. The same integration test passed on ARM: MOS7/MOS6, block-invariant output, gate/Freeze, source replacement, physical gestures, locks, SONG, capture, import/cancel/write-failure/retry, reference protection and persistence. The ARM test's large temporary SongBank was removed from its stack; the application already allocates that bank separately.

## Device stress tests

Stock 1.008 GHz, audio FIFO priority 10, 32000 Hz, 1024 stereo frames: **32 ms per block**. The full audio graph runs; diagnostic output alone is muted. UI pages and tracks change during playback. Sources occupy 32 MiB of synthetic PCM at 96 kHz. Four tracks use 512 canonical steps, 1/64, 240 BPM, dense events and locks. Granular settings include 128 attempted births/s, up to 500 ms, combined +48 semitones and parameter extremes; extra runs cover 1 ms and size/Freeze changes. Capture with background save is requested every ten seconds.

| Binary / scenario | Seconds | Maximum block, ms | p99 ≤ ms |
|---|---:|---:|---:|
| Initial: 4 GRANULAR, ORBIT+POND | 60 | 27.906 | 25.750 |
| Initial: 4 GRANULAR, ORBIT+CRUSH | 30 | 23.968 | 23.250 |
| Initial: 4 GRANULAR, ORBIT+REEL | 30 | 25.182 | 23.250 |
| Initial: 4 GRANULAR, POND+CRUSH | 30 | 26.841 | 25.750 |
| Initial: 4 GRANULAR, CRUSH+REEL | 30 | 25.793 | 23.500 |
| Initial: 4 GRANULAR, POND+REEL | 600 | 28.524 | 26.500 |
| Initial: 1 ms grains, POND+REEL | 60 | 14.268 | 13.250 |
| Initial: size/Freeze changes, POND+REEL | 60 | 27.764 | 26.500 |
| Initial: GRANULAR/MONO/ORGANISM/FM | 120 | 22.134 | 21.250 |
| Initial: SONG, 32 sequences / 64 slots | 180 | 27.007 | 25.750 |
| Final: 4 GRANULAR, POND+REEL | 120 | 30.436 | 26.500 |
| Final: SONG | 60 | 26.637 | 25.500 |

Every run had zero audio-budget overruns, late intervals, stream underruns and UI frames exceeding 33.333 ms; all processes exited with code 0. Total: **1380 seconds / 23 minutes**. p99 is a 250 µs histogram bucket's upper bound. SONG covers all five instruments and six effect pairs, transitioning every second; observed peak RSS was 60,788 KiB. The ten-minute run completed 60 PCM captures; that counter alone does not count successful file writes.

Initial binary: `d49870d2de07b2c20151f3f6084451cca1fce1620aae2c14b87fffcd372de47d`.

Final installed binary: `2a101c8ee2be066a51b3617733934e4a47a8fb83eff09c20c6999aa19ebe98b3`. The ten-minute result belongs to the initial binary; the final binary was retested in the last two rows. These are measured scenarios, not a guarantee for every future combination or a budget for more expensive effects. This build was not tested on GarlicOS 1.3.9.

## Normal launch

The separate launcher ran through Garlic's normal chain. Actual framebuffer captures confirm the library, LENS STUDY, assigned AIR source, playing lens and Freeze/SCAN. evdev buttons, stop and a two-second MENU exit were checked: exit 0, governor restored to interactive. The second launch was left on the demo VOICE with transport stopped.

Normal-session timing includes opening/saving a sequence: the first session recorded one late interval and two long UI frames, at most 65.596 ms. This is not a zero-counter normal-session result. On a second launch the counters were 0/0 before opening; after opening, still before PLAY with zero triggers, they were 1/1. Loading deliberately pauses SDL audio; the global counter does not exclude that pause. Do not conflate these metrics with continuous stress runs. Normal-session maximum audio computation was 4.329 ms, with no budget overrun. Human listening remains a separate quality check.

[Manifest and raw reports](verification/granular-native-v1/verification.json). Actual device captures: `.impeccable/review/native-granular/device-*.png`; the other PNGs there exercise the same renderer on the computer. Visual review: SHIP. Global DESIGN.md and its service style index remain unchanged.
