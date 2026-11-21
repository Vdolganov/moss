# Native sample folders 0.11 · Operate

Slicer refinement: [0.11.1](native-slicer-v0111.md) supersedes the MOVE/TRIM navigation, range presentation, initial layout and audition/master-capture levels below. / Актуальные изменения нарезки — в контракте 0.11.1.

Implemented native extension, documented on 18 November 2026. Scope: WAV browsing, prepared sample pagination, four-part editing, named folders and four-slot assignment at 640×480. The [visual review](../../.impeccable/review/sample-folders-v011/review.md) is **SHIP for three corrected findings**; deployment and final hardware verification are separate and are not asserted here.

This surface inherits [DESIGN.md](../../DESIGN.md): flat graphite, bitmap type, neutral navigation, rectangular geometry, existing margins and four parameter fields. Physical **Y / B / A / X** retain peach / blue / sage / lavender; Windows uses **Z / X / C / V**. The range lines reuse those button colours. A neutral marker identifies the selected part; a separate neutral playhead and named stop action identify audition. No new token, font, raster asset or aesthetic is introduced. This record extends native TAPE/assignment only; browser and other instrument contracts retain their scope.

## Interaction and state contract

| State | Input and result | Visible state / recovery |
|---|---|---|
| Import browser | TAPE → SOURCE → X IMPORT, or VOICE → ASSIGN / REPLACE SAMPLE → IMPORT FROM DEVICE. A enters a folder or opens WAV; B goes up and restores its previous row, then closes at the virtual root. X auditions/stops a selected WAV. | Current path, folders before WAVs, focus outline and page count. Roots: `imports`, `captures`, shared `MOSS-SampleLibrary`. Empty/unavailable folders provide B BACK. Directory limit is 512 entries, depth 16; truncation asks to split the folder. |
| Prepared library | Arrows select; R1+left/right pages. A opens a set; B or `.. LIBRARY` returns. X auditions a selected file. A on a file assigns in assignment context, otherwise opens an editable copy. | Import browser has seven rows per page; prepared library has five. Named folders show `4 WAV`; files show size. Page gestures clamp at the ends and their release does not switch banks. |
| Enter four parts | Open WAV, then L1+R1 → SPLIT INTO FOUR PARTS. | Initial boundaries divide the source into quarters. Each range is capped at 1 MiB PCM16, so a long source can leave gaps. Actual ranges and sizes remain visible; sources shorter than 4 ms are rejected. This is equal-position slicing, not transient detection. |
| MOVE | Hold Y/B/A/X with arrows to move part 1/2/3/4 while preserving its length. Left/right is 1 ms; up/down is 100 ms, with accelerated repeat. Plain arrows select a part silently. | Four range rows map directly to four fields showing start time and size. Movement clamps to source limits. Ranges may overlap or be out of time order; part numbers still determine destination slots. |
| TRIM | L1+right/up selects TRIM; L1+left/down selects MOVE. Selection clamps to these two states, including held repeat. In TRIM, hold Y START / B END / A FADE IN / X FADE OUT with the same fine/coarse axes. | Heading identifies `TRIM / PART n`; numeric values and ms units stay visible. Footer names L1+R1 OPTIONS. Short L1/R1 still changes the outer TAPE bank. |
| Audition in either editor | Tap Y/B/A/X to audition its corresponding part. Tap the same button to stop; another button switches immediately. Editing or plain-arrow part selection stops the current preview. Releasing an editing chord never triggers audition. | Status names the playing part and its actual stop button, e.g. `PART 2 PLAY / B STOP`. TRIM taps still select/audition parts; they do not invoke manual save. SELECT exits; a VOICE import returns to the instrument without assignment. |
| Name and save | L1+R1 → NAME + SAVE FOLDER uses the shared 24-character name panel. A SAMPLER import offers NAME FOLDER + ASSIGN FOUR. | A successful folder contains `01.wav`–`04.wav` and `set.moss`. Existing names are rejected without overwrite. Four free catalog entries are required. A failed save keeps the edit and publishes no incomplete set; retry after correcting the name/storage issue. |
| Assign a set | Y on a folder or inside it replaces slots 1–4 of the current SAMPLER track. Preparation completes before changing assignments. | `Y REPLACE 4 > nn / track name` exposes scope and destination. Synth/GRANULAR context instead says `LOAD SET: OPEN A SAMPLER TRACK FIRST`. The four slot assignments/settings change together; steps and track effects remain. Failure leaves prior assignments intact. OPTIONS → UNDO LAST EDIT restores the prior slots. |

Manual START/END preparation remains available. Import browsing enumerates names without recursively loading audio; opening or auditioning reads the selected source. Large source auditions stream. Set saving publishes the directory only after all four files and its manifest are complete.

## Storage boundary

