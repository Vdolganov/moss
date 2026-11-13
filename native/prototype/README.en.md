# MOSS 0.5.2 — user guide

[Русский](README.md) · **English**

A native groovebox for the original **Anbernic RG35XX**: up to **32 sequences with four tracks each**, plus **32 SONGs with 64 slots each**, SAMPLER / MONO / ORGANISM / FM, independent cycles, parameter locks, a WAV library and internal resampling. Each SAMPLER has **four slots of its own**. Each track and the master can run up to two effects from ORBIT / POND / CRUSH / REEL simultaneously. Display: 640×480. Audio: **32000 Hz, 1024 stereo frames per buffer**.

The build has been tested on **GarlicOS 1.4.9 over ADB**. GarlicOS 1.3.9 remains a compatibility target; this 0.5.2 binary has not been retested there. Current results are in the [0.5.2 verification report](../../docs/NATIVE-VERIFICATION-052.en.md); the previous version's longer tests are in the [0.5.1 performance report](../../docs/NATIVE-PERFORMANCE-051.en.md). MOSS LAB and browser prototype v0.6 are separate applications; their limits and controls do not replace this guide.

## Launching and updating

Open Consoles → APPS → MOSS. On first initialization of an empty project library, SLOW SIGNAL is created with ORGANISM / MONO / FM. Select it, press A, then START. There is no create-DEMO menu action.

To make your own sequence: ATLAS / SEQUENCES → **+ NEW SEQUENCE** → EMPTY, or an existing sequence to copy independently → enter a name → START. Then GRID → **+ ADD TRACK** → choose an instrument → A. EMPTY creates an empty sequence at 120 BPM; a copy retains its source settings. The add row remains visible at four tracks, but explains that the limit has been reached.

The current version saves projects as **MOS6**. Loading MOS5 retains the first four tracks, the first four assigned slots per track compacted in their original order, and the first two active effects in each chain's processing order. Remaining effects are bypassed, with their settings retained. Loading alone never overwrites an old file; a subsequent save writes MOS6. MOS3/MOS4 are not converted.

Before updating, back up original projects and user files. Update the application while preserving `projects/`, `samples/`, sample names, `imports/` and `captures/`. Test WAVs from a clean package must not overwrite your own bank. Deleting a project does not delete its WAVs.

The launcher selects the performance governor, capped at the stock **1.008 GHz**, without overclocking. It restores the previous governor and frequency limits on exit.

## Buttons and navigation

**Parameter fields, left to right: Y / B / A / X.** On Windows, the corresponding keys are **Z / X / C / V**. Hold the field's button: ←/→ changes one unit; ↑/↓ makes a larger change. Coarse increments are 10 for percentages, levels and velocity; 100 for Hz and milliseconds; an octave for pitch; and 16 cells for LENGTH and CYCLE. Enumerated choices advance one at a time. Holding an arrow accelerates repetition. Releasing a button after editing never fires its short-press action.

| RG35XX | Windows | Action |
|---|---|---|
| D-pad | Arrow keys | Select a step, object or item |
| Y / B / A / X + arrows | Z / X / C / V + arrows | First / second / third / fourth field |
| Short A | C | Open, confirm, toggle a trigger, audition VOICE |
| Short B | X | Back or cancel |
| Short X | V | Audition TAPE/WAV; enable or bypass FX |
| Short Y | Z | Does not change pages |
| L1 / R1 | Q / E | Previous/next bank; SEQUENCES/SONGS in ATLAS, track selection in GRID |
| L2 / R2 | [ / ] | GRID / MIX / TAPE or PATTERN / VOICE / SPACE |
| L1 + arrows | Q + arrows | Nested slot, CORE/CHARACTER lock bank or FX target |
| R1 + ←/→ | E + ←/→ | Grid or list page |
| L1 + R1 | Q + E | Context options, when available |
| START | Space | Play/stop; save in the name editor |
| SELECT + arrows | Tab + arrows | Tempo, 30–240 BPM, without resetting musical time |
| Short SELECT | Tab | ATLAS; close/cancel in dialogs |
| Hold MENU for 2 seconds | Hold Esc for 2 seconds | Save and exit when capture is safe |

