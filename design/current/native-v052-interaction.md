# Native MOSS 0.5.2 — sequences and SONG

Extension of native-v051-interaction.md. The native 640×480 graphite style, typography, four button colors, four-track/two-active-effects budget and audio quality remain unchanged. Browser prototype is a separate reference, not updated by this implementation.

## Library and hierarchy

ATLAS has two visible tabs, SEQUENCES and SONGS, selected with L1/R1. D-pad selects a row. A opens it. Each list has a permanent creation row. Existing MOS6 projects are called sequences in the interface; their files and contents remain valid.

New sequence opens a source picker: EMPTY or an existing sequence. Choosing a source then naming it creates an independent copy of all tracks, steps, locks and effects. Sample assets remain shared by reference. Cancel creates no file.

SONG contains an initially empty vertical ordered list, at most 64 slots. A slot references one of at most 32 saved sequences and has REPEATS 1–64 and BARS 1–64. All bars are 4/4. One common SONG BPM overrides individual sequence BPM during arrangement playback; individual sequence files keep their own BPM. LOOP controls the complete arrangement. There are at most 32 saved SONGs.

Default bars = ceil(longest track cycle / 64 canonical ticks), minimum one. A 15/16-step pair at sixteenth resolution defaults to one bar; no least-common-multiple expansion. Changing a sequence later does not silently change an existing slot's explicit duration. Replacing a slot resets repeat to one and derives a fresh default duration.

## Arrangement interaction

- D-pad selects a row; R1 + left/right moves four rows. Selection and playback marker are distinct.
- A on + ADD opens a sequence picker, including + NEW SEQUENCE. A on an existing row opens its sequence. B from its GRID saves and returns to that SONG row. Short SELECT also returns to SONG; from SONG it returns to the library.
- X on a row replaces its sequence.
- Four bottom fields are Y POSITION, B REPEATS, A BARS, X LOOP. Hold Y + up/down to move a row, following the existing effect-chain reorder pattern. B/A + left/right adjusts one; up/down changes repeats by ten or bars by four. LOOP is boolean.
- START plays from the selected row (from the last row if selection is + ADD), or stops. At end, LOOP OFF stops note generation and allows effect tails to decay; LOOP ON returns to slot one.
- L1+R1 contains SAVE SONG, UNDO SONG EDIT, RENAME SONG, DUPLICATE SLOT, REMOVE SLOT, COPY SEQUENCE AS NEW as applicable. Copy-as-new replaces the selected slot with the independent copy and opens it. Duplicate-slot keeps the shared sequence reference.
- Structural editing is locked during playback with “START STOP / THEN EDIT SONG”; scrolling and common BPM remain available. Playback uses an immutable prepared snapshot. Stop/start rebuilds it from saved sequences.
- Repeats extend the same slot's duration continuously: 15/16 cycles keep their phase through repeat boundaries. Entering the next slot, even when it references the same sequence, restarts local phase at zero. Transport fractional timing is preserved.

## Consistent parameter and page gestures

Every numeric field uses left/right for one displayed unit; up/down for a larger musical step. Defaults: ten for percentages/levels/velocity/BPM, 100 for Hz and milliseconds, 12 for pitch, 16 cells for hit LENGTH and track cycle. Enum/boolean choices use their next legal value; ORBIT sync time and FM ratio remain one choice per press. CRUSH bits/rate use coarse increments two/four. Capture bars use four. Holding still accelerates repetition. Locks use the same increments as their source parameters.

R1 + left/right jumps 16 cells in PATTERN, preserving position within the page and clamping at a partial last page. Short L1/R1 still changes parameter bank. R1 + left/right also pages lists and menus. Modifier release must not select a bank or move a step unexpectedly. Menu/list navigation takes precedence over the underlying grid. MIX always shows LEVELS / MASTER FX sub-tabs.

## Storage, recovery and real-time boundary

SON1 is a separate checksummed format under projects/song-N.moss; sequence MOS6 format is unchanged. Writes use temporary files, flush and atomic replacement. Failed rename/create/save retains the name/edit for retry. Missing or corrupt sequence rows are labelled and replaceable; START preflights every sequence and assigned sample, refusing partial playback.

Deleting a slot or SONG does not delete sequences or samples. A sequence referenced by a saved SONG or current SONG undo is protected against deletion and its missing ID is reserved against accidental reuse. Canceling the name editor creates nothing. Song undo covers slot edits/reorder/repeats/bars/loop/tempo.

All unique sequence snapshots are prepared with audio paused before playback, bounded at 32 × sizeof(Project), about 12.1 MiB. No reads, writes, allocation, locks or bulk effect-buffer resets occur at a slot boundary. Existing effect histories continue in corresponding racks; old notes end and a 128-frame output ramp limits discontinuity when changing instruments/routes. This is a single-engine transition, not overlapping playback of two full arrangements. Tracks absent in the incoming sequence invalidate their effect histories.

## Verification

v052_test exercises real button gestures and release order, partial pages, fine/coarse values and locks, empty creation, independent copy, row movement, reference protection, checksum errors, save retry, missing references, sample-exact bar boundaries, polyrhythm phase, stop/loop/common BPM and block-size-invariant transitions. Hardware profile kind 4 uses 32 unique maximum-density sequences in 64 slots, switches once per second at 240 BPM through all instrument and effect-pair types, keeps 32 MiB samples loaded, captures audio and renders the SONG list. Performance evidence is reported separately from visual review.
