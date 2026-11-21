# Native UI 0.13 — verification / проверка

20 November 2026. Original Anbernic RG35XX, GarlicOS 1.4.9, stock 1.008 GHz performance governor, 32 kHz / 1024 stereo frames (32 ms audio deadline).

## Scope and source

The approved monochrome Interface Lab design was frozen locally before implementation. Source snapshots: `work/ui-port-2026-11-20/before/approved-browser` and `before/native-src`. Current contract: [native-ui-v013](../../../design/current/native-ui-v013.md). [Russian controls](../../UI-v013.md) · [English controls](../../UI-v013.en.md).

This ports the visual and hardware interaction model. It preserves native audio engines, WAV processing, resource limits and MS18 storage. Native telemetry is real; browser wrapper and simulated audio are not included.

## Software checks

[checks.json](checks.json) records 26 passing suites: sequencer/timing, locks/repeat/swing, all instruments and effects, sample folders, exact slice PCM, capture output, and UI workflows. UI coverage includes scoped clear, explicit per-field lock actions, both chord orders and same-face confirmation, modal shoulder isolation, effect identity and bypass, nested axes, view memory, accepted-edit undo, kit confirmation/assignment refusal, draft roundtrip/write failure, SONG duplicate/recovery, 16/64 pagination, one-shot duration with pitch/trim/live locks, and empty SONG controls.

The acceptance run stopped on a new test fixture that changed a SampleBuffer after Engine had already copied its SampleView. The test fixture was corrected; the runtime duration code was unchanged. UI tests, Windows build and visual fixtures then passed. Logs retain this history; the failed invocation is not labelled a complete build pass. A final modal-only visual fix was separately rebuilt and checked.

## Short device profiles

All dense profiles use four tracks, 512 steps, 240 BPM, two active effects per track plus master and 32 MiB resident PCM. Mixed profiles also switch instrument/effect/grid views and issue real UI edits. Capture runs concurrently where indicated in the JSON.

| Scenario | Seconds | Max audio block | Late / over deadline / stream underrun |
|---|---:|---:|---|
| Four samplers, 16 voices; REEL + POND | 20 | 26.808 ms | 0 / 0 / 0 |
| Four FM; REEL + POND | 20 | 29.225 ms | 0 / 0 / 0 |
| Four granular, 32 grains; REEL + POND | 20 | 27.512 ms | 0 / 0 / 0 |
| Mixed sampler/synths; DRIVE + ORBIT; UI edits | 20 | 22.053 ms | 0 / 0 / 0 |
| Initial mixed granular/synths; REEL + POND; UI edits | 20 | **32.323 ms** | **0 / 1 / 0 — failed** |
| Optimized mixed granular/synths; REEL + POND; UI edits | 30 | 29.311 ms | 0 / 0 / 0 |
| Final mixed acceptance, before modal backdrop cleanup | 20 | 28.980 ms | 0 / 0 / 0 |
| Installed final binary, after modal backdrop cleanup | 20 | 29.546 ms | 0 / 0 / 0 |

The initial failure exposed expensive full-project copies in repeated UI edits. Scalar edits now compare affected values; event edits compare only a step and its four hits. A project snapshot is still made once for an accepted gesture. In the same mixed scenario, peak UI frame time dropped from 17.906 to 11.916 ms, with no audio deadline excess in the repeat test. Final build identity and last smoke result are recorded in the JSON files alongside this report.

These are short regressions, not proof of indefinite stability or every possible parameter combination. `project_saved:false` in profiles ending during capture describes isolated test state, not failure to save a user's sequence. Audio duration lines are planned source/gate duration, not a waveform or effect-tail meter.

## Ordinary launcher and data

The device backup includes 10 project-related files, 80 library files, five captures and the previous binary/launcher. Large original import packs were not touched. Hash comparisons before installation and after normal launch/exit show the same project, library and capture files. Installation uses a verified temporary binary followed by rename on the same filesystem. See [deployment.json](deployment.json).

The first normal-launch session opened an existing project and exited with status 0. It recorded zero over-budget callbacks and zero stream underruns, but one late interval and a 67.937 ms UI iteration. The logger includes deliberate SDL audio suspension and file work when loading a project; this is consistent with that load pause, not a clean full-load profile. We retain the raw [ordinary-session report](installed-session.json) and do not fold it into the zero-late stress results. Normal project loading is not a seamless live SONG transition.

The final visual cleanup was also checked through the ordinary launcher: the options menu and rename keyboard were opened, naming was cancelled and exit completed normally. See [final session](final-installed-session.json) and [startup/exit log](final-installed-startup.log). The data comparison was repeated against the original backup. The console is left in the Garlic menu after a filesystem sync. No GitHub push was performed.

## Screenshots

Actual framebuffer: [project library](installed-atlas.png), [existing pattern](installed-pattern.png), [MONO](installed-mono.png), [lock menu](device-lock-menu.png), [applied lock](device-lock-applied.png), [final options](installed-options.png), [final name keyboard](installed-name-keyboard.png).

Native renderer fixtures: [64 cells](pattern64.png), [16 cells](pattern16.png), [FINE waveform](tape-fine.png), [name keyboard](name-keyboard.png), [FM](voice-3.png). Fixtures use generated test data, not screenshots embedded into the app.

## По-русски

Перенос проверен на компьютере и оригинальной RG35XX. После устранения лишнего копирования проекта тяжёлый тест с четырьмя дорожками, REEL/POND, записью и изменениями UI укладывается в 32 мс без пропусков. Начальный провал и все последующие результаты сохранены. Отдельная загрузка существующего проекта включает остановку аудио для файловой операции; её задержка не считается доказательством чистого нагрузочного прогона. Пользовательские проекты, подготовленные сэмплы и записи проверены по хешам и не изменились. Управление и границы переноса описаны в [памятке](../../UI-v013.md).
