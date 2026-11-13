# GRANULAR CLOUD — 0.6.1-dev verification

[Русский](GRANULAR-CLOUD-v2.md) · **English** · 13 November 2026

The separate MOSS-GRANULAR experiment is installed on the original RG35XX running GarlicOS 1.4.9. Stable MOSS 0.5.2 was not replaced. Projects, the previous executable and launcher were backed up; SHA256 checks confirmed the installer left existing project files unchanged. A new **LENS CLOUD** sequence was added.

## Changes

- SCATTER 100% distributes grain births across the selected region independently of anchor position, scan speed and FREEZE. Eight zones are balanced against active grains, with a random position inside each selected zone. SCATTER 0% follows the anchor.
- Read bounds, pan and fades are latched per grain. Moving START/END does not cut existing grains.
- Four banks: PLAY, SCAN, TEXTURE, ENVELOPE. The last bank provides GRAIN IN/OUT as percentages of SIZE and NOTE IN/OUT in milliseconds. L1/R1 select banks.
- Cloud gain accounts for expected overlap and window energy, with a separate LEVEL control. Monitor gain increased from 135% to 180% (+33% relative); internal resampling remains before this gain stage.
- MOS8 stores 16 parameters and locks. MOS7 FADE migrates to NOTE IN/OUT; nonzero JITTER migrates to SCATTER 100%. Old JITTER locks are cleared because milliseconds cannot be converted unambiguously to a region fraction. Original files remain backed up.

## Hardware verification

Executable SHA256: `a85fa355b408920b17e6bae61f61539244cf8fd4390dab608457ff72f19f7e1b`.

32 kHz, 1024 frames per callback: **32 ms** budget. Four tracks, up to eight grains per track, the two permitted effects on each track plus two master effects. POND+REEL, 32 MiB PCM, maximum patterns at 1/64, 240 BPM, active UI and periodic resampling.

| Profile | Duration | Mean, ms | p99 upper bound, ms | Maximum, ms |
|---|---:|---:|---:|---:|
| Four GRANULAR tracks, full load | 600 s | 26.401 | 27.500 | 30.034 |
| Short grains | 60 s | 12.317 | 13.250 | 13.887 |
| Changing size, fades and FREEZE | 60 s | 26.379 | 27.500 | 30.211 |
| GRANULAR / MONO / ORGANISM / FM | 60 s | 20.379 | 21.500 | 22.093 |
| SONG | 60 s | 20.856 | 27.250 | 27.855 |

Across **14 minutes**, every profile recorded zero budget overruns, late callbacks, stream underruns and slow UI frames. The worst measured margin is **1.789 ms**: limited room for new effects, not a guarantee for future changes. [Raw reports and manifest](verification/granular-cloud-v2/manifest.json).

Host/ARM unit and integration tests passed; host UBSan found no errors. Coverage includes zone distribution, full-scatter anchor independence, bounds, envelopes, finite output, block invariance, 16 locks, four banks, MOS7 migration and MOS8 roundtrip. Browser tests: 6/6. LENS CLOUD → VOICE → ENVELOPE was also opened on the console during playback, followed by a normal application exit.

Synthetic WAV examples were checked numerically for finite output and absence of PCM rail values. This is not a listening assessment of the console output; character, perceived loudness and transitions still need headphone evaluation.

The separate normal UI session recorded one late callback interval and one 36.083 ms UI frame, with maximum audio processing of 4.876 ms and no audio budget overrun. This session included project transitions and audio stop/start; the log cannot attribute the event to a specific transition. These readings are not included in the five stress profiles' zero counters.

[Controls](GRANULAR-INSTRUMENT.en.md) · [Trigless locks: proposal only](TRIGLESS-LOCKS-PLAN.en.md)
