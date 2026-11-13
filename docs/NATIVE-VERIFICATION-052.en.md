# MOSS 0.5.2 — verification, November 13, 2026

[Русский](NATIVE-VERIFICATION-052.md) · **English**

The build was installed on the original RG35XX with GarlicOS 1.4.9. ARM binary SHA256: `e22b6a3da14a8ae88a4d4db7015ad5760374d96ce78ae6c3bd0bdaaac473c942`.

## Desktop

Eight test suites passed: music, interaction, features, synth, workflow, v05, v051 and v052. ARM and Windows share the model, engine and controls. The v052 tests cover:

- Fine/coarse adjustments, notes/octaves, locks, FX and trimming.
- Page jumps, partial last pages, both modifier-release orders and menu priority.
- Empty SONG creation, adding slots, repeats/bars, reordering, independent copies and return-to-row focus.
- Referenced-sequence protection, checksums, retry after save failure and missing files.
- Sample-accurate bar boundaries, continuous 15/16-cycle phase across repeats, reset on slot change, shared BPM, ending and looping.
- Identical audio across different block sizes, including transitions.

## Physical device

Stock 1.008 GHz, audio FIFO priority 10, 32000 Hz, 1024 stereo frames: 32 ms per block. Both runs use four tracks, maximum 512-tick cycles, dense events/locks, 32 MiB of prepared samples, background reports and capture saving, and UI updates.

| Run | Duration | Mean block | Maximum block | Overruns / late intervals / slow frames |
|---|---:|---:|---:|---:|
| SONG: 32 unique sequences, 64 slots, a transition every second, all four instruments and all six legal FX pairs on tracks and master | 180 s | 16.454 ms | 22.596 ms | 0 / 0 / 0 |
| Four ORGANISM tracks, POND + REEL on every track and master, screen changes | 60 s | 19.836 ms | 21.457 ms | 0 / 0 / 0 |

SONG cycles through effect pairs, with two effects active at a time as in normal operation. `fx_seen_mask=15`, `instruments_seen_mask=15`. Capture was started and saved in both runs; `stream_underruns=0`, `profile_passed=true`. These are diagnostic tests with muted speaker output: the counters demonstrate processing deadlines, not subjective sound quality.

Peak virtual address space in the SONG process was 95,952 KiB; RSS was 58,992 KiB. The bank of 32 sequence snapshots occupies 12,636,716 bytes (approximately 12.1 MiB). The raw report's `reserved_objects_bytes` includes static objects but excludes this separately allocated bank; process memory is reported separately to avoid understating usage.

Reports: `music-1794594140-428.json` and `music-1794594321-542.json`, included in verification.json. These are new 0.5.2 tests. The longer ten-minute 0.5.1 results remain historical and are not attributed to this new build. GarlicOS 1.3.9 was not retested in this session.

## Installation and normal launch

A fresh complete copy of the device's MOSS directory was saved to the computer before installation. Checksums of 50 user files under projects/samples/library/imports/captures were verified. Only the application, launcher and guide were updated; user WAVs and sequences were not replaced.

Normal launch through `APPS/MOSS.sh` was exercised through Garlic's standard launch chain. The framebuffer confirmed version 0.5.2, SEQUENCES / SONGS and the existing MORPH sequence. R1, SONG creation/naming, adding MORPH, playback and deletion of the test SONG through the context menu were checked through the real evdev input path. The original MORPH file retained its checksum. The application was left open on SEQUENCES.

Actual device screenshots: `.impeccable/review/native-v052/device-*.png`. Other captures in that folder use synthetic scenarios with the same 640×480 renderer. An independent visual review concluded `ship` for the reviewed issues: documentation pointers were synchronized; an initial report of missing MIX tabs was withdrawn after reinspection confirmed they were present.

## Practical limits

SONG structural editing requires playback to stop. All referenced sequences are loaded before START; a missing sequence or assigned sample prevents playback with an explanation. Transitions perform no file operations, memory allocation or bulk FX-buffer clearing. Old notes end, corresponding FX tails continue, and a short output ramp limits discontinuities. Two independent engines do not overlap.

These stress tests cover the stated scenarios and durations. They do not prove the absence of every possible future failure.
