# GRANULAR / LENS · 0.8.0-dev

0.8.0 adds CLOUD: reverse, detune, size variation and birth jitter. MS14 saves. [Research, controls and verification](AMBIENT-RESEARCH-v08.en.md).

[Русский](GRANULAR-INSTRUMENT.md) · **English**

One prepared sample, up to eight simultaneous grains per track. Four tracks, two effects per track and two on the master. This separate **MOSS-GRANULAR** experiment leaves stable MOSS 0.5.2 separate.

## First cloud

Open APPS → MOSS-GRANULAR → LENS CLOUD → LENS → VOICE. START plays the sequence. To assign your own source, use L1+R1 → ASSIGN / REPLACE SAMPLE → a prepared sample or + IMPORT FROM DEVICE. Import reuses TAPE's manual range selection, naming and save; cancelling retains the old assignment.

GRANULAR has one assignment, independent of SAMPLER's four slots. Prepared PCM16 WAV files remain limited to **1 MiB**. VOICE bounds do not save memory; save a shorter file through TAPE for that.

## Controls

L1/R1 cycle five banks. Fields follow **Y / B / A / X**, mapped to Z / X / C / V on Windows. Hold the field's button and use ←/→ for fine or ↑/↓ for coarse changes. Short A auditions for twice SIZE plus NOTE IN, clamped to 2–20 seconds, followed by NOTE OUT; without a source it opens assignment. Short X (Windows V) freezes the scan anchor.

| Bank | Y | B | A | X |
|---|---|---|---|---|
| PLAY | SPEED −3…3× | PITCH ±24 semitones | SIZE 1–8000 ms | DENSITY 0–128/s |
| SCAN | POSITION 0–100% | START 0–99.9% | END 0.1–100% | SCATTER 0–100% |
| TEXTURE | STEREO 0–100% | JUMP 0–100% | CUTOFF 20–14000 Hz | LEVEL 0–200% |
| ENVELOPE | GRAIN IN 0–100% | GRAIN OUT 0–100% | NOTE IN 0–9000 ms | NOTE OUT 0–9000 ms |
| CLOUD | REVERSE 0–100% | DETUNE ±0–100 cents | SIZE VAR ±0–75% | JITTER 0–100% |

**SCATTER 100%** distributes births over the complete selected region. Eight areas are chosen in random order, prioritizing unoccupied areas; positions within each are random. A full cloud no longer clusters around one playhead. Lower density can produce fewer than eight simultaneous grains. This distributes birth locations, not permanently stationary playback cursors: each grain continues reading at its own pitch.

**SCATTER 0%** births grains exactly at the scan anchor. Intermediate values set a window width as a fraction of START–END, centered on the anchor and wrapping inside that range. SPEED moves the anchor; POSITION seeks it; FREEZE holds it. At 100%, the head position, SPEED and JUMP do not bias births: the cloud covers the whole region. The head remains visible; speed/position fields say SCAN ANCHOR.

START/END constrain both births and reading of new grains. Existing grains finish with their latched bounds. PITCH controls read speed separately from SPEED. STEREO assigns each grain its own fixed balance, replacing whole-cloud panning. **JUMP** replaces IRREGULAR and controls random head displacement width relative to the selected region. At 0% there are no jumps; at 100% any position within START–END is possible. Random jump intervals are 0.75–1.75 seconds, with SPEED continuing between jumps. FREEZE holds both continuous motion and jumps, while grains keep playing. Head changes affect future births; existing grains retain their reading. Head randomness is independent of grain-distribution randomness.

**SIZE now reaches 8 seconds.** Actual birth rate is capped at `min(DENSITY, 8 / min(8, SIZE_in_seconds × (1 + SIZE_VAR/100)))` to distribute voices evenly through time. For example, SIZE 8 s allows at most one new grain per second. Actual G/S is shown left of LENS. Density never grows the pool or steals an active grain; existing grains retain their lengths after edits. Retriggering an active cloud preserves its birth-scheduler phase. Reading wraps from END to START, so longer SIZE needs no larger PCM buffer.

