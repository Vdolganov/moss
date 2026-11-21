# MOSS Interface Lab · 0.12

Браузерный прототип текущего интерфейса MOSS для обсуждения и изменения дизайна. Работают состояния, редактирование, библиотека, файлы, клавиатурное управление и модель транспорта. **Аудиодвижков и звука нет.** Предпросмотр, ресэмплинг и портреты звука — визуальная имитация; они не измеряют DSP. Импортированный WAV читается для отображения настоящей формы волны, включая отдельные отсчёты в FINE.

## Запуск

Открой `START.cmd` или выполни `npm start` в этой папке. Адрес: **http://127.0.0.1:4174**. Для работы нужен Node.js, установка пакетов для самого приложения не нужна. Сервер доступен только на этом компьютере. Предыдущий прототип в родительской папке сохранён отдельно.

Кликни по экрану для управления клавиатурой. На странице есть раскрываемая инструкция и экранные кнопки консоли. Кнопка «Сценарии и файлы» открывает быстрые переходы к FM, грануляру, сэмплеру, нарезке и SONG. Это обычные редактируемые данные, а не записанные анимации.

| Клавиатура | Anbernic / действие |
|---|---|
| Стрелки | D-pad, навигация |
| Z / X / C / V | Y / B / A / X, четыре параметра слева направо |
| Удержание Z/X/C/V + ←/→ | Изменение на единицу |
| Удержание Z/X/C/V + ↓/↑ | Крупное изменение; удержание стрелки ускоряет |
| Q / E | L1 / R1, видимые вкладки и банки; строки не переключаются |
| [ / ] | L2 / R2, верхние вкладки |
| Q + E | Контекстное меню |
| Кнопка поля + Escape | Меню одного лока: SET / CLEAR / RESET |
| E + ←/→ | Страницы грида и списков |
| Q + ←/→ | Слот сэмплера / участок TAPE |
| Q + ↑/↓ | Страница параметров выбранного таба |
| Space | START, пуск/стоп; в клавиатуре имени — сохранить |
| Tab | SELECT, библиотека секвенций |
| Tab + стрелки | BPM секвенции или SONG |
| Escape | Назад; удержание 2 секунды — сохранить и выйти в библиотеку |
| Shift + Tab | Выйти из управления эмулятором в обычную страницу |

В PATTERN порядок вкладок: **STEP / V-LOCK / ORBIT LOCKS / REEL LOCKS / TIME**, если включены ORBIT и REEL. Каждый включённый эффект получает отдельную вкладку в порядке цепочки. Общего FX-LOCK больше нет. Выключение скрывает вкладку, сохраняя локи по идентификатору эффекта; повторное включение возвращает доступ к ним. Если выбранный эффект выключен, открывается STEP. Без эффектов остаются STEP / V-LOCK / TIME. Локи эффектов относятся ко всей дорожке — в контексте указано TRACK.

**Q / E (L1 / R1)** переключают табы. **Q + ←/→** во всех табах паттерна и VOICE выбирает слот сэмплера, в TAPE — участок. **Q + ↑/↓** выбирает страницу параметров текущего таба: вверх предыдущая, вниз следующая, по кругу. Например, REEL LOCKS имеет четыре страницы, ORBIT LOCKS — две. В VOICE синта, редакторе эффекта и TAPE MOVE/TRIM/FINE эта же вертикальная комбинация переключает видимые банки. Обычные стрелки в VOICE синта меняют ноту предпросмотра.

Вложенные страницы обозначены стрелками; активный таб добавляет **номер / всего** и две кликабельные стрелки. Слева от параметров — шкала страниц. Одностраничные табы обходятся без индикатора. В TAPE горизонтальный счётчик PART / SLICE показывает выбранный участок, а FINE центрируется при его смене. Зажатая Q показывает контекстные подсказки в служебной строке; удержание от 350 мс не переключает таб при отпускании. **Q + E** открывает меню.

Слоты сэмплера монохромные, со скруглением 12 px: выбранный слот светлый, остальные тёмные. Нота отдельно инвертируется во время звучания, включая одновременно звучащие слоты. Длинные имена сокращаются многоточием. Проверки: `npm run test:nested`.

В TAPE участки нарезки монохромные. Выбранный участок имеет более светлую подложку, яркую волну и границы START/END со скобками. Над ним — округлая метка с пиктограммой кнопок и буквой; она остаётся читаемой даже у очень короткого участка. Остальные части обозначены буквами, ручные слайсы — номерами. Предпросмотр показывает отдельная линия с треугольной головкой. В FINE стрелки у краёв означают границы вне видимой области. Сохранение и назначение разделены: SAVE KIT — библиотека, SAVE SELECTED + ASSIGN — исходный слот, SAVE KIT + ASSIGN FOUR — все четыре слота с подтверждением замены. Проверки: `npm run test:tape`.


