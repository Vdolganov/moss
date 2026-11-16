# Native REEL · поверхность и свидетельства / surface and evidence

## Область / Scope

RU: Актуальная поверхность от 16 ноября 2026 следует [ambient-контракту](../../design/current/native-ambient-v08.md). REEL продолжает систему MOSS: плоский графит, экран 640×480, bitmap-глифы 5×7, нейтральные навигация и фокус. Новых токенов нет. Общий обзор GRANULAR / POND — [native-ambient-v08](native-ambient-v08.md).

EN: The current surface, dated 16 November 2026, follows the [ambient contract](../../design/current/native-ambient-v08.md). REEL continues the MOSS system: flat graphite, a 640×480 canvas, 5×7 bitmap glyphs, and neutral navigation and focus. There are no new tokens. GRANULAR / POND are covered in [native-ambient-v08](native-ambient-v08.md).

## Четыре страницы / Four pages

| Страница / Page | Y | B | A | X |
| --- | --- | --- | --- | --- |
| COLOR | DRIVE · % | TONE · dB | BUMP · dB | AMOUNT · % |
| MOTION | WOW · cents | WOW RATE · Hz | FLUTTER · cents | FLT RATE · Hz |
| LIFE | DRIFT · % | STEREO · % | LOSS · % | DROP · % |
| OUTPUT | HISS · dBFS | HISS TONE · % | FOLLOW · % | OUTPUT · dB |

RU: В деталях эффекта L1/R1 выбирают страницу. Все четыре вкладки стоят **под катушками**, сразу над полями Y/B/A/X; текущая подчёркнута. Нижние поля сохраняют прежнюю геометрию, цвета физических кнопок, число и единицу. В FX LOCKS L1+влево/вправо выбирают эффект, L1+вверх/вниз — его страницу; заголовок показывает REEL / страницу и её номер. Числовые локи сохраняют физические единицы; `--` и `RESET` показывают STEP LOCK. L1+R1 открывает контекст с пресетами и отменой.

EN: In effect detail, L1/R1 selects the page. All four tabs sit **below the reels**, directly above the Y/B/A/X fields; the current tab is underlined. The lower fields retain their geometry, physical button colors, value, and unit. Within FX LOCKS, L1+left/right selects the effect and L1+up/down selects its page; the heading identifies REEL, the page, and its number. Numeric locks retain physical units; `--` and `RESET` use STEP LOCK. L1+R1 opens the context containing presets and undo.

## Катушки и состояние / Reels and state

RU: Главная графика — две крупные контурные катушки с широкими трёхлопастными ступицами, неглубокая провисающая дуга ленты и маленький квадрат в центре. Композиция приближена к референсу GrainBrick из текущего контракта. Отдельные история амплитуды, график SPEED / CENTS и подпись OUTPUT / X4 удалены. Над катушками остаётся одна строка `RUN / SPEED … TO … CENTS` с минимумом и максимумом скорости из снимка движка.

EN: The main graphic consists of two large reel outlines with broad three-blade hubs, a shallow suspended tape arc, and a small central square. The composition follows the GrainBrick reference recorded in the current contract. The separate amplitude history, SPEED / CENTS chart, and OUTPUT / X4 label are removed. One line above the reels, `RUN / SPEED … TO … CENTS`, retains the speed minimum and maximum from the engine snapshot.

RU: Поворот катушек следует фазе снимка, а небольшое отклонение дуги ограничено измеренной огибающей; случайного движения нет. Строка состояния явно меняется на STOP или BYPASS, заголовок — REEL / ON либо REEL / BYPASS. По контракту STOP и BYPASS останавливают фазу; BYPASS также приглушает графику. Статичные кадры подтверждают компоновку и подписи, но не ход анимации.

EN: Reel rotation follows the snapshot phase, while the measured envelope bounds the small tape displacement; there is no random motion. The status line explicitly changes to STOP or BYPASS, and the heading reads REEL / ON or REEL / BYPASS. The contract freezes phase on STOP and BYPASS; BYPASS also dims the graphic. Still captures establish layout and labels, not animation behavior.

## Проверенные источники / Checked evidence

- System: `PRODUCT.md`, `DESIGN.md`, `.impeccable/design.json`.
- Current UI: `native/prototype/src/app_reel_ui.h`, `app_tools_ui.h`, `app_ui.h`, `effect_defs.h`.
- Current captures: `.impeccable/review/ambient-v08/reel-0.png`, `reel-1-stop.png`, `reel-2-bypass.png`.
- Historical only: `.impeccable/review/native-reel/` contains the superseded waveform/speed-chart composition; `.impeccable/review/native-v051/reel.png` contains the earlier SPEED MODEL.

RU: Финальный визуальный review — SHIP, без существенных дефектов компоновки в просмотренных кадрах. Проверка статическая; она не подтверждает звук, производительность или работу на устройстве. `DESIGN.md`, sidecar и исторические правила системы не изменены.

EN: The final visual review returned SHIP, with no material layout defects in the inspected captures. Verification is static; it does not establish audio quality, performance, or device behavior. `DESIGN.md`, the sidecar, and historical system rules are unchanged.
