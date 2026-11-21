# Levels 0.13.1 — verification / проверка

21 November 2026. Original RG35XX, GarlicOS 1.4.9, stock performance governor; 32 kHz / 1024 stereo frames, 32 ms deadline.

## Finding and change

File preview was unity, while the project bus used MASTER / 300 followed by 180% output gain. At 100% controls this was a fixed 0.6 multiplier. Default 75% track/master and 80% sample slots attenuated it further. GRANULAR used another nominal 0.75 coefficient. These were software gain stages, not a loss introduced by slicing.

The shared bus now uses MASTER / 100, keeps summing headroom through master FX and retains soft protection at the final output. No hidden output boost remains. Defaults are 100% for track/master/slot; instrument audition uses velocity 127. Stored mix values and sequenced velocities remain intact. GRANULAR nominal gain is 1, retaining overlap/window compensation.

See [RU](../../LEVELS-v0131.md) / [EN](../../LEVELS-v0131.en.md) for controls, measurements and behavior of existing projects.

## Signal measurements

`levels_test.cpp` measures the actual Engine output with bypassed effects and latency settled. SAMPLER at all 100%, velocity 127, equals file preview at 6000 RMS: **0.00000 dB** difference (previously −4.43697 dB). Instrument audition has the same level. New default sequenced velocity 90 gives 4251 RMS, **−2.99320 dB**, matching 90/127.

For a dense cloud from decorrelated band-limited stereo material, source RMS is 1984.072 and granular RMS is 1870.179 (**−0.51348 dB**, formerly −7.45805 dB). This is one calibration scenario; filter, window, source correlation, pitch and density can change the level of other granular patches.

The same assertions and measurements passed on Windows and on ARM. Tests include half/quarter mix gain, silence, two-track summing, 16 simultaneous high-level samples, symmetric bounded peak protection, and preservation of saved mixer settings. Separate sample-edit tests verify PCM-preserving slices and exact equality of master capture to monitored output.

## Short device load tests

Each profile ran for **20 seconds**, with four 512-step tracks at 240 BPM, all levels 100%, full PCM cache and two effects on each track plus master. Mixed tests switch UI views and dispatch edits while capture is active. These are short regressions, not an indefinite reliability guarantee.

| Profile | Max audio block | Over budget | Late interval | Stream underrun |
|---|---:|---:|---:|---:|
| 4 SAMPLER / 16 voices, REEL + POND | 26.109 ms | 0 | 0 | 0 |
| 4 GRANULAR / 32 grains, REEL + POND | 28.259 ms | 0 | 0 | 0 |
| GRANULAR + MONO + ORGANISM + FM, REEL + POND, UI edits | 28.830 ms | 0 | 0 | 0 |
| SAMPLER + MONO + ORGANISM + FM, DRIVE + ORBIT, UI edits | 20.822 ms | 0 | 0 | 0 |

All four raw reports and the tested binary SHA-256 are retained here. `project_saved:false` at the end of a synthetic capture profile concerns its isolated test state; it does not refer to user projects.

## Build and installation

The full ARM/Windows build, native regression suite and 26 browser model/timing checks passed. Historical assertions were updated only where they explicitly required the retired 180% monitor gain or 80% slot default. The baseline run is preserved separately.

Before installation, the current device projects, prepared library, captures and previous binary/launcher were backed up locally. Installation used a hash-checked temporary binary and same-filesystem rename. The ordinary Garlic launcher was checked and MOSS exited normally. User data hashes were compared with that fresh backup; saved mixer levels were not rewritten. The console was left in Garlic after sync.

Evidence: [host build checks](checks.json), [ARM level checks](levels.txt), [baseline](baseline.txt), [deployment](deployment.json), [installed screen](installed.png).

По-русски: исправлен общий уровень тракта и номинальный уровень грануляра; новые уровни микшера и слотов — 100%. На компьютере и консоли подтверждено совпадение с предпрослушиванием при калибровочных настройках. Четыре коротких нагрузочных проверки прошли без пропусков; пользовательские файлы сохранены.