На экране можно нажимать строки, вкладки и кнопки. Клик по параметру выделяет его для изменения стрелками; колесо мыши тоже меняет его, Shift + колесо — крупнее. Эти возможности относятся только к браузерной оболочке. Клик по шагу выбирает его, повторный клик переключает ноту. Названия вводятся через общую панель символов или явный режим TYPE ON KEYBOARD (Enter сохранить, Escape вернуть панель).

## Состояния и ограничения

Отступы во всём прототипе и браузерной оболочке используют шкалу **4 / 8 / 12 / 16 / 24 / 32 px**. Боковые поля — 16 px; параметры закреплены в 24 px над служебной строкой; вкладки — в 16 px над параметрами. Между вкладками и рисунком инструмента остаётся не менее 16 px. Микшер показывает все четыре дорожки. Основные шрифты — 21 / 14 / 12 px; подписи в овальных плашках на линиях длительности используют отдельный размер 8 px.

В режиме 16 пустой шаг показывает только номер, нота — серую заливку и название, лок без ноты — `LOCK` и ромб. Ромб у ноты обозначает её локи. Светлая рамка — курсор редактирования; инвертированный номер — текущая позиция воспроизведения. Длительность читается в NOTE LEN и по линии вдоль нижнего края ячеек. Точка обозначает запуск, засечка — конец, стрелки — продолжение через строку или страницу. На входе в новую страницу название ноты находится прямо на линии — в овальной плашке с тёмным текстом 8 px; ромб лока сохраняется. Линия толщиной 2 px поднята на 12 px от нижнего края ячейки. В меню продолжения есть GO TO NOTE.

Чтобы сравнить страницы, открой **«Сценарии и файлы» → «GRID · 64 шага» / «GRID · 128 шагов» / «GRID · 512 шагов»**. Каждый пример — отдельная редактируемая секвенция. Переключатель **16 / 64** под гридом задаёт число шагов на странице; **по умолчанию включено 64**. С клавиатуры: **Q + E → GRID VIEW → C** (L1 + R1 → A). Выбранный шаг сохраняется, число страниц пересчитывается: для 512 шагов это 32 или 8 страниц. ↑/↓ перемещают на соседнюю видимую строку, **E + ←/→ — на страницу выбранного размера**. Рядом есть стрелки для мыши. Режим всего паттерна убран. Музыкальные данные и деление не меняются. Проверки: `npm run test:pages`.

Для проверки скрытых вкладок нажми **«Тест переполнения»** в панели над экраном. Режим сужает все навигационные панели, не меняя страницы или данные проекта; он работает и в библиотеке, MIX и TAPE. `[ / ]` переключают верхние разделы, `Q / E` — страницы внутри раздела. В настройках мастер-эффекта Q/E управляют его банками; B возвращает к цепочке эффектов. Повторное нажатие возвращает обычную ширину. При перезагрузке страницы тест выключен.

- ATLAS → SEQUENCES / SONGS. В секвенции GRID / MIX / TAPE; у дорожки PATTERN / VOICE / SPACE.
- Четыре дорожки, пять инструментов: SAMPLER, MONO, ORGANISM, FM, GRANULAR. У каждого синта и грануляра пять банков по четыре параметра. Названия, диапазоны, режимы и пресеты извлечены из текущего `native/prototype/src/model.h`.
- Паттерн хранит до 512 единиц 1/64. Деления 1/8, 1/16, 1/32, 1/64 меняют количество видимых ячеек с сохранением длительности. Произвольный цикл каждой дорожки, страницы по 16 или до 64 шагов, SWING и SHIFT.
- Ноты и локи без ноты, три состояния лока: наследование, значение, RESET. Локи инструментов и эффектов, REPEAT, длина и вероятность шага. Четыре независимые линии сэмплера.
- ORBIT, POND, CRUSH, REEL, DRIVE; весь набор параметров, порядок цепочки и максимум два включённых эффекта на дорожке и мастере.
- TAPE: импорт WAV и папок, целый источник, ручная нарезка, четыре последовательных участка, MOVE / TRIM / FINE, именованные папки, загрузка четырёх слотов. Подготовленный участок ограничен 1 MiB PCM16. Исходник может быть длиннее.
- SONG: до 64 слотов, ссылки на секвенции, повторы и такты, общий BPM, перестановка, копирование источника в независимую секвенцию, loop. По умолчанию длительность слота округляется по самой длинной дорожке до целого такта.
- 32 секвенции, 32 SONG, 256 подготовленных сэмплов; проверка 32 MiB набора сэмплов при пуске SONG. Имена до 24 символов. Отмена последнего изменения, подтверждение удаления, защита используемых сэмплов и секвенций.