L1/R1 selects STEP / VOICE LOCKS / FX LOCKS / CYCLE in PATTERN, CORE / CHARACTER in synth VOICE, LEVELS / MASTER FX in MIX, and SOURCE / EDIT / LIBRARY in TAPE. In sampler VOICE it selects a slot; in SPACE it selects an effect. Releasing L1 after a nested L1+arrow gesture does not also switch banks. Releasing SELECT after changing tempo does not fire its short action either. R1+←/→ jumps 16 grid cells while preserving the position within the page, clamping to the closest available cell on a partial last page. MIX keeps its LEVELS / MASTER FX tabs visible even inside an effect editor.

Tabs and banks are labelled. Colors belong to physical buttons: warm Y, blue B, sage A and lavender X. A neutral border marks the selected object. Holding a parameter retains its unit of measurement; diagnostic hold timers are not displayed.

## Entering names

All names use the same onscreen keyboard: new projects, project/track/sample renaming, and saving one slice or a set of slices. It contains **A–Z, 0–9, space and hyphen**, plus DEL and SAVE. The limit is **24 characters**, with a counter above the field.

- Arrows select a key; A enters the character or activates DEL/SAVE.
- X deletes the character before the caret; START saves.
- L1/R1 moves the caret; B or SELECT cancels.
- The original name is fully selected on entry. The first character replaces it; X clears it. L1/R1 clears the selection and moves to the start/end.
- Empty names cannot be saved. A write error preserves the entered text for retry; slice naming also retains the selected range.

## PATTERN and independent cycles

Open a track from GRID with A. Arrows select a step in an 8×2 grid; moving beyond a page opens the next one. Short A toggles the trigger. A light cell is an active step, the border is the selection, the upper mark is the playhead, the lower line shows the selected event's finite duration, and a small marker indicates locks. PLAY follows the current track's cycle.

L1/R1 selects a bank. The four fields always follow **Y / B / A / X**:

| Bank | Y | B | A | X |
|---|---|---|---|---|
| SAMPLER STEP | VELOCITY 0–127 | FADE IN 0–5000 ms | LENGTH: FULL or finite duration | PROB 0–100% |
| Synth STEP | NOTE C1–C7 | VELOCITY 0–127 | LENGTH in steps | PROB 0–100% |
| SAMPLER VOICE LOCKS | PITCH ±24 semitones | START % | END % | LEVEL % |
| CYCLE | CYCLE: number of cells | STEP: 1/8, 1/16, 1/32, 1/64 | — | — |

