# MOSS 0.5.1 — performance and code structure

[Русский](NATIVE-PERFORMANCE-051.md) · **English**

This report concerns 0.5.1. Current 0.5.2 verification is in a [separate report](NATIVE-VERIFICATION-052.en.md).

Testing used an original RG35XX with GarlicOS 1.4.9 over USB/ADB. These are measurements on a real ARM Cortex-A9, not estimates from desktop speed. The target ABI remains ARMv7 softfp/uClibc for GarlicOS 1.3.9–1.4.9; this new binary was not rerun on 1.3.9.

## Operating limits

| Resource | Limit |
|---|---|
| Tracks | 4 per project, using any of the four instruments |
| Instruments | SAMPLER, MONO, ORGANISM, FM |
| Effects | 2 active per track, plus 2 on the master |
| SAMPLER | 4 assignable slots per instance; 4 primary voices and bounded short retrigger tails |
| Samples | 32 prepared WAVs, each up to 1 MiB PCM; long sources stream for audition |
| Cycle | Up to 512 internal 1/64 ticks; independent track lengths and resolutions |
| Audio | 32000 Hz, stereo, 1024-frame buffer: 32 ms to process each block |
| CPU while running | 1.008 GHz, performance governor, no overclock; previous settings restored on exit |

Moving from 44.1 to 32 kHz reduces computation and lowers the upper frequency limit to 16 kHz. FM remains available: measurements showed ORGANISM to be more expensive. Track/effect limits are enforced by the model and UI, not just by the test scenario.

## Where the dropouts came from

The old 0.5.0 report `music-1794414358-160.json` contains 3,147 audio-block overruns and 120 late intervals. The maximum was 29.673 ms against a 23.22 ms budget. Counters cover the entire session; the project's final state in the report describes only the moment of exit.

A repeatable test of the saved RNUIW project with FM enabled, at the same 44.1 kHz and 1024 frames:

| Stage | Mean per block | Maximum | PCM checksum |
|---|---:|---:|---|
| Original size-optimized build | 21.483 ms | 22.650 ms | 88237b83 |
| Effect optimizations and O3 compilation | 17.555 ms | 22.427 ms | 88237b83 |
| Cached synth coefficients | 15.789 ms | 16.728 ms | 88237b83 |

This is one particular project, not maximum load. Its mean processing time fell by approximately 26.5%, without changing PCM in this comparison.

Eight ORGANISM tracks were also tested on the device at 32 kHz. Even with only two effects on each track and master, mean processing took 40.915 ms and the maximum was 41.989 ms against a 32 ms budget. With four effects, the figures were 48.441 / 49.754 ms. This led to the agreed four-track limit. Removing FM alone would not solve that problem.

## Optimizations

