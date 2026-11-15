# REEL 0.7.0

[Русский](REEL-v1.md) · **English**

The complete REEL is integrated into the working GRANULAR branch. It is an original tape model: saturation, independent wow/flutter, drift and stereo offset, adjustable high-frequency loss, occasional smooth dropouts and noise. No Elektron, RC-20 or GrainBrick algorithms were copied.

## Controls

SPACE → REEL → A. Tap **L1/R1** to change pages. Hold the corresponding **Y/B/A/X** button and press left/right for fine changes, up/down for coarse changes. Physical order and colors are unchanged.

| Page | Y | B | A | X |
|---|---|---|---|---|
| COLOR | DRIVE 0–100% | TONE −6…+6 dB | BUMP 0…+6 dB | AMOUNT 0–100% |
| MOTION | WOW 0–15 cents | WOW RATE 0.10–4 Hz | FLUTTER 0–15 cents | FLT RATE 2–30 Hz |
| LIFE | DRIFT 0–100% | STEREO 0–100% | LOSS 0–100% | DROP 0–100% |
| OUTPUT | HISS OFF / −72…−24 dBFS | HISS TONE −100…100 | FOLLOW 0–100% | OUTPUT −12…+6 dB |

TONE is a high shelf; BUMP is broad emphasis around 80 Hz. LOSS separately introduces high-frequency loss; zero does not impose a low-pass filter. DRIVE adds density and complements the separate DRIVE distortion effect. DRIFT slowly moves the read head, STEREO separates channel read positions without polarity inversion, and DROP introduces occasional smooth attenuation without cutting off notes.

HISS specifies white-noise RMS at HISS TONE=0, FOLLOW=0, OUTPUT=0 dB and AMOUNT=100%. FOLLOW=100% follows the signal envelope; FOLLOW=0 produces constant noise while PLAY runs. STOP fades noise away. OUTPUT trims the complete result, including noise. AMOUNT scales the model's character rather than mixing misaligned signals. At zero, after the transition, REEL is an exact neutral path with fixed latency.

**L1+R1 → REEL PRESET** offers CLEAN, STUDIO, WARM CASSETTE, WIDE DRIFT and WORN LOOP. A preset updates all 16 base values in one undoable action, preserving bypass and locks. X in the effect list enables the effect. The existing limit remains two simultaneous effects per track and master.

Within **FX LOCKS**, L1+left/right selects the effect and L1+up/down selects its page. All 16 parameters support locks, including LOCK ONLY during sustained notes. CLEAR CURRENT LOCK BANK affects only the four visible values. `--` inherits the previous state; `RESET` restores the base value. Numeric locks retain physical units.

## Display and signal path

The reels show transport and slow speed changes. The tape carries 32 measured output-envelope points at the explicitly labelled fixed X4 scale. This is not an invented waveform. The speed display contains audio-block minimum/maximum history and numeric cents, so fast flutter is not misrepresented as slow rotation. STOP freezes the reels; BYPASS dims the drawing. The UI reads a bounded atomic snapshot, never the audio buffers.

At 32 kHz each track reserves 512 frames / 16 ms and the master adds the same, including bypass. Tracks remain aligned and AMOUNT does not introduce dry/wet comb filtering. Library and streamed WAV monitoring use the same total 32 ms. Resampling skips this initial offset and captures the corresponding end of the window: a saved bar does not accumulate another 16/32 ms when reused. Manual finish waits for that short remainder. Wow/flutter's variable displacement remains part of the effect.

## Data and implementation

New saves use **MS13**. MOS5…MS12 load; the old four REEL controls and locks are approximately mapped to the new model. Exact legacy sound is not reproduced. Opening an old sequence with active REEL displays a notice. Loading does not rewrite the original; saving writes MS13, which older builds cannot read. The prior binary, projects, sample names and WAV library were backed up on the computer before installation.

Locks use actual parameter counts: ORBIT 8 + POND 4 + CRUSH 4 + REEL 16 + DRIVE 8 = 40 values / 80 bytes per step, unchanged. REEL is separated from FxState; color uses 2× oversampling and ADAA, and moving heads use 12-tap interpolation with ARM NEON. Slow random processes update every 16 frames with smooth transitions between updates. Track rendering groups up to 128 frames, always stopping before musical events.

## Verification

Host checks cover neutrality, frequency response, saturation alias rejection, stereo FIR, calibrated noise and STOP, AMOUNT/bypass transitions, all locks, presets, MS13/legacy files, resampling and callback-size-independent audio. Windows and ARM share the implementation. [UI evidence](design/native-reel.md).

A **175-second** series ran on the original RG35XX / GarlicOS 1.4.9 at stock clock: four maximum-length tracks, 32 MiB PCM, up to 32 grains or 16 sampler voices, two effects per track and master, five simultaneous REEL instances, locks/SWING/SHIFT/REPEAT, SONG, SD recording and visible animation. **5481 callbacks; zero deadline overruns, late intervals, stream underruns or scheduling overflows.** Worst callback: 29.726 ms out of 32; heavy POND+REEL p99: 29 ms. [Raw reports](verification/reel-v1/v6/manifest.json).

The additional headroom target of p99≤24 ms / max≤28 ms is not yet met in the heaviest cases. This is a short qualification, not an indefinite-run guarantee. After resampling alignment the final binary ran for another 50 seconds: 1566 callbacks, zero faults, maximum 29.587 ms. [Final verification](verification/reel-v1/v7/manifest.json). No twenty-minute soak was run.

A separate device interaction session verified all four pages, fine/coarse changes, preset selection, PLAY/STOP and normal exit. Its [report](verification/reel-v1/ui-report.json) has no processing overruns or stream underruns, but records one late callback interval. The counter includes intentional audio pauses when opening a sequence; this interval has no timestamp, so it cannot conclusively be attributed to loading. This session is not counted as a clean load run. Version 0.7.0 is installed and running; user projects and the library were compared against their backup.
