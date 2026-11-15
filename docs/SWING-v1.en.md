# SWING / SHIFT — 0.6.6-dev

[Русский](SWING-v1.md) · **English**

Each instrument track has its own **SWING**. **SHIFT** is a per-step timing lock: the note, voice/FX locks and REPEAT move together. Sequence BPM and cycle duration do not change.

## Controls

In PATTERN, select TIMING with short **L1/R1** presses. **L1 + right** opens page **2/2**; L1 + left returns to 1/2. On Windows, use Q/E and Q + arrows.

| Field | Anbernic | Windows | Range |
|---|---|---|---|
| SWING | Y + arrows | Z + arrows | 50–75%, whole track |
| SHIFT | B + arrows | X + arrows | −50…+50% of one straight step, selected step only |
| REPEAT | A + arrows | C + arrows | Existing -- / OFF / 1/8…1/64 |
| TYPE | X + arrows | V + arrows | EMPTY / NOTE / LOCK |

Left/right changes one percentage point; up/down changes ten. Holding accelerates repetition. Page 1/2 retains CYCLE / STEP / REPEAT / TYPE. Releasing a used modifier never triggers its short action. SAMPLER slot selection with L1 + arrows remains available in the other PATTERN banks; in TIMING the gesture selects its page.

## Musical behavior

- **50% SWING** is straight, **66%** approaches triplet swing, **75%** gives a 3:1 ratio. Every second cell (2, 4, 6…) is delayed without lengthening the pair.
- Swing follows the track's STEP resolution: 1/8, 1/16, 1/32 or 1/64. Changing STEP changes the pair size while retaining musical CYCLE duration. Events hidden between visible cells follow a continuous pair time warp; their stored positions are neither removed nor quantized.
- **SHIFT −15%** advances the step by 15% of a straight step relative to its swung position; **+15%** delays it. Strong beats can be shifted too. At 120 BPM with STEP 1/16, a straight step lasts 125 ms, so SHIFT +10% adds 12.5 ms.
- **--** and **0%** add no shift. SHIFT never carries over to another step. **CLEAR STEP SHIFT** in the TIMING 2/2 menu clears only this lock. Existing UNDO restores the last gesture.
- SHIFT on an empty step stores timing metadata; it creates neither a note nor a LOCK ONLY event. Add a note or musical lock. A grid dot marks the stored step setting.
- SAMPLER shares one SHIFT across all four slots at that position and their common FX/REPEAT. Independent per-slot shifts are not included.

## Sustains and boundaries

LENGTH starts at the actual note onset. Note-less locks neither retrigger the voice/envelope nor extend LENGTH. Short samples remain one-shots; FX tails continue processing.

SHIFT may reorder events or make them coincide. Each event executes once; ties within a track use original position order. Probability stays tied to the original step identity, not its shifted execution time.

Only the initial START clips negative timestamps to zero. On subsequent cycles an early first step anticipates the wrap. Odd cycle lengths do not drift: swing pairs restart from the track's origin. Positively shifted events may spill past its cycle boundary.

SONG preserves exact slot lengths: no early next-slot events, and any pending old-slot events are discarded at the transition. STOP and REWIND clear the schedule. Editing timing never replays an already executed event; pending events are rescheduled. If an edit moves one into the past, it executes on the next audio frame. BPM changes preserve musical event and gate deadlines.

## Storage and implementation

New saves use **MS12**. MS11 and earlier supported versions load with SWING 50% and SHIFT --. Loading does not rewrite the source. Saving as MS12 requires 0.6.6 or later to reopen; older builds cannot read it. Sequence/track duplication and UNDO include timing values.

The queue has 16 fixed entries per track with four canonical ticks of lookahead and at most eight ticks of delay. No added allocation or file I/O in the audio thread. The sequence clock stays straight; events and gate endings have individual sample-accurate deadlines. Limits remain four tracks, two FX per track plus two master FX, and eight grains per GRANULAR instance.

