# ORGANISM 0.10 — verification / проверка

Date: 18 November 2026. Original RG35XX, GarlicOS 1.4.9, stock 1.008 GHz performance policy restored to interactive on exit. Production SDL stereo audio at 32 kHz, 1024 frames: **32 ms callback budget**.

Final ARM SHA-256: `1758e8993b075d42125a588dd3943a5968613e7c10602093b6af755a2adde38c`. The same binary was tested, installed and pulled back for hash verification. [Manifest](manifest.json), [deployment](deployment.json), [desktop suite](desktop-tests.json).

## Русский

Финальные **15 сценариев по 10–15 секунд**, суммарно **165 секунд / 5176 callback**, прошли без превышений бюджета, поздних интервалов, потерь потока и переполнений планировщика. Максимум — **30,827 мс из 32 мс**. Это ограниченный аппаратный замер, не гарантия любой будущей нагрузки и не оценка качества звука на слух.

Проверены четыре дорожки, максимальная длина, частая смена нот и всех локов, все десять пар эффектов на дорожках и мастере, 32 МиБ PCM, запись результата и работа интерфейса. Отдельные сценарии — длинная нота с локами без новых нот, смешанный проект, MONO, восемь гранул на каждой из четырёх дорожек и SONG. Все прежние ограничения приложения сохранены.

Отдельные тестовые программы выполнены **на ARM устройства**: крайние значения, обе топологии, частоты 8/32/44,1/96 кГц, границы сигнала, legato, glide, отпускание, все банки, лок без новой ноты, MS16 и миграция MS15. В изолированном измерении A1 амплитуда основного тона — 0,533; при полной DENSITY — 0,588, без прежнего провала. SUB добавляет A0 с амплитудой 0,483. SPREAD=0 даёт одинаковые левый и правый каналы. Это амплитуды цифрового выхода, не замер наушников.

Вручную через реальные события кнопок проверены открытие VOICE, смена страниц R1, Y+вправо (+1), Y+вверх (+10), START, неподвижный REST после STOP и выход MENU. Снимки — в `.impeccable/review/organism-v010/`. В отдельной интерактивной сессии с USB-управлением и чтением framebuffer зарегистрирован **один поздний интервал**, без превышения времени обработки (максимум 6,540 мс) и без stream underrun; причина не установлена. При первом запуске установленной версии также был один поздний интервал (максимум обработки 5,684 мс). Повтор открытия секвенции при остановленном транспорте дал 0→0 поздних интервалов; загрузка как причина не подтверждена. Эти результаты не скрыты и не подменяют чистые нагрузочные серии: [интерактивная сессия](interactive-session.json), [повтор загрузки](stopped-load-check.json).

Перед установкой обе прежние библиотеки, проекты, бинарники и launcher сохранены на компьютере. Установленная свежая библиотека проверена по хешам: **70 пользовательских файлов без изменений**, плюс новый независимый LIVING MATTER в слоте 6. Старый каталог MOSS сохранён на карте в `/mnt/mmc/MOSS-Backups/20261118-organism/MOSS-legacy`; свежий MOSS-GRANULAR стал основным `/mnt/mmc/Roms/APPS/MOSS`. После запуска проверены выход 0, возврат в Garlic, восстановление governor и завершение sync. Повторной полной сверки пользовательских файлов после запуска не было.

## English

The final **15 profiles of 10–15 seconds** cover all ten two-effect combinations, four maximum-length tracks with dense note/parameter changes, master effects, a 32 MiB PCM library, capture and UI drawing. Additional profiles cover a held drone with note-free locks, mixed instruments, MONO, four eight-grain clouds and SONG. **5176 callbacks / 165 seconds**, zero processing-budget overruns, late intervals, stream underruns or scheduling overflows; maximum **30.827 ms / 32 ms**. These are bounded hardware results, not an exhaustive guarantee.

Separate ARM unit programs passed control extremes, PAIRS/RING, 8/32/44.1/96 kHz, finite output, legato, glide, release, five control/lock banks, note-free locks, MS16 and MS15 migration. An isolated A1 test measured root amplitude 0.533, rising to 0.588 at full density; the independent sub measured 0.483 at A0. SPREAD=0 is exactly mono. The tanh stage's isolated 3101 Hz antialias check reduced out-of-band harmonic-fold energy by **9.72 dB** relative to naive tanh; this is one nonlinear-stage test, not a whole-voice alias-free claim. See [antialias](antialias.json) and [dry preset measurements](audio-metrics.json).

Real button events verified +1/+10 edits, bank navigation, transport, stationary STOP state and normal exit. Both interactive USB sessions recorded one late callback interval, with no processing overrun; the cause remains unconfirmed. A separate stopped-load check did not reproduce it. These sessions remain distinct from the clean stress qualification.

Installed data was checked against the backup before boot: 70 existing files unchanged plus the new sample-free demo in slot 6. Both old installations were preserved; the freshest data now lives under the single MOSS launcher. Normal startup and exit were observed, then Garlic menu and final sync confirmed. No further device work is pending. Original user files are private and excluded from this evidence directory.

## Final profiles

Times in milliseconds. Error columns: over budget / late callback interval / stream underrun.

| Profile | Mean | Maximum | Errors |
|---|---:|---:|---|
| orbit-pond | 21.133 | 22.659 | 0 / 0 / 0 |
| orbit-crush | 18.396 | 19.626 | 0 / 0 / 0 |
| orbit-reel | 26.046 | 30.827 | 0 / 0 / 0 |
| orbit-drive | 20.867 | 21.967 | 0 / 0 / 0 |
| pond-crush | 19.330 | 20.541 | 0 / 0 / 0 |
| pond-reel | 26.869 | 30.526 | 0 / 0 / 0 |
| pond-drive | 21.731 | 24.547 | 0 / 0 / 0 |
| crush-reel | 23.757 | 27.323 | 0 / 0 / 0 |
| crush-drive | 18.763 | 19.820 | 0 / 0 / 0 |
| reel-drive | 26.021 | 29.261 | 0 / 0 / 0 |
| held-note | 25.600 | 28.405 | 0 / 0 / 0 |
| mixed | 25.634 | 28.964 | 0 / 0 / 0 |
| mono | 24.592 | 28.022 | 0 / 0 / 0 |
| granular | 25.332 | 29.756 | 0 / 0 / 0 |
| song | 19.548 | 28.023 | 0 / 0 / 0 |


## Development history and scope

The first fully oversampled four-carrier candidate failed the device budget (~40.9 ms mean with REEL/POND); it was never installed. The final design uses a half-rate, harmonically limited drone bank, full-rate sub/noise/filter and antiderivative antialiasing. The preceding full matrix is retained in `pre-mono-collapse/`; the final matrix above adds coherent mono behaviour at SPREAD=0. No clock increase, track reduction or effect-limit reduction was used.

All desktop checks passed, including render-span/reference equivalence and prior sequencer, sampler, granular, effect and interaction suites. Eight actual saved sequences from the two device installations decoded successfully without rewriting their original files. SONG uses unchanged SON1. The visual reviewer returned SHIP for the eight native fixtures; that verdict is separate from sound and timing measurements.

Audio files are reproducible with `node native/prototype/scripts/organism-render.cjs`. Four dry presets use A1 / velocity 110, no effects, no normalization; the 40-second LIVING MATTER study uses held-note locks, REEL and POND. The last seconds have an export fade. Subjective listening remains the user's decision; no comparative audition of the reference plugins is claimed.