- Static synth/effect coefficients are recalculated when parameters, tempo or sample rate change, rather than for every audio sample.
- POND and ORBIT ring buffers use stored positions and conditional wrapping instead of frequent modulo operations. Tail state is preserved.
- Zero FIR coefficients are no longer calculated; internal 2× synth processing is retained. `fast-math` is not enabled.
- Track gain avoids expensive 64-bit division while preserving its integer result. Sample gain is prepared at trigger/change time; a separate check allows at most one PCM LSB of rounding difference.
- Completed resampling writes, waveform-overview generation and diagnostic report writes run in the background. The audio thread neither writes files nor allocates memory. Captured PCM remains immutable until writing finishes; errors retain it for retry.
- Menus reflect available actions. Saved-project sample references are cached, so drawing a menu does not reread every project from the card.
- Only the audio thread receives Linux SCHED_FIFO priority 10; UI and SD writes remain normal threads. Between blocks, the audio thread waits for ALSA, and processing is bounded by the stated budget. The device report confirms successful scheduling setup. The calling-thread semantics were checked against [Linux man-pages](https://man7.org/linux/man-pages/man2/sched_setscheduler.2.html).

## Test method

Maximum-load scenario: four tracks, the longest cycle, 240 BPM, events on every 1/64 tick, four simultaneous slots in each sampler, different pitches, maximum levels, changing extreme/intermediate voice and FX locks, and both synth modes. All 32 one-MiB sample buffers are filled. Track and master effects are enabled, master resampling runs, and the interface changes screens, banks and selected steps.

All six effect pairs were tested on ARM for all four instrument types. The most expensive pair is POND + REEL, used by the long full-application test with real SDL, display and ALSA. Diagnostic runs silence output after signal processing so the speaker does not play prolonged test noise. Audio-driver logs are also checked.

<!-- DEVICE_RESULTS -->
The final build with audio-thread priority passed on the physical device. Every row below uses four tracks, POND + REEL on each track and master, 32 MiB of samples, master capture and changing screens.

| Scenario | Duration | Mean / maximum audio block | P99 upper bound | Maximum UI frame | Overruns / late intervals |
|---|---:|---:|---:|---:|---:|
| 4 ORGANISM | 600 s | 20.108 / 25.816 ms | 21.25 ms | 11.338 ms | 0 / 0 |
| SAMPLER + MONO + ORGANISM + FM | 60 s | 17.724 / 20.836 ms | 19.00 ms | 11.176 ms | 0 / 0 |
| 4 SAMPLER | 60 s | 15.890 / 20.554 ms | 17.00 ms | 7.200 ms | 0 / 0 |
| 4 FM, installed binary | 30 s | 16.350 / 17.753 ms | 17.50 ms | 7.406 ms | 0 / 0 |

All runs finished normally, capture started and finished, and ALSA logs were empty. P99 is the upper boundary of a 250 µs histogram bucket. A UI frame is slow after 33.333 ms; none were slow. The long run additionally included four 3.1 MB USB copies to the same card and reading/hashing the original projects.

The previous candidate, with background writing but normal audio priority, also had no overruns in 600 seconds, but its maximum reached 31.603 ms against a 32 ms budget. Audio-thread-only priority was added and retested afterward. Earlier synchronous writing caused UI pauses up to 480 ms; the table shows the background-writing result.

The current ARM headless binary also completed 24 combinations: four instruments × six effect pairs, each with three seconds of measured audio after warmup. None exceeded the budget. This short sweep supplements the long application test rather than replacing it.

During the long run, resident memory was approximately 45.5 MiB, peak virtual space approximately 81.6 MiB, and no swap was used. Reserved main objects occupy 58.84 MB; ELF load memory is 63.21 MB. No new libraries are required: dependencies are Garlic's libSDL-1.2 and libc.

Final ARM binary SHA256: `715af4c3a03728318d23854d53de470e41301d747a49f7de167971008e736f10`.

Raw reports, logs, before/after results and the installation manifest are retained in the local `MOSS-0.5.1-qa` verification archive. That working archive is not included in the public repository.
<!-- /DEVICE_RESULTS -->

Windows tests additionally imposed a 256 MiB process-memory limit. Four-ORGANISM and four-SAMPLER scenarios with the expensive effect pair completed, with approximately 60.6 MB private memory. This checks memory budget and reproducibility, **not ARM performance emulation**. A 1% CPU quota was also tested: it caused periodic long process suspensions, with nine overruns, while preserving the PCM hash. This limits CPU usage but does not reproduce the steady speed of a Cortex-A9, so it is not a console acceptance criterion.

Seven automated test suites cover the sequencer, polyrhythms, resolutions, simultaneous hits, LENGTH and one-shot behavior, effect tails across rests, locks, command queues, hold/release order, project migration, import and write errors. Name-editor checks cover replacement, insertion, deletion, length limit, cancel, failure and save/create retry. A separate DSP trace compared 20 combinations of rates and dynamic parameters against the retained baseline; effect and synth hashes matched.

## Implementation structure

| Area | Files under `native/prototype/src` |
|---|---|
| Shared limits and field IDs | `config.h` |
| Musical data and editing | `model.h`, `parameter_spec.h`, `effect_defs.h`, `synth_defs.h` |
| Sequencer, commands and mixer | `engine.h` |
| Synthesis and processing | `synth.h`, `sample_voice.h`, `effects.h`, `dsp_math.h` |
| PCM, library and streaming | `sample.h`, `library.h`, `stream_reader.h` |
| Resampling and background writing | `capture.h`, `capture_store.h` |
| Input, projects, names and menus | `app.h`, `app_management.h`, `app_menu.h`, `name_keyboard.h` |
| Drawing and shared parameter fields | `app_ui.h`, `app_tools_ui.h`, `app_synth_ui.h`, `parameter_ui.h` |
| Format and atomic saves | `storage.h` |
| Platform input/audio/window | `main_garlic.cpp`, `main_windows.cpp` |
| Shared stress scenario | `diagnostics/stress_scenario.h` |

`app_management.h` and `app_menu.h` are explicitly identified parts of App; `app_ui.h` is its separate renderer. They are not presented as independent libraries. Structural project changes reach the engine with audio stopped; quick edits use a bounded command queue. Shared parameter definitions and limits are used by the engine, UI and tests. Class hierarchies were not added merely for formal SOLID compliance.

## Reproducing the tests

From `native/prototype`, `node scripts/build.cjs` builds ARM/Windows and runs the seven suites associated with 0.5.1; the current 0.5.2 build adds the eighth, v052. `node scripts/benchmark.cjs` builds comparable headless workloads for both platforms. Toolchain and O3 flags are defined in `scripts/toolchain.cjs`; the SDK is pinned through the existing hardware lab setup.

Windows: `build/performance.exe <bank-folder> 20 - 32000 1024 256 0 stress-4-ORGANISM 10`. The final number is the effect bitmask; 10 means POND + REEL. The program loads the user's bank, then creates a full synthetic library for stress testing. The measured region contains only audio rendering.

On the console, `moss --profile 2 10 600` runs a separate 600-second ORGANISM scenario; 0 selects SAMPLER, 1 MONO, 3 FM and -1 a mixed project. Its directories are under `perf/run`; user projects are not changed. The full GUI test launches through Garlic's standard one-shot `CFW/retroarch/command.sh` after its menu exits, rather than competing with the menu for the display. `scripts/profile-command.sh` provides a scenario batch; an existing `command.sh` must not be overwritten.

Packages are checked for ARMv7 Cortex-A9 softfp/uClibc ABI, the expected loader, allowed libraries/symbols, a non-executable stack and a hash matching the tested binary. Installation replaces executables and the guide while retaining projects, WAVs, imported sources and captures. MOS5 loads with the agreed limits; MOS6 stores the new LENGTH field. Original data is backed up to the computer first.

Results support the listed scenarios on this device. They do not establish stability for arbitrary future versions, a faulty card or different overclocking. DSP or limit changes require new stress testing.

## Normal-launch fix — November 11, 2026

During direct installation, MOSS.sh retained CRLF line endings, causing Garlic to return immediately to its menu. Earlier stress tests invoked the binary directly and did not check that launcher. The launcher was converted to LF and reinstalled. Logs and the user confirmed normal launch through MOSS.sh: the application opened and loaded projects and the library. The audio binary was unchanged. Build and packaging now reject incorrectly formatted launchers; .gitattributes enforces LF.
