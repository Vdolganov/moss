# Native slicer refinement · 0.11.1

Mode: Operate. Supersedes only the slicing presentation/navigation and audition/master-capture level clauses of [0.11](native-sample-folders-v011.md). Folder browsing, kit publication, assignments/Undo, catalog IDs and PCM budgets stay as documented there. The user explicitly requested color overlays, standard tabs, precise zoom, actual waveform detail and contiguous defaults.

## Controls and state

- TAPE with four parts: SOURCE / MOVE / TRIM / FINE / LIBRARY, short L1/R1. Importing from VOICE exposes MOVE / TRIM / FINE. The old L1+arrow mode switch is removed. Options remain L1+R1; SELECT exits.
- MOVE holds Y/B/A/X to translate the corresponding whole range: 1 ms horizontally, 100 ms vertically. TRIM retains START / END / FADE IN / FADE OUT. Selection and exact boundaries persist between tabs.
- FINE: Y START, B END, A ZOOM, X VIEW. Boundary deltas are one source frame horizontally, ten vertically. Range positions store integer frame remainders without rounding at export. Zoom starts around the selected range, clamps at a 16-frame view, and VIEW pans by 1% or 10% of the window. An edited offscreen boundary recenters into view.
- Short Y/B/A/X taps audition parts in every editor bank; a gesture release never auditions. Selection/editing stops the previous range preview. Tab changes within the editor retain preview; leaving the editor stops it. Empty/no-source states retain the manual/source flow.
- Four initial ranges are adjacent from frame zero. A source fitting four budgets is split evenly in frames; a longer source uses four adjacent 1 MiB PCM16 segments. ARRANGE PARTS IN ORDER keeps all lengths/fades, packs from part 1, shifts the set left if needed, and rejects overfull total lengths without mutation.
- Names/save/load/error recovery follow the 0.11 set contract. Exact fine bounds are used for preview, single saves and all four kit files.

## Rendering

Existing graphite, bitmap font and physical Y/B/A/X colors. The active range uses stronger preblended fill plus outline; unselected ranges are lighter. No lower range lanes remain. Color labels identify physical buttons. Waveform and playhead use actual sample/frame data. Overlapping regions render inactive first and active last; labels may be omitted when their positions would collide, with all four controls still visible below.

The full view and FINE cache signed min/max for each of 604 pixel columns, for each WAV channel. Every frame in the viewport is decoded, including transients; deep zoom shows individual source values. Stereo has two bands. The UI pumps at most 32 KiB per iteration; rendering and audio callbacks perform no file I/O. File audition pumping comes first. Waveform work pauses during project transport. Loading/error states are explicit. No PCM cache enlargement. Precise values longer than seven digits use the existing smaller bitmap size without truncating numbers.

## Audio

Cuts without fades preserve PCM16 bit for bit; higher-depth conversion retains the existing PCM16 contract. Source and prepared-library auditions share unity gain, bypassing project/master gain. Project output gain/protection stays unchanged. MASTER capture now records the audible project master after output gain/protection and transition smoothing, excluding auditions. Track capture stays after track FX/fader and before master. Playback through sampler tracks still applies velocity, key level, track/master levels and FX. Old WAVs are not rewritten.

## Русский

Цветные области Y/B/A/X находятся непосредственно на реальной стереоволне; активная область плотнее и обведена. Нижние линии удалены. L1/R1 листает SOURCE / MOVE / TRIM / FINE / LIBRARY, в импорте VOICE — три редакторных вкладки. FINE задаёт START/END по одному фрейму (↑/↓ по десять), A управляет увеличением, X — положением окна. Короткие Y/B/A/X сохраняют предпрослушивание частей.

Начальные части идут стык в стык с начала исходника и ограничены 1 МиБ каждая. ARRANGE PARTS IN ORDER состыковывает изменённые диапазоны, сохраняя их длины/fade; слишком длинный набор отклоняется без изменений. Сохранение использует точные границы, а не округлённые миллисекунды.

Предпросмотр исходника и готового WAV имеет единичное усиление. MASTER-захват соответствует слышимому выходу после усиления/защиты, без предпрослушивания. Нарезка без fade не меняет PCM16. Дорожечный velocity/LEVEL и микшер остаются отдельным этапом.
