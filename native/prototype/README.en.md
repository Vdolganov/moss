# MOSS 0.13.1 — user guide

[New native UI and control map](../../docs/UI-v013.en.md)

[FM 0.12 — new voice and five banks](../../docs/FM-v012.en.md)


[Русский](README.md) · **English**

MOSS 0.13.1 is the single stable application for original RG35XX and Windows. GRANULAR is included; the separate MOSS-GRANULAR application is retired. Four tracks, 32 sequences, 32 SONGs with 64 slots, 32 kHz audio and 1024 stereo frames. The catalog holds 256 samples; the PCM cache remains 32 × 1 MiB. [Folders and four parts](../../docs/SAMPLE-FOLDERS.en.md).

MONO and ORGANISM each have five banks and twenty controls; GRANULAR has PLAY / SCAN / TEXTURE / ENVELOPE / CLOUD with eight grains per track. All five effects are available, two active per track and two on master. Current detail: [ORGANISM](../../docs/ORGANISM-v010.en.md), [MONO](../../docs/MONO-v09.en.md), [GRANULAR](../../docs/GRANULAR-INSTRUMENT.en.md), [REEL](../../docs/REEL-v1.en.md), [POND](../../docs/AMBIENT-RESEARCH-v08.en.md), [DRIVE/ORBIT](../../docs/FX-CHARACTER-v1.en.md), [LOCK ONLY/REPEAT](../../docs/TRIGLESS-LOCKS-PLAN.en.md), [SWING/SHIFT](../../docs/SWING-v1.en.md).

Saves use MS18 and read MOS5…MS17; SONG remains SON1. Back up original data: older binaries cannot read MS18, and updated ORGANISM/MONO/POND change patch sound. [Filesystem and load verification 0.11](../../docs/verification/sample-folders-v011/README.md). Browser prototype and hardware lab are separate tools.

## Launching and updating

Open Consoles → APPS → MOSS. On first initialization of an empty project library, SLOW SIGNAL is created with ORGANISM / MONO / FM. Select it, press A, then START. There is no create-DEMO menu action.

To make your own sequence: ATLAS / SEQUENCES → **+ NEW SEQUENCE** → EMPTY, or an existing sequence to copy independently → enter a name → START. Then GRID → **+ ADD TRACK** → choose an instrument → A. EMPTY creates an empty sequence at 120 BPM; a copy retains its source settings. The add row remains visible at four tracks, but explains that the limit has been reached.

New saves use MS18. MOS5…MS17 can be read without rewriting the source until you save. Old MOS5 data is limited to the first four tracks, four sampler slots and two active effects per chain. MOS3/MOS4 are not supported. For migration from MOSS-GRANULAR, preserve the complete project/library set as described in the [installation guide](../../README.en.md).

Before updating, back up original projects and user files. Update the application while preserving `projects/`, `samples/`, sample names, `imports/` and `captures/`. Test WAVs from a clean package must not overwrite your own bank. Deleting a project does not delete its WAVs.

The launcher selects the performance governor, capped at the stock **1.008 GHz**, without overclocking. It restores the previous governor and frequency limits on exit.

## Buttons and navigation

| Action | RG35XX | Windows |
|---|---|---|
| Main tabs; ATLAS SEQUENCES / SONGS | L2 / R2 | [ / ] |
| Visible parameter or local tabs | Tap L1 / R1 | Tap Q / E |
| Sampler slot or TAPE region | L1 + ← / → | Q + ← / → |
| Nested parameter page | L1 + ↑ / ↓ | Q + ↑ / ↓ |
| Grid or list page | R1 + ← / → | E + ← / → |
| Current screen options | L1 + R1 | Q + E |
| Four fields, left to right | Y / B / A / X | Z / X / C / V |
| Fine / coarse adjustment | Field button + ←→ / ↑↓ | Field key + ←→ / ↑↓ |
| SET LOCK / CLEAR LOCK / RESET TO BASE | Field button + MENU | Field key + Esc |
| Confirm / back | A / B | C / X |
| Transport | START | Space |
| Tempo | SELECT + arrows | Tab + arrows |
| Save and exit | Hold MENU for 2 seconds | Hold Esc for 2 seconds |