Демонстрационные сэмплы — явно виртуальные данные, их волны иллюстративны. При добавлении WAV показывается его реальная волна. В результате беззвучного захвата создаётся виртуальный источник; экспорт аудиофайлов отсутствует намеренно.

Чтобы проверить переходы длинных нот между страницами, открой **«Сценарии и файлы» → «Длинные ноты»** и выбери **16** под гридом. В LONG NOTES две дорожки по 32 шага: MONO с нотой на 12 шагов и сэмплер с двумя длительными голосами. Нажми Space, затем E + → для второй страницы. Переключай слоты сэмплера кликом или Q + ←/→: страница и курсор сохранятся. Подсветка названия ноты в верхнем слоте показывает его отдельное воспроизведение. Сценарий беззвучный, его источники виртуальные; данные можно редактировать как обычную секвенцию.

Линии показывают запланированную длительность с учётом следующего запуска того же голоса; для сэмплера также учитываются файл, подрезка, питч и LENGTH. Подсветка слотов следует реально принятым событиям модели, включая вероятность. Релиз и хвосты эффектов не рисуются. Если исходная нота скрыта внутри более крупного шага, GO TO NOTE приводит к содержащей её ячейке и предлагает более мелкое деление для точного редактирования.

## Сохранение

Отдельная база IndexedDB `moss-interface-012`, автосохранение после редактирования. Рабочие файлы MOSS и данные консоли не изменяются. Экспорт/открытие JSON переносит структуру и формы волн дизайн-прототипа, **не нативный формат .moss**. Исходные WAV сохраняются локально в браузере; JSON их не включает. После переноса JSON в другой браузер остаются сохранённые формы волн, но отдельные отсчёты недоступны. Повторный импорт WAV создаёт новый исходник с детализацией; ранее перенесённые срезы автоматически с ним не связываются.

При ошибке сохранения статус прямо сообщает об этом; можно выгрузить JSON. Импорт проекта проверяется до замены библиотеки. Удаление данных браузера удаляет и локальные исходники; сохраняй экспорт отдельно.

## Разработка и проверка

Стандартные HTML/CSS и ES-модули, без сборщика и UI-фреймворка. `src/model.js` — данные и транспорт; `input.js` — жесты и удержания; `fields.js` — общий контракт параметров; `views.js` — экраны; `visuals.js` — портреты/волны; `menus.js` — контекстные действия; `storage.js` — IndexedDB и чтение WAV; `app.js` — сценарии. Шкала отступов, шрифты и цвета заданы в `style.css`; `src/layout.js` связывает общую геометрию экранов и визуализаций. Нативный bitmap-шрифт перенесён из собственных глифов MOSS.

`npm ci` устанавливает только инструменты проверки. `npm test` проверяет модель, `npm run test:browser` — сценарии через клавиатуру в Edge, `npm run test:regression` — исправленные сценарии и единицы измерения. Можно указать путь к другому Chromium-браузеру через `MOSS_BROWSER`. Для браузерных проверок сервер должен быть запущен. `npm run test:tabs` проверяет навигацию и переполнение, `npm run test:spacing` — общие отступы, нижнюю панель, меню, анимации и размеры оболочки. `npm run test:grid` проверяет состояния ячеек и позицию воспроизведения; `npm run test:notes` и `npm run test:duration` — длительности и независимые голоса сэмплера. `npm run capture` обновляет снимки экранов. Матрица соответствия — `PARITY.md`.

Проверка 19 ноября 2026: 16 проверок модели, 17 браузерных сценариев и 7 регрессионных проверок прошли; браузерные отчёты не содержат ошибок выполнения. Независимый финальный просмотр подтвердил исправление шести замечаний по 23 снимкам. Его вердикт относится к этим исправлениям, а не ко всему приложению или нативной версии. Подробности: `../../.impeccable/review/interface-lab-v012/documentation.md`.

## English

