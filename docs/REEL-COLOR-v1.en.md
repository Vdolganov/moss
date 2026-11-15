# REEL: saturation, colour and processing cost

[Русский](REEL-COLOR-v1.md)

This continues the [clean path and WOW/FLUTTER stage](REEL-MOTION-v1.en.md).
It is an isolated audio candidate behind `MOSS_REEL_MOTION_LAB`. It does not
replace the installed MOSS or load user projects using the experimental control
layout. Visualization, final control pages, project migration and preview latency
alignment remain integration work.

## Added controls

| Control | Range | Behaviour |
|---|---:|---|
| DRIVE | 0–100% | Transparent audio through soft, dense saturation; up to +9.5 dB internal drive |
| TONE | −6…+6 dB | Independent high shelf centred around 3 kHz |
| BUMP | 0…+6 dB | Broad boost around 80 Hz, Q=0.7 |
| AMOUNT | 0–100% | Scales colour and tape-motion depth |

WOW, WOW RATE, FLUTTER and FLUTTER RATE remain available. The laboratory carrier
uses slots 5/6/7 for DRIVE/TONE/BUMP, including parameter locks. This is a temporary
carrier layout, not the final COLOR/MOTION page design. HISS, DROP, LOSS and the
remaining life/output controls have not been added yet.

Saturation uses an original, symmetric quintic curve with a continuous second
derivative. It is a colour model, not a physical simulation of a particular tape
or its hysteresis. Reversible high-frequency emphasis precedes saturation and its
exact linear inverse follows it. There is no mandatory bass-cut filter. A fixed
reference-amplitude gain correction is used; there is no automatic gain control.

## Signal path and latency

`input → pre-emphasis → 2× + ADAA → de-emphasis → WOW/FLUTTER → BUMP → TONE`

The divided difference of the shaper's antiderivative reduces aliasing, with a
stable polynomial expression for neighbouring inputs. ADAA's inherent smoothing
is compensated by a short FIR rather than left as a hidden treble roll-off.
Relevant background: [DAFx 2019](https://www.hsu-hh.de/ant/wp-content/uploads/sites/699/2020/10/DAFx2019_paper_4.pdf)
and [DAFx 2024](https://dafx.de/paper-archive/2024/papers/DAFx24_paper_33.pdf).
The TONE shelf follows the [Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook/).

The two 23-tap halfbands plus ADAA correction consume 12 samples. These fit
inside REEL's existing 512-sample reservation at 32 kHz; the total track-plus-master
latency remains 32 ms. With saturation off, two FIFOs collapse into one. Enabling
it transfers history in bounded work, retaining the same read position and
continuous WOW/FLUTTER phases. At the auxiliary 8 kHz test rate, latency is
increased to retain safe history margins at maximum motion depth. The device's
working rate is 32 kHz.

Neutral controls produce an exact delayed float copy, including headroom above
0 dBFS. Colour edits use finite 20 ms linear ramps. Preparing unchanged controls
does not restart an existing ramp.

## Reducing processing cost

- The neutral FIFO is separate from the heavy motion interpolation kernel.
- Colour state occupies 1296 bytes per instance. Mirrored rings provide contiguous
  FIR input; the ARM implementation uses NEON with correct D-register clobbers.
- Motion locks do not rebuild unchanged colour coefficients.
- Laboratory POND history rows are offset across cache sets; its formulas, line
  lengths and arithmetic order remain unchanged.
- The engine renders one track in spans of up to 32 samples, always ending before
  the next tick, note, gate deadline or lock. This improves buffer locality without
  adding audio latency.

Span rendering is compared with single-sample rendering: PCM, internal capture
and event counts match. Host checks cover 12 instrument/effect combinations with
1/31/1024-frame calls; ARM checks cover three instrument configurations with
REEL+POND. SWING, SHIFT, LOCK ONLY, REPEAT and SONG transitions are checked separately.

## Audio measurements

A dense frequency grid bounds the saturation path's linear-response error below
0.25 dB through 12 kHz. BUMP measures +6 dB at 80 Hz; TONE approaches ±6 dB at
12 kHz. With saturation enabled, antialiasing filters attenuate the near-Nyquist
region; a flat response above 12 kHz is not claimed. Neutral bypass is exact.

Maximum-DRIVE sine probes at peak amplitude 0.3 compare the same shaper, gain and
pre/de-emphasis at base rate and through the new antialiasing path:

| Frequency | New-path aliasing relative to fundamental | Reduction versus 1× |
|---:|---:|---:|
| 3991 Hz | −67.98 dBc | 43.58 dB |
| 7177 Hz | −52.97 dBc | 41.33 dB |
| 11003 Hz | −44.98 dBc | 37.14 dB |

At 997 Hz both implementations approach float roundoff, so a relative improvement
is not scored. These measurements describe selected inputs, not an alias-free
guarantee for arbitrary audio. Listening comparisons use original synthetic
fixtures and are RMS-matched. Subjective user listening remains part of integration.

## Device verification

The latest series and exact binary hashes are in the
[manifest](verification/reel-color-v1/color/manifest.json). The suite targets
180 seconds of audio profiles plus short ARM checks. It exercises four tracks,
up to eight grains per track, two effects per track and master, 32 MB of PCM,
maximum cycles, locks, swing, REPEAT, live visualization and resampling writes.
CPU frequency is 1.008 GHz, without overclocking.

Latest series: **180 seconds, 5636 callbacks, zero over-budget callbacks,
underruns, late intervals, queue overflows or slow UI frames**.

| Candidate scenario | Mean | p99 upper bound | Maximum |
|---|---:|---:|---:|
| REEL bypassed, POND+DRIVE | 20.578 ms | 23.250 ms | 23.985 ms |
| REEL+POND, four granular tracks | 24.305 ms | 26.250 ms | 27.191 ms |
| REEL+DRIVE, mixed instruments | 22.495 ms | 24.000 ms | 24.343 ms |
| REEL+ORBIT | 22.320 ms | 24.750 ms | 25.185 ms |
| SONG with changing effect pairs | 17.438 ms | 23.000 ms | 26.540 ms |

The paired bypass control fell from 23.131 to 20.578 ms mean versus the earlier
motion candidate. With REEL+POND active, colour plus the optimizations adds
3.923 ms mean versus that candidate. The ≤28 ms maximum target is met; the
≤24 ms p99 target is not met everywhere. The early ≤1.5 ms additional-cost
target also remains open. This is a successful short audio-candidate check,
not full release qualification.

Earlier candidates which exceeded the time budget are not reported as passes;
see the [experiment record](verification/reel-color-v1/experiments.json).
This short series qualifies specific scenarios for this build. Release integration
and extended checks of user projects remain outstanding.

## Reproducing the checks

From the repository root:

```text
node native/prototype/scripts/reel-color.cjs
node native/prototype/scripts/reel-color-checks.cjs
python native/prototype/scripts/reel-color-analyze.py
python native/prototype/scripts/reel-color-device.py color vN
python native/prototype/scripts/reel-color-collect.py color --final
```

Analysis requires NumPy and `work/reel-motion/audio/mix-dry.wav`, generated by
`reel-motion.cjs --render`. It also reproduces the halfband coefficients from a
least-squares fit with an exact DC constraint and checks them against the header.
Use a fresh `vN` directory on the card. Garlic must be at its menu; the launcher
refuses an open MOSS session or an occupied one-shot command.