Use D-pad to select list rows. Holding L1 reveals a hint; releasing a held or used modifier does not also change tabs. Open menus isolate the underlying screen. The name keyboard retains L1/R1 caret movement.

## Entering names

All names use the same onscreen keyboard: new projects, project/track/sample renaming, and saving one slice or a set of slices. It contains **A–Z, 0–9, space and hyphen**, plus DEL and SAVE. The limit is **24 characters**, with a counter above the field.

- Arrows select a key; A enters the character or activates DEL/SAVE.
- X deletes the character before the caret; START saves.
- L1/R1 moves the caret; B or SELECT cancels.
- The original name is fully selected on entry. The first character replaces it; X clears it. L1/R1 clears the selection and moves to the start/end.
- Empty names cannot be saved. A write error preserves the entered text for retry; slice naming also retains the selected range.

## PATTERN and independent cycles

Open a track from GRID with A. Arrows select a step; R1+←/→ changes pages. OPTIONS → GRID VIEW selects 16 (8×2) or up to 64 (16×4) cells per page; 64 is the default. Short A toggles a trigger. Filled cells contain notes, a diamond marks locks, a border marks selection and an inverted caption marks playback. Duration lines carry across rows/pages, with small note labels. Sampler slot activity follows actual voices.

L1/R1 selects a bank. The four fields always follow **Y / B / A / X**:

| Bank | Y | B | A | X |
|---|---|---|---|---|
| SAMPLER STEP | VELOCITY 0–127 | FADE IN 0–5000 ms | LENGTH: FULL or finite duration | PROB 0–100% |
| Synth STEP | NOTE C1–C7 | VELOCITY 0–127 | LENGTH in steps | PROB 0–100% |
| SAMPLER VOICE LOCKS | PITCH ±24 semitones | START % | END % | LEVEL % |
| TIMING 1/2 | CYCLE: number of cells | STEP: 1/8, 1/16, 1/32, 1/64 | REPEAT | TYPE |