Open **«Сценарии и файлы» → «Длинные ноты» (Long notes)** to create/reopen the editable LONG NOTES study. It contains two 32-step tracks: a 12-step MONO note beginning at step 13, and two overlapping sampler voices. Choose 16 below the grid, press Space, then E+Right to visit page two. Slot selection preserves page/cursor; independent inverted slot badges show active voices. The study is silent and uses virtual sources. GO TO NOTE returns to a continuation's source cell; hidden off-grid onsets show a finer-resolution hint.

Planned lines assume written triggers fire, excluding release and FX tails. Sampler durations account for source length, crop, pitch, gate and sample locks. Live badges follow accepted events, probability, tempo, one-shot completion and retriggers. Run `npm run test:notes` and `npm run test:duration` for targeted timing and browser checks.

The device and browser workbench share a **4 / 8 / 12 / 16 / 24 / 32px** spacing scale. Content insets are 16px; parameter fields are anchored 24px above the service footer, with page tabs 16px above them. Portraits end at least 16px before the tabs. The mixer shows all four tracks. Primary type remains 21 / 14 / 12px, with a scoped 8px role for duration-line capsules. Run `npm run test:spacing` for layout, dialog, animation-bound and responsive-shell checks.

Grid cells use 4px corners: empty steps show only a number; notes have a gray fill and note/HIT label; lock-only steps show LOCK and an outlined diamond. Notes with locks also have a diamond. The editing cursor uses a single light border; the playing step inverts its number. LENGTH remains in the parameter area; connected duration lines also show trigger dots, end caps and row/page continuation arrows. The first continuation on a page has an inverted 8px note label in an oval capsule centered on the 2px line, retaining its lock diamond. The line sits 12px above the cell bottom. Run `npm run test:grid` for state combinations, sampler lanes, playback boundaries and editing checks.

Use **«Тест переполнения» (Overflow test)** above the device to constrain all navigation strips, including the library, MIX and TAPE. Brackets switch main sections; Q/E switch the current section's pages. Inside a master effect, Q/E control its parameter banks and B returns to the effect chain. The toggle only changes the available navigation width; project data is untouched. Toggle it again to restore the normal layout. The test starts disabled after a reload.

An interactive, **silent** browser reference for the current MOSS 0.12 interface. Run `START.cmd` or `npm start`, then open http://127.0.0.1:4174. No runtime dependencies are required beyond Node.js. The previous visual prototype is preserved.

Open **Сценарии и файлы → GRID · 64 / 128 / 512** for the editable pagination studies. **16 / 64** below the grid sets steps per page; **64 is the default** on each browser session. Keyboard: **Q+E → GRID VIEW → C** (L1+R1 → A). Switching preserves the cursor and musical data; a 512-step track has 32 or eight pages respectively. Up/down follows visible rows; **E+left/right jumps by the chosen page size**. Visible arrows also turn pages. The whole-pattern ALL mode has been removed. Run `npm run test:pages` for interaction and layout checks.

Physical Y/B/A/X map to Z/X/C/V. Hold a face key and use left/right for fine edits, up/down for coarse edits. Q/E select parameter banks; brackets select major tabs; Q+E opens options; E+left/right jumps pages; Space starts/stops the visual transport. Tab opens the sequence library, Tab+arrows changes tempo, and Shift+Tab exits the emulator's keyboard focus. Names use the on-screen character panel.

PATTERN uses **STEP / V-LOCK / ORBIT LOCKS / REEL LOCKS / TIME** when ORBIT and REEL are enabled. Each enabled track effect has its own tab, in chain order. There is no generic FX-LOCK tab. Bypassing hides its tab but retains stored locks by effect identity; re-enabling exposes them again. If the selected effect is bypassed, selection falls back to STEP. With no effects, only STEP / V-LOCK / TIME remain. Effect locks apply to the whole track, indicated by TRACK in the context line.

**Q/E (L1/R1)** select tabs. **Q+left/right** selects the sampler slot in every pattern tab and sampler VOICE, or the part/slice in TAPE. **Q+up/down** selects the current tab's parameter page, wrapping within that tab. ORBIT LOCKS has two pages; REEL LOCKS has four. The same vertical chord also changes visible banks in synth VOICE, FX editors and TAPE MOVE/TRIM/FINE. Unmodified synth VOICE arrows still change the audition note.

Hidden page stacks have inactive chevrons, an active current/total counter with clickable arrows, and a neutral rail beside the parameters. Single-page tabs omit them. TAPE parts use a separate horizontal PART/SLICE selector, not a vertical page count. FINE recenters on part changes. Held Q reveals footer hints; a hold of at least 350ms releases without changing tabs. Q+E still opens options.