**GRAIN IN / OUT** independently set raised-cosine attack and release as percentages of SIZE, with a plateau in the remaining interval. For SIZE 400 ms, IN 10%, OUT 60%, that means 40 ms attack, 120 ms plateau and 240 ms release. Increasing one fade beyond the available space reduces the other. Conflicting independent locks are normalized proportionally to 100%. Even a nominal zero fade retains one zero endpoint sample. New settings apply to newly born grains; active windows are not cut.

**NOTE IN / OUT** control the entire cloud's gate envelope. Step LENGTH holds the note, then NOTE OUT releases it. CUTOFF and LEVEL change smoothly. Gain compensates for expected overlap and window shape instead of always dividing by eight; it does not inspect input loudness and is not a compressor. LEVEL matches different sources. Application monitor gain increases from 135% to 180% (+33% relative to the previous version), retaining the soft peak limiter. Monitor gain is not repeatedly baked into internal resampling.

## PATTERN and LENS

C3 with PITCH 0 reads at original pitch. VOICE LOCKS exposes all 20 parameters. L1+arrows chooses its nested bank; L1/R1 still navigates STEP / VOICE LOCKS / FX LOCKS / TIMING. `--` adds no parameter event; RESET returns to the base. A POSITION event explicitly seeks the anchor.

The circle consists of actual sample peaks in absolute source coordinates. The waveform outside START–END is dimmed, with boundary ticks. Each grain arc represents its source extent, accounting for length, pitch and END-to-START wrapping; a solid square follows its actual read position. Arc brightness follows the grain envelope. Each arc retains its lane throughout the grain lifetime. Grains spanning a complete region show its full arc and LOOP GRAINS; their read squares make repeated passes. The outlined square outside the waveform is the scan head. Existing grains retain their old bounds until they finish. Snapshots come from the audio engine, with no independent animation clock. Missing sources show NO SAMPLE / A ASSIGN SAMPLE.

**LOCK ONLY and audio REPEAT are implemented in 0.6.5.** Existing notes keep their gate/envelope. Controls and lifetime rules are in [TRIGLESS-LOCKS-PLAN.en.md](TRIGLESS-LOCKS-PLAN.en.md).

## Files and verification

New saves use **MS14**, including 20 granular parameters and eight POND parameters with locks. MOS5…MS13 remain readable. Added CLOUD fields become zero and added locks inherit; live grains retain their direction, length and pitch. New POND changes the sound of older presets. Opening a file does not overwrite it. Preserve the original files: older MOSS versions cannot read MS14. Historical IRREGULAR/JITTER migration still applies to pre-0.6 files; it is separate from the new CLOUD JITTER.

Cloud 0.6.1 verification history: [GRANULAR-CLOUD-v2.en.md](GRANULAR-CLOUD-v2.en.md). [GRANULAR-NATIVE-v1.en.md](GRANULAR-NATIVE-v1.en.md) covers only 0.6.0-dev. Linear interpolation can still alias at large upward transpositions. This is original DSP, not a Mangl source port.

The [browser study](../visual%20prototype/granular.html) remains silent and synthetic. It mirrors four banks and cloud distribution, but cannot validate audio. Its short C / A FREEZE intentionally differs from native X / V.

New DRIVE/ORBIT, MOS9 and 0.6.2 verification: [FX-CHARACTER-v1.en.md](FX-CHARACTER-v1.en.md).

Current long-pad, JUMP and LENS verification: [AMBIENT-LENS-v1.en.md](AMBIENT-LENS-v1.en.md). Start with SIZE 3200 ms, SPEED 0.12×, JUMP 75%, SCATTER 40%, GRAIN IN/OUT 50/50 and a sufficiently long note LENGTH. SIZE does not replace LENGTH: short notes still release the whole cloud through NOTE OUT.
