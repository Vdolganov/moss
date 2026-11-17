# MOSS

[Русский](README.md) · **English** · [Documentation](docs/README.en.md)

A standalone groovebox for the original **Anbernic RG35XX**, operated with its buttons and no extra boards. The shared C++17 engine also runs on Windows.

**Current stable version: [MOSS 0.9.0](https://github.com/Vdolganov/moss/releases/tag/v0.9.0).** GRANULAR is now a standard MOSS instrument. There is one application, MOSS, and one primary branch, `main`. The historical 0.5.2 release remains available.

## Features

- Up to four tracks using **MONO, ORGANISM, FM, SAMPLER or GRANULAR**.
- MONO: twenty controls across five banks, a saturating resonant filter, sub/noise, detune, glide, drift, PWM, LFO and measured waveform/envelopes.
- GRANULAR / LENS: eight grains per track, up to eight-second grains, scatter, reverse, random jumps, envelopes, detune and freeze.
- SAMPLER: four assignments and hit patterns per instance, one-shots, pitch, manual start/end, length and locks.
- Independent cycles up to 512 ticks at 1/64; 1/8–1/64 resolution, probability, SWING/SHIFT, note and **LOCK ONLY** events, and REPEAT audio fragments.
- **ORBIT, POND, DRIVE, CRUSH and REEL**; two active effects per track and two on the master. REEL includes saturation, colour, wow/flutter and tape defects.
- TAPE: WAV import, streaming preview of long sources, manual slices, naming and internal resampling.
- Up to 32 sequences and 32 SONGs with 64 slots each, shared song BPM, repeats, bars and independent variations.
- 640×480 display, physical Y/B/A/X parameter order, Undo and an onscreen name editor.

Prepared library: 32 samples, each up to 1 MiB PCM16. Source catalog: 2048 WAV files. Audio: 32 kHz / 1024 stereo frames, stock 1.008 GHz without overclocking. Sequence format: **MS15**, reading MOS5…MS14; SONG format: SON1.

## Download and install

[Release 0.9.0: downloads and checksums](https://github.com/Vdolganov/moss/releases/tag/v0.9.0). This is an application, not a GarlicOS image. The measured platform is the original RG35XX / GarlicOS 1.4.9. Other RG35XX variants and GarlicOS 1.3.9 are not verified for this release; see the release's test conditions and limits.

**GarlicOS:** copy the archive's `Roms` folder onto the card. Launch **Consoles → APPS → MOSS**. Paths: `Roms/APPS/MOSS.sh` and `Roms/APPS/MOSS/`.

**Windows:** unpack the complete archive into a writable folder and run `MOSS.exe`, keeping `samples/` alongside it. It uses the console engine; desktop speed is not RG35XX performance evidence.

Back up the full installed folder before updating. Preserve `projects/`, `samples/`, `library/`, `imports/` and `captures/`; do not replace personal WAV files with demo files of the same name.

**Migrating from MOSS-GRANULAR:** back up both previous installations. Use the complete current MOSS-GRANULAR data set as the new MOSS folder, then replace its executable and launcher. Do not merge individual files from unrelated project/library sets: sequence and sample IDs can collide. After verification, move the old MOSS folder and separate MOSS-GRANULAR launcher to a backup outside APPS. See the [guide](native/prototype/README.en.md).

Older versions cannot read MS15. Rollback requires the old executable **and original data**. Updated MONO/POND can change older patches' sound.

## Controls

| RG35XX | Windows | Action |
|---|---|---|
| D-pad | Arrows | Navigation |
| Y/B/A/X + arrows | Z/X/C/V + arrows | Four fields: horizontal fine, vertical coarse |
| A / B | C / X | Confirm / back |
| L1/R1 | Q/E | Parameter banks |
| L2/R2 | [ / ] | Main tabs |
| L1 + arrows | Q + arrows | Nested bank, slot or effect |
| R1 + left/right | E + left/right | Grid/list page |
| L1+R1 | Q+E | Context options |
| START | Space | Play / stop |
| SELECT + arrows | Tab + arrows | Tempo |
| MENU, two seconds | Esc, two seconds | Save and exit |

## Build

Windows x64, Node.js 18+, Python 3.9+. Setup installs pinned Zig 0.14.1 and a uClibc SDK with SHA256 checks. Docker/WSL are not required.

```powershell
node "hardware lab/scripts/setup.cjs"
node native/prototype/scripts/build.cjs
```

Set `MOSS_PYTHON` to a full Python path when needed. Build produces ARM/Windows binaries and runs engine, persistence, interaction and effect checks. `scripts/package.py` checks ABI, tests and exact-binary hardware evidence before producing the GarlicOS package.

## Project navigation

- [Native guide RU/EN](native/prototype/README.en.md), [MONO](docs/MONO-v09.en.md), [GRANULAR](docs/GRANULAR-INSTRUMENT.en.md), [REEL](docs/REEL-v1.en.md).
- `native/prototype/`: current app, tests and build. `hardware lab/`: diagnostics and SDK setup.
- `visual prototype/`: historical silent browser interaction prototype; launch `START.cmd` or `index.html`.
- `docs/` and `design/current/`: guides, decisions and dated evidence. Old measurements are not attributed to new binaries.
- `releases/`: release records. No personal projects, recordings or third-party packs are published. Three original test WAVs are generated by `hardware lab/scripts/make_samples.py`.
