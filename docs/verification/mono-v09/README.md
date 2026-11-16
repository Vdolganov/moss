# MONO 0.9 · verification / проверка

2026-11-16. Original RG35XX, GarlicOS 1.4.9, stock maximum 1.008 GHz, 32 kHz stereo / 1024-frame callbacks. Budget: **32 ms**. Four tracks, two effects per track plus two on master, 32 MiB prepared PCM pool; changing UI, locks and internal capture remain active.

## Короткие прогоны / short runs

| Scenario / сценарий | Seconds | Mean ms | p99 upper ms | Maximum ms |
|---|---:|---:|---:|---:|
| 4 MONO, PULSE, maximum controls, REEL + POND | 20 | 24.585 | 26.250 | 27.887 |
| 4 MONO, changing all 20 locks, REEL + POND | 20 | 25.476 | 26.750 | 28.316 |
| 4 MONO, changing locks, ORBIT + REEL | 15 | 24.248 | 25.750 | 27.096 |
| SAMPLER / MONO / ORGANISM / FM, REEL + POND | 15 | 25.955 | 27.000 | 29.154 |
| 4 GRANULAR, 32 live grains, fixed heavy REEL + POND | 20 | 26.799 | 29.250 | 30.078 |
| SONG transitions, all five instrument/effect types observed | 15 | 19.757 | 26.750 | 27.737 |

**105 seconds, 3289 callbacks.** All six completed with zero over-budget callbacks, late intervals, stream underruns, schedule overflows or real-time scheduling errors. Highest observed utilization: **94.0%**, leaving **1.922 ms** in that block. These short representative stress cases are evidence for this binary, not an exhaustive guarantee for every possible patch or a long soak. Profile audio is rendered fully but muted after measurement at the loudspeaker.

**105 секунд, 3289 блоков.** Во всех шести прогонах — ноль превышений бюджета, опоздавших интервалов и ошибок потока/планировщика. Максимум занял **94,0%** бюджета; оставалось **1,922 мс**. Это короткая проверка выбранных тяжёлых сценариев, не обещание для всех комбинаций параметров и не длительный прогон. В нагрузочном режиме звук полностью рассчитывается, затем заглушается перед динамиком.

The full MONO profile keeps every control at maximum except ATTACK=3 ms; each note retriggers rapidly at the fastest step division and the maximum 512-unit timeline. Separate changing-lock profiles exercise extremes rather than only a stationary patch. Effect count is still capped at two. The SONG run observed effect mask 31 and instrument mask 31, not every conceivable ordering/setting.

The profiler was corrected so its fixed heavy settings are applied **before** copying the project into the engine. This also corrects the earlier profile 21 setup: its historical results should not be cited as proof of fixed maximum REEL/POND settings. The new granular report confirms eight simultaneous grains on each of four tracks. Initial probe [v14](probes/v14.json) exited before audio because the CLI still rejected profile IDs above 21; it is not counted as a measurement.

## Sound and correctness / звук и корректность

- [Host suite](host-suite.txt), [undefined-behaviour checks](host-ubsan.txt), [ARM unit tests](arm-unit.txt): both waves, control extremes, rapid locks, 8/32/44.1/96 kHz, bounded finite output, legato phase/envelope retention, glide, release completion, live targets and all twenty control/lock slots.
- [Persistence](user-project-read.txt): actual user project read without rewriting it, plus a genuine MS14 field layout and MS15 round trip in the unit tests. Added old-patch modulation is neutral.
- [Bass comparison](bass-analysis.json): old commit `4e7def3`, E1/A1/E2, SAW, cutoff 1200 Hz, resonance 0/85, ENV AMT/SUB/NOISE=0, DRIVE=20, velocity100. Analyse the final two seconds of each three-second render with Hann-windowed projection at the fundamental. Fixture: `native/prototype/tests/mono_bass.cpp`, compiled against old/current headers; `MOSS_LEGACY_MONO` selects the old call signature. Float files contain the quantized integer voice output, before effects/master gain.
- [Preset render measurements](audio-analysis.json): four dry 12-second phrases, peaks 0.623–0.883, no nonfinite values or samples hitting the voice limit. Reproduce with `mono-render.cjs` and `mono-analyze.py`. No comparative human audition or analogue-output recording is claimed.

Бас сравнивается при одинаковых настройках, до эффектов и мастер-усиления. Цифровые измерения и тесты стабильности не заменяют прослушивание; запись аналогового выхода консоли не делалась.

## UI and deployment / интерфейс и установка

The [independent visual review](../../../.impeccable/review/mono-v09/review.md) returned **SHIP** for the eight native captures. [Documentation](../../../.impeccable/review/mono-v09/documentation.md) records the unchanged system. Additional `device-*.png` captures in the same folder come from RG35XX framebuffer RGB565, not desktop simulation.

On the isolated device fixture: L1/R1 reached all five banks; Y+right changed LFO RATE 0.18→0.19 Hz, then Y+up changed it to 0.29 Hz. STOP flattened the scope; MENU saved and returned to Garlic. Its separate [interactive report](ui-runtime.json) has a maximum callback of 4.354 ms and no over-budget callback or stream underrun, but **one late interval**. The existing interval counter includes the deliberate audio pause while opening a project; this session did open one before playback. The report has no per-event timestamps, so that gap is retained rather than silently discarded or claimed as a clean stress result. The six uninterrupted stress profiles above are the performance qualification.

На отдельном тестовом проекте проверены страницы, точный и крупный шаг, остановка осциллограммы и выход с сохранением. В интерактивном отчёте сохранён один длинный интервал; счётчик также учитывает штатную паузу при открытии проекта и не содержит времени отдельных событий. Это не скрыто и не включено в шесть чистых нагрузочных прогонов.

Installed only in **MOSS-GRANULAR**, leaving stable MOSS unchanged. The installed binary was read back and matched [manifest](manifest.json) SHA-256 `65d2f0a28e468eb02e9b4911c159b1b032a845b16b762799c9c47a453bf5907d`. Previous binary is `moss.before-mono-090`; the original project/library backup is retained locally. Normal launch displayed 0.9.0-dev and the existing library, then exited successfully. [Deployment verification](deployment.json) confirms **66 unchanged user files** after launch and return to Garlic. The computer was not rebooted.

Установлена проверенная сборка только в MOSS-GRANULAR. После обычного запуска и выхода сверены хеши **66 файлов** проектов и библиотеки — изменений нет. Предыдущий бинарник и резервная копия данных сохранены. Консоль оставлена в меню Garlic; компьютер не перезагружался.

## Reproduce / повторить

1. `node native/prototype/scripts/build.cjs`
2. Copy `native/prototype/build/moss` to `native/prototype/build/reel-release/moss` (the runner retains its historical name).
3. With MOSS closed and Garlic menu connected over ADB, use a **fresh** revision directory: `python native/prototype/scripts/reel-release-device.py vNN mono-max:24:10:20 mono-locks:22:10:20 mono-orbit:22:9:15 mixed:23:10:15 granular:21:10:20 song:20:10:15`.
4. After `complete`, run `python native/prototype/scripts/reel-release-collect.py vNN docs/verification/mono-v09-new`.

The runner uses isolated directories and never installs the candidate over the user's application. Review source/setup and back up current data before a separate installation. MS15 saves require a matching binary; rolling back also requires original project files.