Each SAMPLER has **four independent hit patterns**. Named slots 1–4 (C3–D#3) remain visible above the grid. **L1+arrows** selects a slot; the grid shows that slot's hits. Empty slots are selectable, but require a sound assignment in VOICE before enabling a hit. Several slots may trigger together. Retriggering a slot smoothly replaces its own voice while the others continue.

**LENGTH = FULL** is stored as 0 and plays the selected range once to its end. A finite value limits that particular hit, with a short release to prevent clicks. A short source always ends naturally; increasing LENGTH never loops or stretches it. FADE IN applies to the hit. Old fade-out values remain in the saved data, while TAPE fades are written into the WAV itself.

CYCLE changes musical duration while preserving hidden events beyond the new end. STEP changes only resolution: 15 cells at 1/8 become 30 at 1/16, 60 at 1/32 and 120 at 1/64. Event times, finite durations and BPM stay unchanged. The limit is **512 ticks at 1/64**, equivalent to 64 eighth notes. Events between coarse grid lines still play and are marked in the cell; switch back to a finer grid to edit them. A partial final cell is labelled LAST CELL.

V-LOCK follows the five instrument banks; L1+↑/↓ selects a nested page. Each enabled effect has its own LOCKS tab in chain order; bypass preserves stored locks. `--` inherits; RST returns to the base value. Use a field key + MENU or OPTIONS → PARAMETER for SET/CLEAR/RESET. LOCK ONLY modifies a sustaining voice without retriggering. TIME contains cycle/division, repeat/type and swing/shift.

FX locks affect the **shared track bus**, through which all its slots pass, and remain attached to the effect's identity when the chain is reordered. The effect must be enabled. OPTIONS offers LENGTH TO CYCLE END on STEP, and CLEAR CURRENT LOCK BANK only when that bank contains locks.

## VOICE: sampler and import

In VOICE, arrows or L1/R1 select any of the four slots. A auditions an assigned sound or opens the library for an empty slot. Use OPTIONS → ASSIGN / REPLACE SAMPLE to replace an assignment, or REMOVE ASSIGNMENT to clear it. One WAV can be assigned to several slots with independent PITCH / START / END / LEVEL. Four-part slicing is available in TAPE; Y loads a prepared folder into all four slots.

To import directly into a slot:

1. In the sample picker, open **+ IMPORT FROM DEVICE**.
2. Select a WAV: X auditions the full file; A opens EDIT.
3. Y / B / A / X fields set START / END / FADE IN / FADE OUT. Short X auditions the selected range.
4. Short A opens the name editor; enter a name and press START or onscreen SAVE.
5. The new WAV is saved to the library, assigned to the original slot, and the screen returns to VOICE.

B/SELECT in the name editor returns to the same EDIT state. B in the file browser goes up a folder, then closes at the root; B out of manual import returns to VOICE without replacing the assignment. In four-part mode, SELECT returns to VOICE and B auditions part 2. Errors, unsupported files and a full library preserve the old assignment.

## VOICE: synthesizers

OPTIONS → CHOOSE INSTRUMENT selects SAMPLER / MONO / ORGANISM / FM / GRANULAR. Switching instruments retains their separate settings, patterns and locks; the previous voice is faded out. A synth has one musical voice per track. Four internal oscillators/operators do not provide chord polyphony.

MONO has CORE / COLOR / OSC / ENV / MOTION, with all twenty controls described in the [MONO guide](../../docs/MONO-v09.en.md). ORGANISM has CORE / TONE / MOTION / VOICES / ENVELOPE; see the [ORGANISM guide](../../docs/ORGANISM-v010.en.md). FM has CORE / BODY / HIT / COLOR / MOTION (twenty controls). L1/R1 switches banks; Y/B/A/X edits the four fields. Mode and presets are in L1+R1 options. Presets replace all base controls for that instrument while retaining mode and locks. MONO displays actual pre-effect audio and envelopes; ORGANISM uses four contours driven by actual voice telemetry; FM shows live voice and envelope telemetry. A auditions; arrows without a modifier choose the audition note.

## SPACE and MIX

The chain contains ORBIT / POND / DRIVE / CRUSH / REEL, once per type. **Up to two effects may be active per track, and two on the master**, for a maximum of ten across four tracks. To enable a third, first put one of the active effects into BYPASS.

Arrows select a row, A opens its parameters, X enables/BYPASSes it, and **Y+↑/↓** moves it in the chain. Processing runs from top to bottom. Inside the parameter editor, L1+arrows selects an effect and B returns to the chain. Order, settings and locks persist regardless of BYPASS.

ORBIT and DRIVE expose CORE/CHARACTER; POND exposes TAIL/SPACE; REEL exposes COLOR/MOTION/LIFE/OUTPUT; CRUSH has one bank. L1/R1 changes the bank. Every parameter has a matching track lock. REEL separates saturation, tone/body, wow/flutter depth and rate, drift, wear, dropouts and hiss. Delay/reverb tails continue through empty steps.

New track, slot and MASTER levels default to **100%**. The common output path has unity gain with soft peak protection; there is no hidden division by track count or monitor boost. A auditions at velocity 127; sequenced notes retain their own velocity (90 for new steps). MASTER capture records the audible output, excluding file auditions. A track capture stays after its FX/fader and before the master. [Level calibration and existing-project behavior](../../docs/LEVELS-v0131.en.md).

## TAPE: files, ranges and the library

L1/R1 selects SOURCE / EDIT / LIBRARY. Arrows select a range in EDIT or a sample in LIBRARY.

- **SOURCE → X:** WAV file list; X auditions the full file, A opens EDIT.
- **EDIT:** Y / B / A / X + arrows edits START / END / FADE IN / FADE OUT. For boundaries, ←/→ changes 1 ms and ↑/↓ changes 100 ms; holding accelerates. Extending beyond the size limit shifts the opposite boundary. Short X plays/stops the range; OPTIONS → AUDITION FULL SOURCE plays the full source.
- **Short A in EDIT:** enter a name → START/SAVE. Saves an independent PCM16 WAV, leaving the source intact.
- **Multiple ranges:** OPTIONS → ADD NEXT RANGE, up to 16. SAVE ALL SLICES and REMOVE SELECTED SLICE appear when there is more than one range. SAVE ALL asks for a common name and adds numbers. SPLIT INTO FOUR PARTS creates four initial ranges: [controls and folder saving](../../docs/SAMPLE-FOLDERS.en.md).
- **LIBRARY:** R1+left/right jumps pages. A opens a folder; Y loads a set into the current SAMPLER’s four slots. For individual files, A edits a copy; X auditions. OPTIONS renames or deletes an unused sample. Open a VOICE slot to assign a sample.

A prepared sample/range is limited to **1 MiB PCM16**; the shared library holds **256 samples**, with a **32 MiB resident PCM cache**. EDIT displays the size. Duration depends on sample rate and channels: approximately 5.94 seconds of stereo at 44.1 kHz, or 23.78 seconds of mono at 22.05 kHz. SAVE ALL checks sizes and free slots; an error reports how many files have already been written.

Long-source audition streams through a 65,536-stereo-frame PCM16 buffer / 256 KiB. Supported inputs are RIFF WAV PCM16/24/32 or float32, mono/stereo, 8–96 kHz. WAV Extensible, RF64 and compressed formats are unsupported. Saving retains source sample rate and channel count; there is no automatic downsampling. Search locations are `imports/`, `captures/` and `../../../MOSS-SampleLibrary/`, with limits of 512 entries per current folder and 16 directory levels.

Deletion requires confirmation and is unavailable if a sample is needed by an open/saved project, Undo, or a recoverable deleted project. Deleting a prepared sample does not delete the full source WAV. Range markers live only for the current session; save the desired slices before exiting. TAPE file operations are not included in Undo.

## Internal resampling

In SOURCE, **Y+arrows** chooses MASTER or an existing TR 01–TR 04; **B+arrows** chooses 1–8 bars. A starts recording; another A ends it early. A stopped transport starts from the beginning; during playback, capture starts immediately. Duration is calculated from BPM at capture start, and the available limit is shown onscreen.

MASTER records the sum including master effects. TR records the track with its SPACE, level and pan, before the master. External input/microphone recording is not supported. The resulting WAV is saved as `captures/take-NNNN.wav` and opened in EDIT.

During **SAVING CAPTURE / PLAYBACK CONTINUES**, the WAV and its overview are prepared in the background while music and UI continue. The completed PCM buffer remains immutable until success. If writing fails, the recording stays in RAM: A in SOURCE retries, and OPTIONS → DISCARD CAPTURE discards it. Exit, project switching and structural operations remain blocked until save or discard.

Opening a source, starting audition, saving a slice or deleting a sample stops transport; audition does not restart it afterward. Saving a project or completed capture does not require stopping.

## Sequences, SONG, Undo and exit

ATLAS contains SEQUENCES / SONGS tabs. SEQUENCES holds up to 32 sequences and a + NEW SEQUENCE row; SONGS holds up to 32 arrangements and + NEW SONG. GRID holds up to four tracks and + ADD TRACK. OPTIONS is filtered by screen and state: project/track management belongs in the relevant lists, instruments/presets in VOICE, and file operations in TAPE. Empty menus never open.

TRACK OPTIONS and SEQUENCE OPTIONS include RENAME / DUPLICATE / DELETE. Copies are named COPY; duplicating a track retains patterns, locks, assignments, both parameter banks and FX. Deletion requires A confirmation. SAVE SEQUENCE saves without exiting; a + beside the name indicates unsaved changes.

UNDO LAST EDIT reverses one last project gesture, including track-structure changes; repeating Undo swaps the two states. Structural operations and Undo stop transport. Project-file operations are outside this Undo: RESTORE DELETED SEQUENCE in ATLAS restores the most recently deleted project when space is available; the next deletion replaces that recovery copy.

**SONG:** select SONGS → + NEW SONG → enter a name. In an empty list, A on + ADD SEQUENCE adds an existing sequence or opens new-sequence creation. Each row has a repeat count and the duration of one repeat in 4/4 bars. Default duration rounds up from the longest track: 15/16-step tracks at 1/16 default to one bar, without waiting for their cycles to realign.

SONG fields, left to right: **Y POSITION / B REPEATS / A BARS / X LOOP**. Y+↑/↓ moves the row. B+←/→ changes repeats by one, ↑/↓ by ten. A+←/→ changes bars by one, ↑/↓ by four. X+arrows enables/disables looping the whole SONG. R1+←/→ pages the list four rows at a time. START plays from the selected row or stops playback; if + ADD is selected, playback starts from the last existing slot.

A SONG has **one shared BPM**: SELECT+←/→ changes one, SELECT+↑/↓ changes ten. Within a slot, polyrhythms continue across repeats without phase reset. Entering the next slot resets the local sequence position to zero. LOOP OFF ends the SONG and lets effects decay. Individual files' BPM values apply when sequences play independently.

A on a row opens its sequence; B from that sequence's GRID or short SELECT saves and returns to the same SONG row. X replaces the slot's sequence, resets repeats to one and derives a new duration from the longest track. OPTIONS can save/rename the SONG, undo an edit, duplicate/remove a slot, or **COPY SEQUENCE AS NEW**: name an independent variation, replace the current slot with it and open its editor. Ordinary DUPLICATE SLOT references the same sequence file; edits to that sequence are heard in all slots that reference it.

During SONG playback, you can browse the list and change common BPM; stop with START before structural edits. All required sequences are loaded and checked for missing files/samples before playback. No card reads occur at a slot boundary. Corresponding track effects retain their tails; old notes end, and a short transition limits clicks when routes change. One engine is used, without overlapping two complete sequences.

SONGs are stored separately as `projects/song-N.moss` (SON1); existing sequence references are preserved. Deleting a slot/SONG does not delete sequences or WAVs. A sequence referenced by a SONG or its Undo cannot be deleted. Save errors retain edits for retry. Changing a track cycle later does not automatically change an existing slot's duration.

To exit, hold MENU for two seconds or open **ATLAS → OPTIONS → EXIT MOSS**. A save failure leaves the application open. Projects and WAVs use temporary-file writes followed by replacement; losing power during a write can still damage the filesystem.

The native application does not yet support scenes, A–D pattern banks, key/scale metadata, or JSON transfer from the browser prototype.

## Build and verification

Run `node scripts/build.cjs` for ARM/Windows and the complete native suite. `scripts/package.py` checks binaries, ABI and exact hardware evidence. Garlic uses SDL 1.2/evdev, Windows GDI/waveOut. Rendering performs no file I/O or memory allocation; reports are written asynchronously. `late_intervals` is an interval diagnostic, not an exact audible-glitch count.

See the [0.9.0 release](../../releases/0.9.0/RELEASE.en.md) for current evidence. Dated 0.5.x–0.9.0-dev reports remain historical and retain their original hashes.

Four-part slicing uses L1/R1 for SOURCE / MOVE / TRIM / FINE / LIBRARY, or MOVE / TRIM / FINE during VOICE import. Colored ranges overlay the actual stereo waveform. FINE: Y START / B END move by one source frame, up/down by ten; A ZOOM / X VIEW set magnification and viewport. OPTIONS → ARRANGE PARTS IN ORDER joins the four ranges without changing their lengths. [Details](../../docs/SAMPLE-FOLDERS.en.md).
