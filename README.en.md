# MOSS

[Русский](README.md) · **English**

[Documentation RU / EN](docs/README.en.md)

A standalone groovebox for the **original Anbernic RG35XX**: a sequencer, synthesizers, sampler, effects and internal resampling. Controlled with the handheld's buttons, with no additional hardware. The shared C++17 engine also runs on Windows.

**Stable snapshot: [v0.5.2](https://github.com/Vdolganov/moss/releases/tag/v0.5.2).** Tested on the original RG35XX with **GarlicOS 1.4.9**, including hardware stress tests and user-confirmed everyday operation. This is an early version of the application; measured results and limitations are documented in the [0.5.2 verification report](docs/NATIVE-VERIFICATION-052.en.md).

RG35XX Plus/H/2024/SP and other models have not been tested for this release. GarlicOS 1.3.9 remains a compatibility target, but this particular 0.5.2 build has not been retested on it.


Current development branch **0.9.0-dev** adds [rebuilt MONO](docs/MONO-v09.en.md) with twenty controls and a live scope, [ambient GRANULAR and updated POND](docs/AMBIENT-RESEARCH-v08.en.md), plus the larger REEL visual. Saves use MS15; the published stable 0.5.2 snapshot remains a separate release.

## Features

- Up to four tracks per sequence: **MONO, ORGANISM, FM and SAMPLER**.
- Independent track cycles; 1/8, 1/16, 1/32 and 1/64 resolution; probability, note length and parameter locks. The model stores up to 512 ticks at 1/64 resolution per track; each grid page displays 16 cells.
- Four assignable samples in each SAMPLER instance, with one-shot playback, manual trimming and per-hit settings.
- **ORBIT** delay, **POND** reverb, **CRUSH** distortion/redux and **REEL** tape processing; up to two active effects on each track and on the master.
- **TAPE**: streaming audition of long WAV files, manual slicing, naming, a shared library, and track or master resampling.
- Up to 32 sequences and 32 **SONGs** with 64 slots each: a shared BPM, repeats, slot duration in bars and independent sequence copies.
- A flat 640×480 interface, SD-card storage and undo for the last edit.

The prepared sample library holds up to 32 assets, each up to 1 MiB of PCM16. Audio output is 32 kHz with a 1024-stereo-frame buffer. The application targets the stock 1.008 GHz clock, without overclocking.

## Download and run

Ready-to-use packages are available in the [v0.5.2 release](https://github.com/Vdolganov/moss/releases/tag/v0.5.2):

- **MOSS-0.5.2-GarlicOS.zip** — the application for the original RG35XX.
- **MOSS-0.5.2-Windows.zip** — the desktop version with the same engine.
- **MOSS-0.5.2-verification.json** and **SHA256SUMS.txt** — verification data and checksums.
- **MOSS-0.5.2-README-EN.md** — the English user guide, supplied separately for the original release packages.

### GarlicOS

1. Back up your existing MOSS folder and switch off the handheld before transferring files.
2. For a new installation, copy the archive's `Roms` contents into `Roms` on the card. You should have `Roms/APPS/MOSS.sh` and `Roms/APPS/MOSS/`.
3. When updating, replace the application and launcher while preserving your `projects/`, `samples/`, `library/`, `imports/` and `captures/`. Do not overwrite your own samples with identically named demo WAVs from the archive.
4. Safely eject the card, insert it into the handheld and open **Consoles → APPS → MOSS**. An empty project library is initialized with **SLOW SIGNAL**; open it with A, then press START.

The archive contains an application, not a firmware image. See the [user guide](native/prototype/README.en.md) for installation, format and update details.

### Windows

Extract the entire archive into a writable folder and run `MOSS.exe`. Keep `samples/` beside the executable. Data is stored locally. The Windows version lets you test music and controls; it does not emulate RG35XX performance.

## Main controls

| RG35XX | Windows keyboard | Action |
|---|---|---|
| D-pad | Arrow keys | Navigate |
| Y / B / A / X + arrows | Z / X / C / V + arrows | Four parameters, left to right: ←/→ fine, ↑/↓ coarse |
| A / B | C / X | Confirm / back |
| L1 / R1 | Q / E | Parameter banks and nested pages |
| L2 / R2 | [ / ] | Main tabs |
| R1 + ←/→ | E + ←/→ | Jump to another grid or list page |
| L1 + R1 | Q + E | Context menu |
| START | Space | Play / stop |
| SELECT + arrows | Tab + arrows | Global tempo |
| Hold MENU for 2 seconds | Hold Esc for 2 seconds | Save and exit |

Full controls and workflows: [0.5.2 user guide](native/prototype/README.en.md), [interaction contract](design/current/native-v052-interaction.md).

## Build from source

The supported build host is **Windows x64**, with Node.js 18+ and Python 3.9+. The setup script installs local copies of Zig 0.14.1 and a pinned uClibc SDK, verifying their SHA256 hashes. Docker/WSL is not required. The compiler and SDK are not included in the repository.

From the cloned repository's root:

```powershell
node "hardware lab/scripts/setup.cjs"
node native/prototype/scripts/build.cjs
```

If Python is not detected automatically, set `MOSS_PYTHON` to the full path of `python.exe` before the first command. The build uses the original demo WAVs in `hardware lab/samples/`.

The second command builds ARM and Windows, runs eight native test suites and creates previews in `native/prototype/build/`. The Garlic backend uses SDL 1.2 and evdev; Windows uses GDI and waveOut. The shared model, UI, sequencer and DSP are in `native/prototype/src/`.

`node native/prototype/scripts/benchmark.cjs` builds the stress-test programs. Desktop results do not replace hardware testing. The packager, `native/prototype/scripts/package.py`, additionally requires a local `build/device-verification.json` matching the exact ARM binary hash. An old release's report must not be substituted for an untested new build.

## Repository layout

| Folder | Contents |
|---|---|
| `native/prototype/` | Current application, tests, build scripts and launcher |
| `hardware lab/` | Hardware diagnostics, shared input/drawing code and SDK setup |
| `native/perf-baseline-v050/` | Historical engine used for performance comparisons |
| `visual prototype/` | Browser interaction prototype without an audio engine |
| `design/current/` | Current diagrams and interaction contracts |
| `docs/` | Decisions, development history and verification reports |
| `releases/0.5.2/` | Recorded information about the published build |

Run the browser prototype through `visual prototype/START.cmd` or `index.html`. Some features and controls differ from the native application. For 0.5.2, the native user guide and current interaction contract take precedence. Older documents are retained as development history.

The repository includes only three original test sounds generated by `hardware lab/scripts/make_samples.py`. Personal sequences, recordings and third-party sample packs are not distributed. A granular instrument is being discussed separately and is not part of 0.5.2.