The disk catalog holds **256 prepared samples**, including set members. This does not enlarge resident PCM: **32 × 1 MiB plus the existing 1 MiB scratch buffer**. Preparation occurs while audio is stopped; the audio callback reads prepared views and performs no file I/O. SONG prepares the union of referenced samples before playback, rejecting more than 32 distinct samples; sequence transitions do not read storage.

MS17 stores persistent catalog IDs; MOS5–MS16 remain readable. Existing IDs, WAVs, names and projects are preserved rather than renumbered or rewritten on update. New sequences using IDs above 31 require 0.11. Implementation: `app_sample_folders.h`, `app_sample_folders_ui.h`, `sample_catalog.h`, `library_kits.h` and the input/service paths in `app.h`, under `native/prototype/src/`.

## Русский

Это локальный контракт нативного расширения 0.11: папки WAV, страницы библиотеки, четыре части и назначение набора. Сохранены плоский графит, bitmap-шрифт, нейтральная навигация, четыре поля и физические цвета **Y/B/A/X: персиковый / голубой / шалфейный / лавандовый**. Новых визуальных токенов нет.

Импорт открывается из TAPE → SOURCE → X IMPORT или назначения в VOICE. **A** входит в папку/открывает WAV, **B** возвращает вверх с прежним выделением, в корне закрывает список; **X** прослушивает/останавливает WAV. **R1+←/→** листает по семь строк импорта или пять строк библиотеки. Видны путь, фокус и страница; пустая/недоступная папка предлагает B BACK. Пределы: 512 записей в папке, 16 уровней. Корни — `imports`, `captures` и общая `MOSS-SampleLibrary`.

**L1+R1 → SPLIT INTO FOUR PARTS** ставит начальные границы на четвертях исходника. Каждый диапазон ограничен 1 МиБ PCM16; у длинного файла возможны промежутки, показаны реальные границы и размеры. В **MOVE** удержание **Y/B/A/X+стрелки** двигает соответствующую часть без изменения длины: ←/→ по 1 мс, ↑/↓ по 100 мс, с ускорением. Границы ограничены исходником; перекрытия и иной временной порядок разрешены. Обычные стрелки выбирают часть без звука.

**L1+вправо/вверх → TRIM**, **L1+влево/вниз → MOVE**; удержание не переключает обратно. В TRIM поля — **Y START / B END / A FADE IN / X FADE OUT**, заголовок указывает редактируемую часть. Короткие L1/R1 сохраняют смену внешнего банка. В обоих режимах короткие **Y/B/A/X** прослушивают части 1/2/3/4; повторная та же кнопка останавливает, другая переключает. Статус называет реальную кнопку остановки. Правка и выбор стрелкой останавливают предпросмотр; отпускание жеста правки не запускает звук. SELECT выходит, из импорта VOICE — без назначения.

**NAME + SAVE FOLDER** использует общую панель имени до 24 символов и создаёт `01.wav`–`04.wav` с `set.moss`. Имя существующей папки отклоняется; нужны четыре свободные записи каталога. Ошибка сохраняет нарезку для повторной попытки и не публикует неполный набор. Для импорта из SAMPLER доступно **NAME FOLDER + ASSIGN FOUR**. Ручной START/END остаётся.

В библиотеке **Y** на папке или внутри неё заменяет назначения и настройки четырёх слотов текущей дорожки SAMPLER. Перед действием видны номер/имя дорожки и область `REPLACE 4`; для синта/GRANULAR показана инструкция открыть SAMPLER. Шаги и эффекты сохраняются, ошибка чтения не меняет назначения, **UNDO LAST EDIT** возвращает прежние слоты. A открывает папку; отдельный файл назначается через A в контексте назначения.

Каталог — **256** образцов на диске, PCM — прежние **32 МиБ + 1 МиБ**. Подготовка при остановленном аудио, callback без файлового I/O; SONG заранее готовит объединение образцов и отклоняет более 32 разных. MS17 сохраняет постоянные ID и читает MOS5–MS16; старые файлы/ID не переписываются. Секвенции с ID выше 31 требуют 0.11.

## Evidence and limits / Подтверждение и границы

User guides: [Русский](../../docs/SAMPLE-FOLDERS.md) / [English](../../docs/SAMPLE-FOLDERS.en.md). The reviewer marked three fixes resolved: clamped MOVE/TRIM selection, truthful options/stop instructions, and visible four-slot destination/recovery. The review covers ten supplied native-renderer recaptures and inspected handlers/fixture assertions; it is not a new exhaustive surface review. The documenter separately inspected `four-parts.png`, `trim-part.png`, `playing-part.png`, `kit-folder.png` and `kit-ineligible.png` in that review folder.

Still captures and inspected tests do not establish handheld readability, audio quality, I/O latency or sustained performance. Final deployment/hardware evidence remains pending for this documentation pass; no installation claim is made. / Визуальный SHIP подтверждает три исправления, но не заменяет проверку читаемости, звука, задержек и нагрузки на устройстве. Установка этой записью не подтверждается.
