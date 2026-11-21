# FM 0.12 verification / Проверка FM 0.12

19 November 2026. Original RG35XX, GarlicOS 1.4.9, 32 kHz stereo / 1024-frame callback (32 ms deadline).

## Final installed build / Итоговая сборка

`972725d80590ef20c64e3975261afa36c5e11721804c03ce408add87741e9b9d`

Four 10-second runs on the deployed binary (v36); 314 callbacks each. / Четыре прогона по 10 секунд на установленном бинарнике, по 314 callback.

| Profile | Peak callback, ms | Mean, ms | Over budget / late / underrun |
| --- | ---: | ---: | --- |
| final-held | 27.039 | 23.115 | 0 / 0 / 0 |
| final-mixed | 28.577 | 25.580 | 0 / 0 / 0 |
| final-orbit-reel | 27.381 | 23.497 | 0 / 0 / 0 |
| final-pond-reel | 28.610 | 24.845 | 0 / 0 / 0 |

Worst final peak: 28.610 ms; 3.390 ms below the callback deadline. Short tests establish the observed margin, not an unlimited worst-case guarantee. / Максимум итоговых прогонов 28,610 мс, запас 3,390 мс. Это наблюдение коротких тестов, не гарантия для любого будущего сценария.

## Coverage / Покрытие

- Four FM tracks, 512 canonical timeline ticks each, fastest 1/64 clock at 240 BPM, dense note/parameter events, stereo, maximum sound parameters and both enabled FX on each track/master.
- Long held FM notes with lock-only events; mixed GRANULAR/MONO/ORGANISM/FM with eight simultaneous grains and capture.
- All ten two-effect combinations in v33/v35. Final v36 repeats expensive ORBIT+REEL and POND+REEL plus held and mixed scenarios after correcting initial click gain.
- ARM unit test: all twenty controls measurably affect audio; 27.5/55/110 Hz fundamentals at 8/32/48/96 kHz, bounded DC/output, stereo sustain, MS17 migration and MS18 roundtrip, held-note continuity and live ratio/velocity-sensitive pitch telemetry.
- Full desktop suite passed before the final initial-gain refinement; final targeted synth, v05 control, FM and trigless/repeat regressions passed. Audio metrics are numerical, not a subjective listening assessment.
- Native renderer captures and real framebuffer captures in `.impeccable/review/fm-v012`. Independent review resolved two telemetry findings; its final SHIP is scoped to those fixes.

## Data and install / Данные и установка

82 pre-existing project/library files match their pre-update SHA256 hashes after normal installed launch and exit. Added sample-free ELASTIC MATTER in project slot 7. No user project was rewritten. / Все 82 прежних файлов проектов/библиотеки совпадают по SHA256; добавлено только демо.

Previous program and projects: `/mnt/mmc/MOSS-Backups/20261119-fm-v012`. Full project/library backup also lives on the computer in `work/fm/preinstall`. Console left in Garlic menu after sync, governor interactive.

Invalid CLI attempts v32/v34 are explicitly excluded from performance evidence. Detailed raw reports, desktop log and ARM unit log are adjacent to this file.
