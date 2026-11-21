# MOSS

[Русский](README.md) · **English** · [Documentation](docs/README.en.md)

A standalone groovebox for the original **Anbernic RG35XX**, operated with its buttons and no extra boards. The shared C++17 engine also runs on Windows.

**Current version on `main`: MOSS 0.13.1**, with rebuilt ORGANISM and FM, folder imports and four-part kits, the monochrome UI and corrected SAMPLER/GRANULAR levels. [Level changes and verification](docs/LEVELS-v0131.en.md). The latest prebuilt downloads remain [0.9.0](https://github.com/Vdolganov/moss/releases/tag/v0.9.0); those archives do not include these changes.

## Features

- Up to four tracks using **MONO, ORGANISM, FM, SAMPLER or GRANULAR**.
- MONO: twenty controls across five banks, a saturating resonant filter, sub/noise, detune, glide, drift, PWM, LFO and measured waveform/envelopes.
- GRANULAR / LENS: eight grains per track, up to eight-second grains, scatter, reverse, random jumps, envelopes, detune and freeze.
- SAMPLER: four assignments and hit patterns per instance, one-shots, pitch, manual start/end, length and locks.
- Independent cycles up to 512 ticks at 1/64; 1/8–1/64 resolution, probability, SWING/SHIFT, note and **LOCK ONLY** events, and REPEAT audio fragments.
- **ORBIT, POND, DRIVE, CRUSH and REEL**; two active effects per track and two on the master. REEL includes saturation, colour, wow/flutter and tape defects.
- TAPE: WAV folder imports, streaming preview, manual slicing or four-part kits, fine trimming and internal resampling.
- Up to 32 sequences and 32 SONGs with 64 slots each, shared song BPM, repeats, bars and independent variations.
- 640×480 display, physical Y/B/A/X parameter order, Undo and an onscreen name editor.

Prepared library: 256 samples, up to 32 resident, each up to 1 MiB PCM16. Source catalog: 2048 WAV files. Audio: 32 kHz / 1024 stereo frames, stock 1.008 GHz without overclocking. Sequence format: **MS18**, reading MOS5…MS17; SONG format: SON1.

## Download and install

[Release 0.9.0: downloads and checksums](https://github.com/Vdolganov/moss/releases/tag/v0.9.0). This is an application, not a GarlicOS image. The measured platform is the original RG35XX / GarlicOS 1.4.9. Other RG35XX variants and GarlicOS 1.3.9 are not verified for this release; see the release's test conditions and limits.

**GarlicOS:** copy the archive's `Roms` folder onto the card. Launch **Consoles → APPS → MOSS**. Paths: `Roms/APPS/MOSS.sh` and `Roms/APPS/MOSS/`.

**Windows:** unpack the complete archive into a writable folder and run `MOSS.exe`, keeping `samples/` alongside it. It uses the console engine; desktop speed is not RG35XX performance evidence.

Back up the full installed folder before updating. Preserve `projects/`, `samples/`, `library/`, `imports/` and `captures/`; do not replace personal WAV files with demo files of the same name.

**Migrating from MOSS-GRANULAR:** back up both previous installations. Use the complete current MOSS-GRANULAR data set as the new MOSS folder, then replace its executable and launcher. Do not merge individual files from unrelated project/library sets: sequence and sample IDs can collide. After verification, move the old MOSS folder and separate MOSS-GRANULAR launcher to a backup outside APPS. See the [guide](native/prototype/README.en.md).

Older versions cannot read MS18. Rollback requires the old executable **and original data**. Updated instruments, effects and levels can change older patches' sound.

## Controls

| RG35XX | Windows | Action |
|---|---|---|
| D-pad | Arrows | Navigation |
| Y/B/A/X + arrows | Z/X/C/V + arrows | Four fields: horizontal fine, vertical coarse |
| A / B | C / X | Confirm / back |
| L1/R1 | Q/E | Parameter banks |
| L2/R2 | [ / ] | Main tabs |
| L1 + left/right; L1 + up/down | Q + arrows | Slot/region; nested parameter page |
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
- [Current silent browser prototype](<visual prototype/current/README.md>): launch `visual prototype/current/START.cmd`. The parent folder retains the earlier prototype.
- `docs/` and `design/current/`: guides, decisions and dated evidence. Old measurements are not attributed to new binaries.
- `releases/`: release records. No personal projects, recordings or third-party packs are published. Three original test WAVs are generated by `hardware lab/scripts/make_samples.py`.

## License

MOSS is released under the [MIT License](LICENSE). You may use, modify and distribute the code, including in commercial and closed-source projects, provided you preserve the license and copyright notice in copies or substantial portions of the software. The software is provided without warranty.

Third-party libraries and imported samples retain their own terms: [license scope and external components](THIRD-PARTY-NOTICES.txt).