## Verification

 Functional coverage includes exact timestamps across tempos/sample rates, all resolutions and five instruments, early/late events, odd cycles, NOTE/LOCK ONLY/REPEAT, LENGTH, STOP/START, live edits, SONG, controls and old-project loading.

**14 hardware runs / 560 seconds / 17514 audio callbacks:** zero audio-budget overruns, late intervals, stream underruns, slow UI frames or schedule overflows in these stress runs. Maximum **29.236 ms** within the 32 ms budget; worst p99 upper bound **24.50 ms**.

Original RG35XX / GarlicOS 1.4.9, stock 1.008 GHz, 32000 Hz / 1024 frames. Four maximum-length tracks, independent 1/8–1/64 resolution and 58–75% SWING, dense −50…+50% SHIFT, locks and all REPEAT divisions. All ten legal FX pairs enabled on every track and master; 32 MiB PCM / 32 stereo 96 kHz sources, eight 8-second grains per track or four sampler slots each, mixed synths, live UI, SD resampling and SONG. Output muted after full DSP. This qualifies these measured cases, not every possible signal.

| Profile | FX | Seconds | Mean ms | p99 ≤ ms | Max ms |
|---|---|---:|---:|---:|---:|
| 4 GRANULAR | POND+DRIVE | 20 | 21.654 | 24.25 | 27.216 |
| 4 GRANULAR | ORBIT+DRIVE | 20 | 20.845 | 23.50 | 24.786 |
| 4 GRANULAR | REEL+DRIVE | 20 | 19.621 | 22.25 | 23.040 |
| 4 GRANULAR | ORBIT+POND | 20 | 20.872 | 23.75 | 24.587 |
| 4 GRANULAR | POND+REEL | 20 | 19.421 | 22.25 | 23.696 |
| 4 GRANULAR | ORBIT+CRUSH | 20 | 18.407 | 21.25 | 22.383 |
| 4 GRANULAR | ORBIT+REEL | 20 | 18.328 | 20.75 | 21.248 |
| 4 GRANULAR | POND+CRUSH | 20 | 18.949 | 21.75 | 22.629 |
| 4 GRANULAR | CRUSH+REEL | 20 | 16.512 | 19.25 | 20.445 |
| 4 GRANULAR | CRUSH+DRIVE | 20 | 17.957 | 20.25 | 21.008 |
| 4 GRANULAR | POND+DRIVE | 180 | 23.010 | 24.50 | 29.236 |
| 4 SAMPLER / 16 slots | POND+DRIVE | 60 | 17.078 | 18.75 | 19.747 |
| GRANULAR + 3 synths | POND+DRIVE | 60 | 21.224 | 22.75 | 23.378 |
| SONG transitions | ALL | 60 | 15.364 | 20.50 | 24.674 |

14 host suites, targeted UBSan and the ARM functional suite passed. An independent timestamp calculation exactly matched timing-event, note-trigger, LOCK ONLY and REPEAT counts in all non-SONG hardware runs. Every four-granular stress run reached eight grains on each track. The first ARM test launcher failed because a large temporary SONG fixture exceeded its stack; the test fixture was corrected without changing the qualified application binary, then the complete ARM suite passed.

The installed binary was also opened with an isolated copy of LOCK STUDY. Physical gestures verified TIMING 1/2 ↔ 2/2, SWING 61%, NOTE SHIFT −11%, LOCK ONLY SHIFT +10%, fine/coarse increments and normal MENU exit. Loading the saved MS12 file confirmed these exact values. This separate session includes loading/saving and recorded 1 late audio interval and 2 slow UI frames (maximum 302.916 ms), with no audio-budget overruns or stream underruns. Their precise causes were not isolated; these counters are not part of the clean stress-run totals.

Four original sequences and 25 prepared samples remained byte-identical. Stable MOSS was unchanged. The experimental 0.6.6 binary is installed as MOSS GRANULAR; the device exited to Garlic and restored the interactive governor.

[Binary hash, counters and raw reports](verification/swing-v1/manifest.json).
