# GRANULAR / LENS · experimental 0.6.0-dev

[Русский](GRANULAR-INSTRUMENT.md) · **English**

GRANULAR is MOSS's fifth native instrument, developed after the browser comparison of SPLICE / FIELD / LENS. The user selected **LENS** on 13 November 2026. One prepared sample becomes a cloud of up to **eight simultaneous grains per track**. The limits remain four tracks, up to two effects per track and two on the master.

This is an experimental extension. The main [0.5.2 guide](../native/prototype/README.en.md) describes the stable version; GRANULAR changes are documented here. Separate installation on an RG35XX with GarlicOS 1.4.9 and hardware checks are complete; see the [verification report](GRANULAR-NATIVE-v1.en.md) for conditions and results.

## First sound

On the prepared console, open **APPS → MOSS-GRANULAR → LENS STUDY**, then the LENS track and VOICE tab. **START** plays the sequence. The demo uses the copied AIR sample; the other sequences and library are also available as a separate copy.

1. Add a **GRANULAR** track, or open VOICE → L1+R1 → CHOOSE INSTRUMENT → GRANULAR.
2. Press **A** in an empty VOICE to assign a sample. OPTIONS → ASSIGN / REPLACE SAMPLE replaces it; REMOVE ASSIGNMENT clears it.
3. For your own WAV, select **+ IMPORT FROM DEVICE**, choose a file and a manual range in EDIT, then name and save it. The prepared WAV enters the library, is assigned to the cloud, and VOICE reopens. This reuses TAPE's preparation workflow; cancellation or failure retains the previous assignment.
4. Press **A** to audition: the gate stays open for two seconds, followed by the FADE release. To sequence it, enable a PATTERN trigger and press START.

GRANULAR has **one assignment per track**, separate from SAMPLER's four slots. A prepared PCM16 sample is limited to **1 MiB**. Instrument START/END do not reduce memory use; save a shorter file through TAPE/import to do that.

## Controls

Short **L1/R1** selects PLAY / SCAN / TEXTURE. The four fields follow **Y / B / A / X**, or **Z / X / C / V** on Windows. Hold the field's button and use arrows: ←/→ for fine changes, ↑/↓ for coarse changes. Releasing after editing does not fire the short action.

| Bank | Y | B | A | X |
|---|---|---|---|---|
| PLAY | SPEED −3.00…3.00× | PITCH −24…+24 semitones | SIZE 1–500 ms | DENSITY 0–128 grains/s |
| SCAN | POSITION 0–100% | START 0–99.9% | END 0.1–100% | JITTER 0–500 ms |
| TEXTURE | SPREAD 0–100% | IRREGULAR 0–100% | CUTOFF 20–14000 Hz | FADE 0–9000 ms |

SPEED moves the grain birth position through the source; negative values reverse this scan. PITCH changes grain reading speed independently. SIZE sets grain duration, and DENSITY sets the rate of birth attempts. When all eight voices are occupied, new attempts are skipped; high density does not enlarge the pool.

POSITION moves the head within START–END. During playback its field shows the current position; holding Y lets you edit it. START/END constrain **birth-position scanning**, not each grain's reading or the original WAV. JITTER offsets births; a grain can read outside that range and wraps across the full source. SPREAD controls stereo dispersion, IRREGULAR varies birth timing, CUTOFF controls the filter, and FADE sets the cloud's ASR attack and release.

Short **X** freezes the head (**FREEZE**); grains continue to be born and read while the gate is open. This is **V** on Windows. Short A remains audition, or sample assignment when empty.

## PATTERN and the lens

NOTE transposes the cloud relative to **C3**: C3 with PITCH 0 reads at the original pitch. LENGTH holds the gate, then FADE releases it. VOICE LOCKS exposes the same 12 parameters. **L1+arrows** selects the nested PLAY / SCAN / TEXTURE bank; ordinary L1/R1 still selects STEP / VOICE LOCKS / FX LOCKS / CYCLE. `--` inherits the base value. A POSITION lock explicitly seeks the head on its trigger.

The outer lens contour is a prepared overview of the actual source. A marker shows the head; expanding rings show grain age, and short arcs show reading. The UI receives atomic audio-engine snapshots and has no separate clock inventing movement. An empty source shows NO SAMPLE and A ASSIGN SAMPLE.

## Files, verification and limits

The experiment reads **MOS5/MOS6** and saves **MOS7**, including assignment, FREEZE, all 12 parameters and locks. Stable **0.5.2 cannot read MOS7**. Loading alone does not change the original; a subsequent save uses the new format. Work on copies of sequences and retain original MOS5/MOS6 files separately.

**MOSS GRANULAR** is installed separately alongside stable **MOSS 0.5.2**, with its own working data. Original projects, SONGs, WAVs, names and captures were backed up to the computer; the library was copied and the demo added as a new file. Original sequence and stable-binary checksums still match after installation. The two applications do not automatically synchronize data.

Windows regression and granular integration checks, UBSan and ARM functional checks passed. Across 23 minutes of hardware stress runs, no audio-budget overruns, late intervals or slow UI frames were recorded; the report lists durations, both binary hashes and verification limits. The DSP is an original implementation of conventional granular synthesis, not a port of Mangl code. Linear interpolation does not guarantee suppression of aliasing at large upward pitch shifts.

The [browser study](../visual%20prototype/granular.html) remains silent, with a synthetic source and its own storage. Its short **C / A** action toggles FREEZE; that is specific to the study. Native FREEZE is **X / V**. All three views remain available for comparison; LENS is the selected native view.