Sampler slots are monochrome with 12px corners: light selected slot, dark idle slots, and independently inverted note badges for playback. Long names use ellipsis. `npm run test:nested` covers these interactions and the overflow behavior.

TAPE regions are monochrome: brighter selected waveform/background, inward START/END brackets, and a rounded physical-button label above the region. The selected label remains readable for subpixel cuts. Other parts use letters; manual slices use numbers. Preview uses an independent line with a triangular head. In FINE, edge chevrons indicate offscreen boundaries. Editing, preview and saving behavior remain unchanged. Run `npm run test:tape` for targeted coverage.


All five instruments and effects use the current native parameter definitions. Sequence and SONG management, per-track timing, locks without note retrigger, sample folders, four-part slicing, fine frame editing, assignment, FX chains and persistent state are interactive. Audio previews, capture and instrument telemetry are simulations. Imported WAV data is read for waveform display only; no AudioContext, DSP, rendering or audio export is used.

Data lives in the separate IndexedDB database `moss-interface-012`. JSON export is an interface-prototype format, not a device project; it includes waveform metadata but not original WAV bytes. After transfer to another browser, importing a WAV creates a new source with frame detail; it does not reconnect existing cuts automatically. Device files are untouched.

Run `npm ci` for test tools, `npm test` for model checks, `npm run test:browser` for browser scenarios, and `npm run test:regression` for the correction checks. Browser checks require the local server and Edge, or a Chromium executable supplied through `MOSS_BROWSER`. `npm run capture` refreshes the screenshots. On 19 November 2026, all 16 model, 17 browser and 7 regression checks passed. The independent review confirmed the six scored corrections using 23 valid captures; its ship verdict covers those fixes only. See `PARITY.md` for adaptation boundaries before transferring design changes to the native application.

## Надёжность и единое управление · 20.11.2026

Все пункты U01–U13 из аудита внедрены в этот прототип; [перечень и границы проверки](../../design/current/ux-fixes-2026-11-20.md).

- Одно удержание отменяется целиком через UNDO LAST EDIT. Навигация и отказ не стирают снимок. SAVE и выход сообщают успех после завершения записи; при отказе доступны повтор и экспорт.
- Черновик TAPE восстанавливается при повторном открытии источника, в том числе после reload. Он не добавляет готовые сэмплы в библиотеку. SAVE KIT сохраняет только комплект; отдельные команды назначают один или четыре слота.
- CLEAR SELECTED HIT затрагивает выбранный слот. Общие локи и общая очистка шага имеют отдельные команды. У одного лока SET создаёт событие, CLEAR удаляет его, RESET TO BASE задаёт возврат к базе на этом шаге. -- / ◆ / ◇ RST различают эти состояния; единицы сохранены.
- В TIME: CYCLE — число клеток, DIV — длительность клетки; NOTE LEN — длительность ноты. 16/64 и PAGE меняют только просмотр. Кнопка поля показывает внизу размер точного и крупного изменения.
- B/MENU возвращает на один уровень; исключение B в четырёхчастной нарезке сохранено: это preview части. MENU всегда остаётся возвратом. Позиция дорожки и страницы эффекта запоминаются.
- Масштаб: 1×, 1.5×, по ширине. Узкое окно при 1× прокручивает экран, сохраняя читаемый шрифт. Это не мобильная версия.

Проверка новых сценариев: `npm run test:ux`. Действия проверяются в отдельном браузерном профиле. Нативные файлы и устройство не обновляются.

### English

This browser update implements audit U01–U13: reliable undo/save, persistent slicing drafts, explicit kit versus single-slot assignment, scoped clearing, individual lock SET/CLEAR/RESET, consistent navigation, remembered positions, complete recovery menus and truthful footer actions. It also adds physical lock units, clearer empty states, fine/coarse hints, explicit desktop name typing and readable 1× narrow-window scrolling. Run `npm run test:ux`. It remains a silent UI prototype, separate from the native app.

## LIVE: три варианта исполнения

[Открыть LIVE](live.html) после запуска сервера. Пульт / Партитура / Петля используют один эскиз с записью живого тайминга, длительностью, отменой и квантизацией. Второй вариант открывается по умолчанию: пользователь предпочёл его, затем попросил цельную крестовину и круглые кнопки в расположении RG35XX. Выбор для нативной реализации ещё обсуждается.

[Русское руководство](../../docs/LIVE-STUDY.md) · [English guide](../../docs/LIVE-STUDY.en.md). Это отдельный дизайн-эксперимент без аудиодвижка; он не меняет нативную версию и существующие проекты браузерного прототипа.
