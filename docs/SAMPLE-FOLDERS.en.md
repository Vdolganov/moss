# Samples, folders and four parts · MOSS 0.11.1

[Русский](SAMPLE-FOLDERS.md)

Open **TAPE → SOURCE → X IMPORT**, or **VOICE → OPTIONS → ASSIGN / REPLACE SAMPLE → IMPORT FROM DEVICE**. The browser lists folders and WAV files in the current directory.

- **A** opens a folder or WAV; **B** goes up, restoring the previous row. B at the root closes the browser.
- **X** auditions/stops the entire selected WAV. Large sources stream from storage.
- **R1 + left/right** jumps a page, both here and in the prepared sample library.
- Roots are `imports`, `captures` and shared `MOSS-SampleLibrary` next to `Roms`. Up to 512 entries per directory and 16 folder levels; the screen reports a full directory listing.

## Four-part slicing

With a WAV open, use **L1+R1 → SPLIT INTO FOUR PARTS**. Parts start consecutively without gaps. A source fitting four 1 MiB PCM16 parts is divided equally to frame precision; a longer source starts with four adjacent 1 MiB parts, leaving the remainder outside the selection. This does not detect transients.

| Action | Controls |
|---|---|
| Audition part 1 / 2 / 3 / 4 | Tap **Y / B / A / X** |
| Stop the audition | Tap the same button again |
| Switch directly to another part | Tap its button |
| Move a range, keeping its length | Hold its button + **left/right**, 1 ms |
| Coarse movement | Hold its button + **up/down**, 100 ms |
| Select a part silently | Arrows |
| Switch MOVE / TRIM / FINE | Tap **L1 / R1** |
| Adjust START / END in FINE | **Y / B + left/right**: one source frame; **up/down**: ten |
| Zoom / pan FINE | **A / X + arrows** |
| Join ranges again | **L1+R1 → ARRANGE PARTS IN ORDER** |
| Edit the selected range in TRIM | Hold **Y START / B END / A FADE IN / X FADE OUT** + arrows |
| Save the set | **L1+R1 → NAME + SAVE FOLDER** |

Holding an arrow accelerates edits. Releasing after an edit never auditions accidentally. In TRIM and FINE, face-button taps still audition their corresponding four parts; the heading identifies the edited part. Editing stops the preview, and the next audition uses the new boundaries. Ranges may overlap or appear out of time order: their numbers identify destination slots.

The character panel names a folder containing `01.wav`–`04.wav` and a `set.moss` manifest. Existing folders are never overwritten. Failed saves retain the edit and keep incomplete sets out of the library. **SELECT** exits; when importing from VOICE it returns without assigning. The existing manual START/END workflow remains available.

## Loading a set

**A** opens a prepared folder. **Y** on the folder or inside it loads all four files into slots **1–4 of the current SAMPLER track**. This action does not replace a synth or GRANULAR instrument. For a single sample assignment, open the folder and select a file with **A**; **X** auditions it.

Loading replaces the four slot assignments/settings, retaining track steps and effects. OPTIONS → UNDO LAST EDIT restores the previous slots. An unreadable file leaves assignments unchanged. Slicing from SAMPLER offers **NAME FOLDER + ASSIGN FOUR** to save and assign in one action.

## Storage and memory

The catalog holds 256 prepared samples, including set members. MS17 sequences store persistent IDs; MOS5–MS16 remain readable. Updating does not rewrite old projects, WAVs or names. New sequences using IDs above 31 require MOSS 0.11.

PCM stays in a fixed **32 × 1 MiB** resident cache plus the existing 1 MiB scratch buffer. Loading a sequence or assigning samples prepares the required files with audio paused. The audio callback only reads prepared PCM views; it performs no file I/O. SONG preloads the union of its sample references and refuses more than 32 unique samples before playback. Sequence transitions do not read the card.

Windows keys: physical **Y/B/A/X** map to **Z/X/C/V**; L1/R1 map to **Q/E**. See the [manual](../native/prototype/README.en.md) for other controls.

## Precision, waveform and level · 0.11.1

Four colored Y/B/A/X ranges now overlay actual signed WAV waveforms, with separate stereo channels and a stronger fill/outline for the selected part. L1/R1 cycles SOURCE / MOVE / TRIM / FINE / LIBRARY; a VOICE import offers MOVE / TRIM / FINE. The earlier L1+arrow mode switch is removed.

FINE zooms into the selected part. Y START / B END move by one source frame left/right or ten up/down; A ZOOM changes magnification, X VIEW pans the window. Boundaries follow into view while editing. Exact frame positions survive preview and WAV export. Short Y/B/A/X taps still audition their corresponding parts in every mode.

Initial parts are contiguous from the source start: four equal frame ranges when the source fits, otherwise four consecutive 1 MiB PCM16 ranges. The remaining source stays outside the selection. OPTIONS → ARRANGE PARTS IN ORDER preserves lengths/fades and joins parts in Y/B/A/X order from the first start. The set shifts left if needed to fit. If their combined lengths exceed the source, nothing changes and the editor asks you to trim them.

PCM16 cuts without fades preserve exact sample values. Source and prepared-file auditions now use the same unity gain. MASTER capture records the audible project output after monitor gain/peak protection, excluding auditions. Track capture remains after track effects/fader and before master. Existing WAVs are unchanged. Sampler-track playback still applies velocity, slot/track levels and the master.
