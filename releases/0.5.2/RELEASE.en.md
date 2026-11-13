# MOSS 0.5.2

[Русский](RELEASE.md) · **English**

The first published stable snapshot of MOSS for the original Anbernic RG35XX. Tested on GarlicOS 1.4.9 and confirmed by the user after normal launch and use.

## Included

- Four tracks: MONO, ORGANISM, FM and SAMPLER, with four assignable samples per sampler instance.
- Independent track cycles/resolutions, probability, note length, and instrument/effect parameter locks.
- ORBIT, POND, CRUSH and REEL: up to two active effects per track and on the master.
- TAPE: WAV import, long-source audition, manual slicing, sample naming and internal resampling.
- SONG: up to 64 slots, shared BPM, repeats and slot duration in bars; independent sequence variations.
- Fine/coarse editing with the four parameter buttons, page jumps with R1+arrows, and visible MIX tabs.

## Downloads

- **MOSS-0.5.2-GarlicOS.zip** — application for the original RG35XX with GarlicOS.
- **MOSS-0.5.2-Windows.zip** — Windows version with the same musical engine.
- **MOSS-0.5.2-verification.json** — ABI, binary, test and stress-run information.
- **SHA256SUMS.txt** — release-file checksums.
- **MOSS-0.5.2-README-EN.md** — the English guide as a separate file, with its checksum in **SHA256SUMS-docs.txt**. The original ZIPs and their checksums are unchanged.

Application sources are pinned by `v0.5.2`; GitHub generates Source code archives from that tag. Later documentation translations are available on `main` and through the separate English guide. Only three original test WAVs are included; personal projects and third-party sample packs are excluded.

## Installation

For a new installation, copy `Roms` from the GarlicOS archive into `Roms` on the card and launch **Consoles → APPS → MOSS**. This is an application, not a firmware image. When updating, preserve your projects, samples, library, source files and captures; package demo WAVs must not replace user files with matching names.

Extract the entire Windows archive into a writable folder and run `MOSS.exe`.

[English user guide](https://github.com/Vdolganov/moss/blob/main/native/prototype/README.en.md) · [Build from source](https://github.com/Vdolganov/moss/blob/main/README.en.md)

## Verification

A fresh publication build passed eight native test suites. The ARM binary matched the hardware-tested build byte for byte:

`e22b6a3da14a8ae88a4d4db7015ad5760374d96ce78ae6c3bd0bdaaac473c942`

Earlier physical-device runs used stock 1.008 GHz, 32 kHz and 1024-stereo-frame blocks:

- 180 seconds of SONG with 32 unique sequences, 64 slots and a transition every second: maximum audio block 22.596 ms out of 32 ms.
- 60 seconds of four ORGANISM tracks with POND + REEL on every track and master: maximum 21.457 ms.
- Both recorded zero audio-budget overruns, late intervals or slow frames, with background capture and saving enabled.

These results apply to the stated scenarios, not every possible load. [Full test conditions and limitations](https://github.com/Vdolganov/moss/blob/main/docs/NATIVE-VERIFICATION-052.en.md).

GarlicOS 1.3.9 remains a compatibility target, but this 0.5.2 build has not been retested on it. Other RG35XX models are untested. The granular instrument is not part of this release.
