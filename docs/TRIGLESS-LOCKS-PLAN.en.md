# LOCK ONLY and REPEAT · 0.6.5-dev

[Русский](TRIGLESS-LOCKS-PLAN.md) · **English**

Implemented in the experimental native RG35XX/Windows build. This replaces the earlier proposal; stable 0.5.2 and the browser prototype remain separate versions.

## Locks without a note

Place a LENGTH 16 note on step 1. On empty step 8, edit VOICE LOCKS or FX LOCKS with the usual held button + arrows. The event becomes **LOCK ONLY**, with an outlined diamond and LOCK. It never calls noteOn, restarts the envelope or extends LENGTH. NOTE / VELOCITY / LENGTH display `-- / NO NOTE`; PROB remains editable.

Each parameter retains its latest value until another event for that parameter or a new note. Unmentioned parameters survive. **`--` means no event; RESET returns to the track base value.** Unlocked fields on a new note start from the base.

To set an individual RESET, decrease a lock to its minimum and once more. Another decrease selects `--`; increasing from RESET selects the minimum. OPTIONS has **RESET CURRENT LOCK BANK** (return four fields to base) and **CLEAR CURRENT LOCK BANK** (remove four events). Removing the last lock/REPEAT returns LOCK ONLY to empty.

OPTIONS also offers **REMOVE NOTE / KEEP LOCKS**, **ADD NOTE / KEEP LOCKS**, and **CLEAR SELECTED STEP**. Short A toggles the note. Turning NOTE off keeps stored locks inactive; editing a lock on an empty step activates LOCK ONLY. Changes support UNDO, which stops transport.

## TIMING and REPEAT

CYCLE becomes **TIMING**, with four fields from left to right:

| RG35XX / Windows | Field | Scope |
|---|---|---|
| Y / Z | CYCLE | Track cell count |
| B / X | STEP | Track resolution 1/8–1/64 |
| A / C | REPEAT | Whole-track audio at the selected step |
| X / V | TYPE | EMPTY / NOTE / LOCK ONLY |

REPEAT selects **`--`, OFF, 1/8, 1/16, 1/32, 1/64**. These are fragment lengths, not repetition counts: at 120 BPM, 1/16 = 125 ms and 1/64 = 31.25 ms. The first fragment after the event plays live while being captured, then loops. The original voice continues underneath. Repeat runs **before track FX**, retaining delay/reverb tails. Short transitions at seams and when entering/leaving repeat reduce clicks.

Repeat lasts until OFF, a new note without its own REPEAT, stop or sequence change. It may outlast the original note. **`--` does not switch repeat off**: add a subsequent LOCK ONLY with REPEAT OFF. Another explicit value, even the same one, captures a new fragment; BPM changes also recapture. Probability is evaluated once per event, not on each audio loop.

**LOCK STUDY** demo: one LENGTH 16 note at 1/8 resolution; CUTOFF on step 8; REPEAT 1/16 on 9, 1/64 on 11, OFF on 13; CUTOFF RESET on 14; ORBIT lock on 15.

## Scope and limits

- GRANULAR: CUTOFF/LEVEL smoothly affect the cloud. SIZE/PITCH/SCATTER/STEREO/bounds/window affect future births; existing grains retain their settings. POSITION explicitly moves the head without restarting the note or active grains.
- MONO / ORGANISM / FM: targets change on the existing voice, preserving phases, envelope and note deadline.
- SAMPLER: VOICE LOCKS address the selected slot. Pitch, bounds and level change without resetting age; if new bounds exclude the current position, the one-shot ends instead of jumping to its start. FX/REPEAT address all slots on the track. A note in any slot resets shared FX/REPEAT but not voice locks on other slots. Slot-event and separate track-event probabilities are independent.
- Without an active voice, VOICE LOCKS creates no sound. FX LOCKS may change an existing tail. REPEAT captures silence when its pre-FX input is silent.
- STOP and instrument/sequence/SONG-slot changes clear temporary state. START after STOP begins at the start. Different cycle lengths and resolutions use the common 1/64 timeline.

## Files and verification

New saves use **MS11**; MOS5–MOS9 and MS10 remain readable. Old disabled steps with stored locks stay disabled and have no REPEAT event. Loading alone does not rewrite files. Builds 0.6.4 and older cannot open newly saved MS11 files. The experiment is installed separately after a backup.

REPEAT uses four fixed stereo buffers of 96000 int32 frames, about **2.93 MiB** total. Device fragments use at most one second per track. The audio path performs no file operations or allocations. Track/voice/FX limits are unchanged.

Functional checks compare positions, age, envelopes, note deadlines and trigger counters, plus RESET, probability, polyrhythms, SONG, FX tails, physical gestures and migration. [Hardware load report and binary hash](TRIGLESS-REPEAT-v1.en.md).