Each SAMPLER has **four independent hit patterns**. Named slots 1–4 (C3–D#3) remain visible above the grid. **L1+arrows** selects a slot; the grid shows that slot's hits. Empty slots are selectable, but require a sound assignment in VOICE before enabling a hit. Several slots may trigger together. Retriggering a slot smoothly replaces its own voice while the others continue.

**LENGTH = FULL** is stored as 0 and plays the selected range once to its end. A finite value limits that particular hit, with a short release to prevent clicks. A short source always ends naturally; increasing LENGTH never loops or stretches it. FADE IN applies to the hit. Old fade-out values remain in the saved data, while TAPE fades are written into the WAV itself.

CYCLE changes musical duration while preserving hidden events beyond the new end. STEP changes only resolution: 15 cells at 1/8 become 30 at 1/16, 60 at 1/32 and 120 at 1/64. Event times, finite durations and BPM stay unchanged. The limit is **512 ticks at 1/64**, equivalent to 64 eighth notes. Events between coarse grid lines still play and are marked in the cell; switch back to a finer grid to edit them. A partial final cell is labelled LAST CELL.

Synth VOICE LOCKS mirrors CORE/CHARACTER; L1+arrows changes the nested bank. FX LOCKS contains the selected effect's four fields; L1+arrows selects the FX target. `--` means use the base value. Locks from a successful trigger persist through rests until the next successful trigger; a trigger without a lock restores the base value.

FX locks affect the **shared track bus**, through which all its slots pass, and remain attached to the effect's identity when the chain is reordered. The effect must be enabled. OPTIONS offers LENGTH TO CYCLE END on STEP, and CLEAR CURRENT LOCK BANK only when that bank contains locks.

## VOICE: sampler and import

In VOICE, arrows or L1/R1 select any of the four slots. A auditions an assigned sound or opens the library for an empty slot. Use OPTIONS → ASSIGN / REPLACE SAMPLE to replace an assignment, or REMOVE ASSIGNMENT to clear it. One WAV can be assigned to several slots with independent PITCH / START / END / LEVEL. There is no automatic slicing.

To import directly into a slot:

1. In the sample picker, open **+ IMPORT FROM DEVICE**.
2. Select a WAV: X auditions the full file; A opens EDIT.
3. Y / B / A / X fields set START / END / FADE IN / FADE OUT. Short X auditions the selected range.
4. Short A opens the name editor; enter a name and press START or onscreen SAVE.
5. The new WAV is saved to the library, assigned to the original slot, and the screen returns to VOICE.

B/SELECT in the name editor returns to the same EDIT state. B in the file list closes the list; B out of the import flow returns to VOICE without replacing the assignment. Errors, unsupported files and a full library preserve the old assignment.

## VOICE: synthesizers

OPTIONS → CHOOSE INSTRUMENT selects SAMPLER / MONO / ORGANISM / FM. Switching instruments retains their separate settings, patterns and locks; the previous voice is faded out. A synth has one musical voice per track. Four internal oscillators/operators do not provide chord polyphony.

L1/R1 selects CORE / CHARACTER. Field order is **Y / B / A / X**:

| Instrument | CORE | CHARACTER | CHANGE MODE |
|---|---|---|---|
| MONO | CUTOFF / RESONANCE / ATTACK / RELEASE | ENV AMT / SUB / NOISE / DRIVE | SAW / PULSE |
| ORGANISM | TENSION / DRIFT / CHAOS / DENSITY | COLOR / GRAIN / ATTACK / RELEASE | PAIRS / RING |
| FM | INDEX / RATIO / FEEDBACK / DECAY | DETUNE / NOISE / ATTACK / BODY | PAIRS / STACK |

MONO combines saw/pulse waves with a resonant filter, sub-oscillator, noise and drive. ORGANISM has four coupled oscillators with drift; adjacent triggers retain phase and envelope continuity. FM has four operators arranged as two pairs or a chain. These are original compact engines, without a claim of exact emulation of other instruments. DX7 import is not supported.

MONO CUTOFF: 20–12000 Hz. ATTACK: 0–5000 ms; RELEASE: 0–10000 ms. FM DECAY: 50–10000 ms; RATIO: 0.5/1/1.5/2/3/4/5/7 relative to the carrier. Other fields range from 0 to 100. For Hz/ms, ←/→ changes 1 and ↑/↓ changes 100; holding accelerates repetition.

Without a modifier, ←/→ changes the audition note by a semitone and ↑/↓ by an octave. A plays it for two seconds plus its release tail, without editing a step. CHOOSE PRESET offers four sounds and replaces the eight base parameters while retaining mode and locks; manual editing displays CUSTOM. Step LENGTH controls sustain duration, followed by envelope release. FM also decays while held. MODEL graphs illustrate base settings, not measured audio.

## SPACE and MIX

The chain contains ORBIT / POND / CRUSH / REEL, once per type. **Up to two effects may be active per track, and two on the master**, for a maximum of ten across four tracks. To enable a third, first put one of the active effects into BYPASS.

Arrows select a row, A opens its parameters, X enables/BYPASSes it, and **Y+↑/↓** moves it in the chain. Processing runs from top to bottom. Inside the parameter editor, L1+arrows selects an effect and B returns to the chain. Order, settings and locks persist regardless of BYPASS.

| Effect | Y / B / A / X |
|---|---|
| ORBIT: stereo delay | TIME / FEEDBACK / MIX / SPREAD |
| POND: reverb | SIZE / DECAY / DAMP / MIX |
| CRUSH: distortion/redux | DRIVE / BITS / RATE / MIX |
| REEL: tape | WOBBLE / WEAR / NOISE / MIX |

REEL is original processing for speed fluctuations, wear and noise. Delay and reverb tails continue through empty steps.

In MIX / LEVELS, select a track with the arrows. Fields are LEVEL / PAN / MASTER / MUTE; short A toggles mute. L1/R1 opens MASTER FX. Mute smoothly silences the track together with its SPACE output. **+35%** monitoring gain with peak limiting also applies to auditioning. Internal capture is taken before that gain, so repeated resampling does not accumulate amplification.

## TAPE: files, ranges and the library

L1/R1 selects SOURCE / EDIT / LIBRARY. Arrows select a range in EDIT or a sample in LIBRARY.

- **SOURCE → X:** WAV file list; X auditions the full file, A opens EDIT.
- **EDIT:** Y / B / A / X + arrows edits START / END / FADE IN / FADE OUT. For boundaries, ←/→ changes 1 ms and ↑/↓ changes 100 ms; holding accelerates. Extending beyond the size limit shifts the opposite boundary. Short X plays/stops the range; OPTIONS → AUDITION FULL SOURCE plays the full source.
- **Short A in EDIT:** enter a name → START/SAVE. Saves an independent PCM16 WAV, leaving the source intact.
- **Multiple ranges:** OPTIONS → ADD NEXT RANGE, up to 16. SAVE ALL SLICES and REMOVE SELECTED SLICE appear when there is more than one range. SAVE ALL asks for a common name and adds numbers. Import does not slice automatically.
- **LIBRARY:** A edits a copy; X auditions. OPTIONS renames or deletes an unused sample. Open a VOICE slot to assign a sample.

A prepared sample/range is limited to **1 MiB PCM16**; the shared library holds **32 samples / 32 MiB PCM**. EDIT displays the size. Duration depends on sample rate and channels: approximately 5.94 seconds of stereo at 44.1 kHz, or 23.78 seconds of mono at 22.05 kHz. SAVE ALL checks sizes and free slots; an error reports how many files have already been written.

Long-source audition streams through a 65,536-stereo-frame PCM16 buffer / 256 KiB. Supported inputs are RIFF WAV PCM16/24/32 or float32, mono/stereo, 8–96 kHz. WAV Extensible, RF64 and compressed formats are unsupported. Saving retains source sample rate and channel count; there is no automatic downsampling. Search locations are `imports/`, `captures/` and `../../../MOSS-SampleLibrary/`, with limits of 128 files and five directory levels.

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

SONGs are stored separately as `projects/song-N.moss` (SON1); existing MOS6 sequences are not converted. Deleting a slot/SONG does not delete sequences or WAVs. A sequence referenced by a SONG or its Undo cannot be deleted. Save errors retain edits for retry. Changing a track cycle later does not automatically change an existing slot's duration.

To exit, hold MENU for two seconds or open **ATLAS → OPTIONS → EXIT MOSS**. A save failure leaves the application open. Projects and WAVs use temporary-file writes followed by replacement; losing power during a write can still damage the filesystem.

The native application does not yet support scenes, A–D pattern banks, key/scale metadata, or JSON transfer from the browser prototype.

## Building and verification

From this folder, `node scripts/build.cjs` builds ARM and Windows, runs music / interaction / features / synth / workflow / v05 / v051 / v052, and generates previews. `node scripts/benchmark.cjs` builds ARM/Windows measurement workloads; `scripts/package.py` prepares packages with binary checks and SHA256. Compiler and SDK versions are pinned under `native/tooling`.

Garlic uses SDL 1.2 + evdev; Windows uses GDI + waveOut. Model, audio and controls are shared. Audio rendering performs no file operations or memory allocation. Reports are written asynchronously to `reports/`. `stream_underruns` counts streaming-audition starvation episodes; `late_intervals` is not an exact count of audible dropouts.

The previous 0.5.1 visual review concluded SHIP; captures are in `.impeccable/review/native-v051/` from the repository root. This is UI evidence. The final ten-minute four-ORGANISM run with full load, background recording and audio-thread priority had no registered audio overruns or slow UI frames. Its conditions and limitations are recorded in the [0.5.1 performance report](../../docs/NATIVE-PERFORMANCE-051.en.md).

Current 0.5.2 contract: [structure and controls](../../design/current/native-v052-interaction.md). Current SONG and build tests: [0.5.2 verification](../../docs/NATIVE-VERIFICATION-052.en.md).
