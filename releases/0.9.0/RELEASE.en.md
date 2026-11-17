# MOSS 0.9.0

[Русский](RELEASE.md) · **English**

The current application is promoted to stable **MOSS**. GRANULAR is included in the main app; there is no separate MOSS-GRANULAR distribution. Primary branch: `main`; tag: `v0.9.0`. Historical 0.5.2 remains available for rollback.

## Since 0.5.2

- GRANULAR / LENS: eight grains per track, up to eight-second grains, reverse, scatter, random jumps, detune, envelopes and freeze.
- Rebuilt MONO: twenty controls/five banks, stronger bass retention with resonance, glide, drift, PWM, LFO, independent filter decay and a measured scope.
- Redesigned DRIVE, ORBIT, POND and REEL. Five effect types, two active per track and two on the master.
- LOCK ONLY changes a sounding voice without retriggering; RESET, REPEAT audio fragments at 1/8–1/64, track SWING and step SHIFT.
- 2048 source WAVs. Four tracks, four slots per SAMPLER instance, TAPE and SONG remain available.
- One application name/launcher and Russian/English documentation. MS15 saves, reading MOS5…MS14; SONG remains SON1.

## Downloads

- **MOSS-0.9.0-GarlicOS.zip** — original RG35XX / GarlicOS.
- **MOSS-0.9.0-Windows.zip** — the same engine on Windows.
- **MOSS-0.9.0-source.zip** — sources/build materials; GitHub also generates source archives from the tag.
- **MOSS-0.9.0-verification.json**, **SHA256SUMS.txt** — checks and hashes.
- **MOSS-0.9.0-README-RU.md / README-EN.md** — manuals; both are also in the installation archives.

Only three original test WAVs are distributed. Personal sequences, recordings and third-party packs are excluded.

## Updating

Launch `Roms/APPS/MOSS.sh`, using `Roms/APPS/MOSS/`. See [README](https://github.com/Vdolganov/moss/blob/v0.9.0/README.en.md) and the [guide](https://github.com/Vdolganov/moss/blob/v0.9.0/native/prototype/README.en.md).

Back up both previous installations. To migrate from MOSS-GRANULAR, use its entire current data set as the new MOSS folder, then replace the executable and launcher. Do not merge individual `project-N.moss`/`asset-N.wav` files from unrelated libraries: IDs may collide. After verification, move the old MOSS folder and separate MOSS-GRANULAR launcher to a backup outside APPS. Copying the new application does not itself remove or convert user projects.

Older versions cannot read MS15; rollback also requires original projects. Updated MONO/POND change older patches' sound. This publication **does not update the owner's installed application**: the binary was tested separately and installation was deferred at their request.

## Exact-binary verification

ARM SHA-256: `f56568ab3a2e6171d9fcb6dc62f6670b80e9b58482bf59007d31505904fd3339`.

Original RG35XX, GarlicOS 1.4.9, stock 1.008 GHz, 32 kHz / 1024 stereo frames; budget **32 ms**. Six independent profiles, **85 seconds / 2666 callbacks**. Four tracks, two effects per track/master, full 32 MiB PCM pool, locks, UI and background capture/writeback.

| Scenario | Seconds | Maximum callback, ms |
|---|---:|---:|
| Four MONO, maximum controls, REEL + POND | 15 | 27.744 |
| Four MONO, changing locks, REEL + POND | 15 | 28.374 |
| Four MONO, ORBIT + REEL | 10 | 26.211 |
| SAMPLER / MONO / ORGANISM / FM, REEL + POND | 10 | 29.020 |
| Four GRANULAR, 32 grains, REEL + POND | 20 | 29.337 |
| SONG transitions, all instrument/effect types | 15 | 28.549 |

All six report zero over-budget callbacks, late intervals, stream underruns and scheduling errors. Maximum **29.337 ms**, headroom **2.663 ms**. These are short selected scenarios, not a long soak or a guarantee of every parameter combination. [Raw reports](device/manifest.json).

The full host suite passed. The unified MOSS.sh separately opened 0.9.0 / SLOW SIGNAL and exited normally with code 0. The owner's installation was unchanged. Other RG35XX variants and GarlicOS 1.3.9 were not tested with this binary.
