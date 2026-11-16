# Ambient 0.8.0-dev · verification / проверка

16 November 2026. Original RG35XX, GarlicOS 1.4.9, stock 1.008 GHz performance policy; 32000 Hz, 1024 stereo frames, **32 ms deadline**. Qualified binary SHA-256: `18bf7f75d633992bfaf52b40d2c8e638015d654c6a5e6dd562f0c8d5614f9dec`.

## Device / устройство

| Scenario / сценарий | Seconds | Mean ms | P99 upper ms | Max ms |
|---|---:|---:|---:|---:|
| Full 32-grain pool, REEL + POND / полный пул | 30 | 26.488 | 29.250 | 30.258 |
| CLOUD + FX locks / изменяющиеся локи | 20 | 26.418 | 29.250 | 30.483 |
| POND + DRIVE, synths / синты | 15 | 21.039 | 23.000 | 23.436 |
| POND + ORBIT | 15 | 19.577 | 22.500 | 23.336 |
| 16 sampler voices / голоса сэмплера | 15 | 21.795 | 24.250 | 24.916 |
| SONG | 20 | 19.843 | 26.500 | 28.326 |

RU: Все шесть профилей завершились успешно: **115 секунд, 3602 блока**. В каждом четыре дорожки, 32 MiB PCM; эффекты включены на дорожках и мастере. Ноль `over_budget`, `late_intervals`, `stream_underruns`, `schedule_overflows` и `audio_realtime_error`. Запись на SD входит в сценарии; последний открытый захват может остаться незавершённым при окончании ограниченного прогона. Самый тяжёлый блок оставил 1.517 ms запаса — это небольшой резерв, не гарантия любых будущих комбинаций или неограниченной работы. [Манифест](manifest.json), сырые отчёты рядом.

EN: All six profiles passed: **115 seconds, 3602 callbacks**. Each uses four tracks and 32 MiB PCM, with track/master effects enabled. Zero `over_budget`, `late_intervals`, `stream_underruns`, `schedule_overflows`, or `audio_realtime_error`. SD recording is exercised; the final open capture can remain unfinished when a bounded run ends. The worst callback leaves 1.517 ms headroom: a small reserve, not a guarantee for future combinations or indefinite operation. [Manifest](manifest.json); raw reports are adjacent.

RU: Отдельная интерактивная проверка использовала только синтетический проект: назначенный сэмпл, LENS/CLOUD, переключение страниц, fine/coarse жесты, POND SPACE, REEL и выход удержанием MENU. [Её отчёт](interactive-ui.json) содержит один поздний интервал и один медленный кадр в сеансе с загрузкой проекта; его происхождение отдельно не локализовано. Превышений времени обработки аудиоблока и недогрузок потока нет. Это проверка интерфейса, не седьмой успешный стресс-профиль (`profile_kind=-2`). Первые два запуска тестового интерфейса обнаружили отсутствующий источник в самой тестовой папке; путь исправлен до проверки аудио. Установленное приложение отдельно открыло пользовательский список секвенций и штатно закрылось. 67 файлов проектов/библиотеки совпали по SHA-256 до и после запуска; предыдущий бинарник сохранён. [Установка](deployment.json).

EN: The separate interactive smoke uses only a synthetic project: assigned sample, LENS/CLOUD, page navigation, fine/coarse gestures, POND SPACE, REEL, and held-MENU exit. [Its report](interactive-ui.json) contains one late interval and one slow UI frame during a session that includes project loading; the exact origin was not isolated. No callback deadline overruns or streaming underruns. This is a UI check, not a seventh passing stress profile (`profile_kind=-2`). The first two fixture launches exposed a missing source in the test directory; its path was corrected before the audio check. The installed app separately opened the user's sequence list and exited normally. SHA-256 checks confirmed all 67 project/library files were unchanged; the previous binary was retained. [Deployment](deployment.json).

## Rejected candidates / отвергнутые варианты

RU: `probes/v8` — восемь линий POND, максимум 33.423 ms и 16 превышений; v9 и v10 также не прошли. v11 — четыре линии без вложенных allpass, ноль превышений; v12 — четыре линии с вложенной диффузией, ноль превышений. В рабочую сборку вошла последняя архитектура, а не тяжёлый вариант. Пробы сохранены с хешами бинарников; это история измерений, не обещание воспроизвести промежуточный код из текущего файла.

EN: `probes/v8` is the eight-line POND: 33.423 ms maximum and 16 overruns; v9 and v10 also failed. v11 uses four lines without nested allpass stages and passes; v12 adds nested diffusion and passes. The final build uses that compact architecture. Probe binary hashes are retained as measurement history; current source does not reconstruct each intermediate revision.

## Signal and code / сигнал и код

- [Host suite](host-build.txt): sequencer, interaction, storage, instruments, locks, swing, REEL and new ambient tests.
- [UBSan](host-ubsan.txt) and [ARM unit tests](arm-unit.txt): reverse fractional rates/tiny sources, living-grain continuity, real MS13 layout/MS14 round trip, POND at 8/32/44.1/96 kHz, rapid locks and logical reset.
- [Actual legacy file](legacy-user-project.txt): current user MS13 project decoded and validated; private name omitted.
- [Synthetic signal measurements](audio-analysis.json): six impulse variants and four cloud renders. Predelay +100 ms measures exactly +100 ms; diffusion increases normalized late echo density from 0.352 to 0.821; longer DECAY increases extrapolated T20 from 2.997 to 10.074 s. These are source-dependent measurements, not a listening score. No human audition or hardware analogue-output capture is claimed.

RU: POND 0/1/2/3/4/5 — база / длинный decay / diffusion 0 / motion 80 / predelay 100 ms / damp 85. Cloud 0/1/2/3 — прямой / смешанный / обратный / смешанный с POND. Рендеры используют собственный синтетический источник. Пики не клиппуют; громкость вариантов не выравнивалась для субъективного сравнения. REEL DSP в этом обновлении не менялся.

EN: POND 0/1/2/3/4/5 means baseline / longer decay / diffusion 0 / motion 80 / predelay 100 ms / damp 85. Cloud 0/1/2/3 means forward / mixed / reverse / mixed with POND. Renders use an original synthetic source. Peaks do not clip; variants were not loudness-matched for perceptual comparison. REEL DSP is unchanged in this update.

## Reproduce / повторить

From the repository root with the configured toolchain; Python needs NumPy:

```text
node native/prototype/scripts/build.cjs
node native/prototype/scripts/ambient-render.cjs
python native/prototype/scripts/ambient-analyze.py work/ambient/audio
```

For device profiling, copy the built ARM `build/moss` to `build/reel-release/moss`, close MOSS normally, and use a **fresh** runner directory suffix:

```text
python native/prototype/scripts/reel-release-device.py v14 full-pond:21:10:30 cloud-locks:17:10:20 pond-drive:19:18:15 pond-orbit:17:3:15 sampler:18:10:15 song:20:10:20
python native/prototype/scripts/reel-release-collect.py v14 docs/verification/ambient-v08/recheck-v14
```

RU: Запускатель не меняет установленный MOSS или пользовательские проекты. Поле `checks:false` означает, что старый комплект REEL-unit не запускался этим запускателем; новые ARM-unit прошли отдельно, см. `arm-unit.txt`. На старте профиля требуется меню Garlic и свободный `command.sh`.

EN: The runner does not modify installed MOSS or user projects. `checks:false` means the historical REEL-unit bundle was not invoked by this runner; new ARM units ran separately, see `arm-unit.txt`. The runner requires the Garlic menu and a free `command.sh`.
