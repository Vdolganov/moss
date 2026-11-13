# Locks without a new note — proposal

[Русский](TRIGLESS-LOCKS-PLAN.md) · **English**

Status: interaction proposal, not implemented in 0.6.1-dev.

Example: a note starts on step 1 with LENGTH 16. Step 8 changes CUTOFF without restarting the note, envelope or read position. Step 12 changes an effect. The original LENGTH still determines note-off.

Use three explicit states: empty, NOTE and LOCK ONLY. Filled markers indicate notes; hollow diamonds indicate parameter events. Probability may remain available, while NOTE, VELOCITY and LENGTH are unavailable for LOCK ONLY. One event may change several voice and effect parameters.

Editing VOICE LOCKS or FX LOCKS on an empty step creates LOCK ONLY. Removing its final lock restores an empty step. Context actions: “Remove note, keep locks”, “Add note” and “Clear step”, with normal undo. Simply disabling an existing note must not silently activate its hidden locks: NOTE → LOCK ONLY remains an explicit conversion.

A lock persists until the next event for that parameter or the next note. Unmentioned parameters remain unchanged. RESET explicitly restores one parameter's track value; it must differ from `--`, which means no event. Stop and sequence changes clear temporary values. Voice events without an active note do not generate sound; effect events may still shape an existing tail.

For GRANULAR, CUTOFF and LEVEL smoothly affect the cloud. SIZE, PITCH, SCATTER, STEREO, bounds and GRAIN IN/OUT affect subsequent births; existing grains retain their windows, reading and pan. DENSITY changes future births without resetting scheduler phase. None of these events calls noteOn again.

Before implementation, test polyrhythms, independent step durations, pause/resume, SONG transitions, skipped probability triggers, FX tails, no active voice, and persistence of RESET separately from absent locks. SAMPLER needs a distinct targeting decision: voice locks target the selected sample lane, FX locks target the whole track.
