# GRANULAR and POND: research and first implementation

16 November 2026 · [Русский](AMBIENT-RESEARCH-v08.md)

The target is a deep, continuous, evolving texture on the original RG35XX. This is a comparison of useful approaches from primary documentation, not a purchasing ranking. No matched recordings of these instruments were available, so perceived superiority over MOSS has not been established.

## References and decisions

| Reference | Useful principle | MOSS decision |
|---|---|---|
| [Instruō Arbhar 2.0 manual, pp. 20–25](https://www.instruomodular.com/wp-content/uploads/2024/01/Arbhar-Manual-Firmware-2.0-web.pdf) | Grain-direction probability; pitch, direction and duration latch at birth; smooth windows suit drones. | REVERSE probability and independent DETUNE. Existing grains keep their direction and tuning when controls change. |
| [Mutable Instruments Clouds](https://pichenettes.github.io/mutable-instruments-documentation/modules/clouds/manual/) | Overlap, timing distribution, window shape and diffusion are separate dimensions. | Retain GRAIN IN/OUT, add JITTER and SIZE VAR, use POND for diffusion rather than hiding another reverb inside each voice. |
| [Mutable Instruments Beads](https://pichenettes.github.io/mutable-instruments-documentation/modules/beads/manual/) | Independent parameter randomization; probability distributions affect character. | Separate cloud randomness from head jumps. Avoid tying every property to one common LFO. |
| [Audio Damage Quanta 2](https://www.audiodamage.com/collections/granular-synthesis/products/ad055-quanta-2) | Independent rate, pitch, direction, shape, length, pan and source position with extensive modulation. | Adopt the separation of musical tasks, not its much larger voice budget. MOSS remains at eight grains per instance. |
| [Tasty Chips GR-1 2.8 manual](https://tastychips.nl/downloads/GR-1/UserManual/User-manual-GR-1-v2.8.g.pdf) | Scan motion and grain direction are separate controls. | Negative SPEED moves the anchor backwards; REVERSE independently controls grain playback. |
| [Make Noise Morphagene](https://www.makenoisemusic.com/wp-content/uploads/2024/03/morphagene-manual.pdf) | A continuum from long fragments to microsound, with controllable overlap. | Keep lengths up to eight seconds and a held scan head; ambient need not mean tiny flashes. |
| [Soundtoys Crystallizer](https://www.soundtoys.com/wp-content/uploads/Crystallizer-Manual.pdf) | Reverse, splice, pitch and recycle create granular echoes. | Reverse is immediately useful; pitched feedback needs a separate performance and stability study. |
| [granchild / Mangl–Glut family](https://github.com/schollz/granchild) | Slowly moving jitter/spread and musical organization around granulation. | Controlled trajectories and scenes are a future stage; do not replace them with automatic modulation of everything. |

## Why longer grains alone are insufficient

With D births per second and length L seconds, average overlap is approximately D×L. Eight voices cannot render eight-second grains at 128 births/second. DENSITY is the requested rate; the bounded actual rate is shown as G/S in LENS. SIZE VAR reserves capacity for the longest possible grain, so average occupancy can fall. JITTER varies intervals; a full pool can reject a birth instead of cutting a living grain.

Density benefits from overlapping smooth windows, independent source positions, small tuning differences and a persistent tail. Excessive pitch variation can dissolve the harmonic low end into noise; rapid random changes favour glitch rather than slowly evolving ambience. Extreme stereo spread can weaken the mono centre. CLOUD defaults therefore remain moderate; reverse itself is not a loudness boost.

## Implemented controls

The fifth page, **CLOUD**, contains Y: REVERSE 0–100%, B: DETUNE ±0–100 cents, A: SIZE VAR ±0–75%, X: JITTER 0–100%. Use L1/R1 for pages and held face button plus arrows for fine/coarse changes. All controls also appear in voice locks. At 0% new grains play forwards, at 100% backwards, at 50% both directions occur. An eight-second grain can take time to leave the cloud after an edit; this preserves continuity.

Pitch, direction and duration latch at birth. Cutoff, level and the note envelope continue shaping the combined output. LENS shows actual F/R counts, source arcs and moving playback markers. Reverse uses an equivalent positive modular stride, adding no direction branch or division to the per-grain sample loop.

Starting points, not factory presets:

| Texture | SIZE / DENSITY | SPEED / SCATTER | CLOUD | Envelope |
|---|---|---|---|---|
| Slow dense pad | 2400 ms / 8 | 0.08× / 85% | reverse 50, detune 7, size var 20, jitter 10 | grain 50/50%, note 1400/3500 ms |
| Reverse waves | 4500 ms / 4 | 0× or FREEZE / 100% | reverse 100, detune 4, size var 25, jitter 5 | grain 60/40%, note 1500/5000 ms |
| Dark cloud | 1200 ms / 8 | −0.05× / 70% | reverse 35, detune 12, size var 35, jitter 20 | grain 50/50%, cutoff 3500 Hz |

Begin with a harmonically rich, sufficiently long source without silence. A short source still loops inside a long grain; reverse cannot invent missing material. Moderate detuning, a stable root pitch and subtle motion help retain bass.

## Updated POND

The core is an original orthogonal feedback delay network, using general principles described by [Julius O. Smith](https://ccrma.stanford.edu/~jos/pasp/Feedback_Delay_Networks_FDN.html). It is not a reconstructed Elektron algorithm. [Valhalla's diffusion discussion](https://valhalladsp.com/2010/11/30/valhallashimmer-tips-and-tricks-diffusion/) informs the distinction between density and duration; its proprietary implementation is unknown here.

The first eight-line design failed the device budget: 33.423 ms versus 32 ms. The selected compact design uses four main delays, four input allpasses and two allpasses inside the feedback network. Nested diffusion adds reflections on each circulation. On the diagnostic impulse it increased normalized late echo density from approximately 0.72 to 0.82, close to the initial eight-line candidate. This single metric does not prove perceptual equivalence or absence of all resonances.

**TAIL:** SIZE, DECAY, DAMP, MIX. **SPACE:** PREDELAY 0–200 ms, DIFFUSE, MOTION, LOW CUT 0–1000 Hz. DECAY displays nominal low-frequency time, 0.3–40 seconds; damping and interpolation shorten high-frequency tails more. MOTION provides subtle read-head movement. SIZE/PREDELAY edits slew smoothly. LOW CUT=0 adds no mandatory bass cut. The tail runs between sequencer steps. All eight controls support locks.

POND freeze/shimmer, independent modulation-rate control, harmonic grain intervals, multi-source clouds and granular feedback are **not part of this slice**. They deserve separate listening and resource measurements after the core is evaluated musically. A permanently wet tail should not conceal an inadequate source voice.

## Verification and data

MS14 stores the new fields. MOS5…MS13 remain readable; added CLOUD parameters load at zero and new locks remain unassigned. POND sounds different with the former four settings. Original projects and the installed binary are backed up before deployment; opening a file does not rewrite it. Older builds cannot read saved MS14 files.

Checks cover reverse at fractional rates and tiny sources, living-grain preservation, 20 voice/eight POND parameters in storage, logical reset without stale tails, 8/32/44.1/96 kHz reverb, changing locks and callback-size invariance. Short hardware profiles separately cover 32 concurrent grains, changing controls, 16 sampler voices, synths, SONG and SD recording. [Reports and measurements](verification/ambient-v08/manifest.json).

Final device result: six profiles / **115 s / 3602 callbacks**, maximum **30.483 ms** with a 32 ms budget. Zero deadline overruns, late intervals, streaming underruns, or scheduler overflows in these profiles. This is short-run evidence, not an unlimited soak guarantee. Unit tests also passed directly on ARM. The separate interactive smoke reported one late interval around project loading; it is retained in the verification notes and is not included in the zero-error stress claim. No human listening judgment is claimed for the synthetic renders.
