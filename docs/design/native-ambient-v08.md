# Native ambient v0.8 · поверхности / surfaces

RU: Состояние UI на 16 ноября 2026. Режим Operate, RG35XX, 640×480; существующие графит, bitmap-типографика, нейтральная навигация и цвета физических Y/B/A/X сохранены. Направление — [native-ambient-v08](../../design/current/native-ambient-v08.md); подробности REEL — [native-reel](native-reel.md). Этот документ описывает UI, без новых системных токенов.

EN: UI state on 16 November 2026. Operate mode, RG35XX, 640×480; existing graphite, bitmap typography, neutral navigation, and physical Y/B/A/X colors are retained. Direction: [native-ambient-v08](../../design/current/native-ambient-v08.md); REEL details: [native-reel](native-reel.md). This document describes the UI and adds no system tokens.

## GRANULAR · пять страниц / five pages

| Страница / Page | Y | B | A | X |
| --- | --- | --- | --- | --- |
| PLAY | SPEED · × | PITCH · semitones | SIZE · ms | DENSITY · grains/s |
| SCAN | POSITION · % | START · % | END · % | SCATTER · % |
| TEXTURE | STEREO · % | JUMP · % | CUTOFF · Hz | LEVEL · % |
| ENVELOPE | GRAIN IN · % | GRAIN OUT · % | NOTE IN · ms | NOTE OUT · ms |
| CLOUD | REVERSE · % | DETUNE · cents | SIZE VAR · % | JITTER · % |

RU: CLOUD — пятая страница, параметры 16–19 при нумерации с нуля. Все пять вкладок видны под LENS; текущая подчёркнута. L1/R1 меняют страницу, удержание Y/B/A/X со стрелками влево/вправо даёт точное изменение, вверх/вниз — крупное. Числовые VOICE LOCKS используют те же страницы и параметры.

EN: CLOUD is the fifth page, parameters 16–19 with zero-based indexing. All five tabs remain visible below LENS, with an underline on the current tab. L1/R1 changes pages; holding Y/B/A/X with left/right gives a fine edit and up/down a coarse edit. Numeric VOICE LOCKS use the same pages and parameters.

RU: REVERSE задаёт вероятность обратного чтения отдельного зерна независимо от направления сканирования. По контракту изменение относится к будущим зёрнам и сохраняет направление уже звучащих. В LENS дуга обратного зерна показывает его исходный интервал, маркер и короткий штрих направления следуют снимку воспроизведения. Счётчик `F / R` показывает наблюдаемое число прямых и обратных зёрен; это не дополнительный параметр.

EN: REVERSE sets the probability of reverse playback for each grain, independently of scan direction. The contract applies changes to future grains while preserving the direction of living grains. In LENS, a reverse grain's arc represents its source interval; its marker and short direction stroke follow the playback snapshot. `F / R` reports the observed forward/reverse grain count; it is not another control.

RU: Внешняя волна остаётся в абсолютных координатах сэмпла; START/END приглушают исключённую часть, границы остаются видимыми. SOURCE, диапазон, GRAINS / 8, HEAD и скорость генерации помогают читать линзу. X FREEZE меняется на X FROZEN, HEAD — на HEAD HELD. Без доступного сэмпла вместо линзы показаны NO SAMPLE и A ASSIGN SAMPLE; страницы и поля остаются доступны.

EN: The outer waveform stays in absolute sample coordinates; START/END dims the excluded portion while preserving visible boundaries. SOURCE, range, GRAINS / 8, HEAD, and grain rate explain the lens. X FREEZE changes to X FROZEN, and HEAD to HEAD HELD. Without an available sample, NO SAMPLE and A ASSIGN SAMPLE replace the lens; pages and fields remain available.

## POND · TAIL / SPACE

| Страница / Page | Y | B | A | X |
| --- | --- | --- | --- | --- |
| TAIL | SIZE · % | DECAY · sec | DAMP · % | MIX · % |
| SPACE | PREDELAY · ms | DIFFUSE · % | MOTION · % | LOW CUT · Hz |

RU: Обе вкладки стоят над графиком, выбранная подчёркнута. Четыре нижних поля и управление страницами сохраняют общую схему FX. DECAY отображается в секундах с одним десятичным знаком: это номинальное время низкочастотного спада; затухание верхних частот может быть короче. Числовые FX LOCKS сохраняют единицы; наследование `--` и сброс `RESET` подписаны STEP LOCK.

EN: Both tabs sit above the graph, with the selected tab underlined. The four lower fields and page controls retain the shared FX pattern. DECAY displays seconds to one decimal place: a nominal low-frequency decay time; high-frequency decay can be shorter. Numeric FX LOCKS retain their units; inherited `--` and reset `RESET` values use STEP LOCK.

RU: График — **нормированная статичная огибающая**, явно подписанная DESIGN ENVELOPE. Её форма фиксирована; значения DECAY и PREDELAY отражаются в строке `TAIL … S / PRE … MS`. График не показывает измеренный хвост, не отслеживает живой звук и не изображает изменение параметров формой кривой. Заголовок ON / BYPASS, маршрут TRACK / MASTER, включение, порядок цепочки и возврат используют существующий интерфейс FX.

EN: The plot is a **normalized static envelope**, explicitly labelled DESIGN ENVELOPE. Its shape is fixed; DECAY and PREDELAY appear in the `TAIL … S / PRE … MS` line. It does not show a measured tail, track live audio, or change its curve shape with parameters. The ON / BYPASS heading, TRACK / MASTER routing, enable action, chain order, and return use the existing FX interface.

## REEL · кратко / summary

RU: Две крупные плоские катушки, широкие трёхлопастные ступицы, провисающая лента с центральным квадратом и одна строка скорости/состояния заменяют прежние отдельные графики. COLOR / MOTION / LIFE / OUTPUT стоят под катушками над четырьмя полями. STOP и BYPASS названы явно; BYPASS приглушает графику. Полный контракт поверхности — [native-reel](native-reel.md).

EN: Two large flat reels, broad three-blade hubs, suspended tape with a central square, and one speed/status line replace the previous separate charts. COLOR / MOTION / LIFE / OUTPUT sit below the reels and above the four fields. STOP and BYPASS are explicit; BYPASS dims the graphic. Full surface contract: [native-reel](native-reel.md).

## Свидетельства и границы / evidence and limits

- UI sources: `native/prototype/src/app_reel_ui.h`, `app_granular_ui.h`, `app_tools_ui.h`, `app_ui.h`, `granular_defs.h`, `effect_defs.h`.
- Captures: `.impeccable/review/ambient-v08/` — `reel-0.png`, `reel-1-stop.png`, `reel-2-bypass.png`, `graincloud.png`, `grainrange.png`, `grainempty.png`, `pond-0.png`, `pond-1.png`.

RU: Финальный визуальный review — SHIP; существенных дефектов компоновки в этих кадрах не найдено. Проверка статическая: подписи, геометрия и соответствие исходникам. Движение, звук, производительность и аппаратное поведение этим review не проверены. Корневой `DESIGN.md`, sidecar и несвязанные исторические правила сохранены.

EN: The final visual review returned SHIP, with no material layout defects in these captures. Verification is static: labels, geometry, and correspondence with source. This review does not validate motion, audio, performance, or hardware behavior. Root `DESIGN.md`, the sidecar, and unrelated historical rules are preserved.
