# Native REEL · поверхность и свидетельства / surface and evidence

## Область / Scope

RU: REEL продолжает существующую систему MOSS: плоский графит, логический экран 640×480, bitmap-глифы 5×7, нейтральные навигация и фокус. Это расширение поверхности, без новых токенов. `DESIGN.md` и `.impeccable/design.json` сохранены. Исторический REEL с подписью SPEED MODEL относится к прежней реализации.

EN: REEL extends the existing MOSS system: flat graphite, a logical 640×480 canvas, 5×7 bitmap glyphs, and neutral navigation and focus. This is a surface extension with no new tokens. `DESIGN.md` and `.impeccable/design.json` are preserved. The historical REEL labelled SPEED MODEL describes the earlier implementation.

## Четыре страницы / Four pages

| Страница / Page | Y | B | A | X |
| --- | --- | --- | --- | --- |
| COLOR | DRIVE · % | TONE · dB | BUMP · dB | AMOUNT · % |
| MOTION | WOW · cents | WOW RATE · Hz | FLUTTER · cents | FLT RATE · Hz |
| LIFE | DRIFT · % | STEREO · % | LOSS · % | DROP · % |
| OUTPUT | HISS · dBFS | HISS TONE · % | FOLLOW · % | OUTPUT · dB |

RU: В деталях эффекта L1/R1 выбирают страницу; все четыре названия видны, текущая страница подчёркнута. Четыре нижних поля используют прежнюю геометрию и цвета физических Y/B/A/X. Значение и единица остаются видны. В FX LOCKS вложенные L1+влево/вправо выбирают эффект, L1+вверх/вниз — его страницу; заголовок показывает REEL / страницу и её номер. Числовые локи сохраняют физические единицы; `--` и `RESET` показывают STEP LOCK. L1+R1 открывает контекст с пресетами и отменой; список пресетов использует обычную нейтральную рамку выбора.

EN: In effect detail, L1/R1 selects the page; all four names remain visible and the current page is underlined. The four lower fields retain the existing geometry and physical Y/B/A/X colors. Values and units remain visible. Within FX LOCKS, L1+left/right selects the effect and L1+up/down selects its page; the heading identifies REEL, the page, and its number. Numeric locks retain physical units; `--` and `RESET` use STEP LOCK. L1+R1 opens the context containing presets and undo; the preset list uses the ordinary neutral selection outline.

## Измеренная графика / Measured display

RU: Две катушки и путь ленты размещены в существующей панели. Полоски вдоль ленты показывают историю пиков фактического выхода REEL, с отдельным приглушённым вкладом шума и явной подписью OUTPUT / X4. Это история амплитуды, а не осциллограмма отдельных отсчётов. Фаза катушек получает медленную составляющую реального смещения головки; быстрая модуляция не изображается искусственно медленным вращением. SPEED / CENTS показывает измеренные минимумы и максимумы аудиоблоков в истории и текущую числовую пару. DROP показывает текущее ослабление. Снимок передаётся из аудиопотока; UI не читает живые фильтры или звуковой буфер.

EN: Two reels and the tape path occupy the existing panel. Marks along the tape show the history of actual REEL output peaks, with a separate dim noise contribution and the explicit OUTPUT / X4 label. This is amplitude history, not a sample-by-sample waveform. Reel phase follows the slow component of actual read-head displacement; fast modulation is not depicted as artificially slow rotation. SPEED / CENTS shows measured audio-block minima and maxima as history and a current numeric pair. DROP shows current attenuation. The audio thread publishes a snapshot; the UI does not read live filters or the audio buffer.

RU: TAPE RUN / TAPE STOP / TAPE BYPASS и REEL / ON либо BYPASS явно называют состояние. STOP и BYPASS не продвигают фазу катушек; BYPASS приглушает графику. Один неподвижный кадр подтверждает компоновку и подписи, но не доказывает ход анимации или производительность.

EN: TAPE RUN / TAPE STOP / TAPE BYPASS and REEL / ON or BYPASS explicitly identify state. STOP and BYPASS do not advance reel phase; BYPASS dims the graphics. A still capture establishes layout and labels, but does not prove animation behavior or performance.

## Проверенные источники / Checked evidence

- System: `PRODUCT.md`, `DESIGN.md`, `.impeccable/design.json`.
- Rendering and interaction: `native/prototype/src/app_reel_ui.h`, `app_tools_ui.h`, `app_ui.h`, `parameter_ui.h`, `effect_defs.h`, and `hardware lab/src/lab_ui.h`.
- Measured snapshot and phase: `native/prototype/src/reel_dsp.h`.
- Current captures: `.impeccable/review/native-reel/reel-color.png`, `reel-motion.png`, `reel-life.png`, `reel-output.png`, `reel-stop.png`, `reel-bypass.png`, `reel-locks.png`, `reel-motion-locks.png`, `reel-presets.png`.
- Incumbent comparison: `.impeccable/review/native-v051/reel.png`.

RU: Проверка документации не меняет старые разделы системы или её sidecar и не является новой проверкой сборки, звука либо устройства.

EN: This documentation pass does not revise historical system sections or the sidecar, and is not a new build, audio, or device validation.
