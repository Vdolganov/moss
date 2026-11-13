# FX: Elektron research and MOSS redesign plan

[Русский](ELEKTRON-FX-RESEARCH.md) · **English** · 13 November 2026

Goal: expressive reverb, delay, overdrive and tape effects with clear controls and reliable performance on the original RG35XX. This is research and a proposal for the next version. New effects have not been implemented; installed GRANULAR/LENS still uses the existing ORBIT/POND/CRUSH/REEL.

## What was actually inspected

Official Elektron downloads: **Digitakt OS 1.54**, **Analog Heat +FX OS 1.02**, **Analog Heat MKII OS 1.22**. Built [elektron-firmware-tool](https://github.com/mischa85/elektron-firmware-tool/tree/a5bce9a6af644386d900924082993a805e812874), verified each container's checksums and decompressed MAIN OS. Inspected structure, page/parameter strings and their context. Firmware was neither executed nor installed on devices.

| Inspected image | SysEx bytes | MAIN OS bytes | Container / verification |
|---|---:|---:|---|
| Digitakt 1.54 | 1,423,776 | 2,479,680 | ELE3 / OK |
| Heat +FX 1.02 | 1,229,088 | 2,174,964 | ELE3 / OK |
| Heat MKII 1.22 | 637,856 | 1,234,756 | ELE3 / OK |

URLs, SHA256 values and section inventories are in the [manifest](verification/elektron-fx-research/manifest.json). Images and decompressed sections remain in local ignored `work/elektron-fx-research`, outside Git.

+FX contains Delay/Reverb/Warble page names and parameter labels; Digitakt contains project-effect and track-send labels. These are investigation anchors, **not recovered DSP algorithms**. PLATEAU, for example, appears between PLASTER and PLATOON in a general word dictionary: it does not establish a plate topology. Stock reverb topology, nonlinearities and coefficients have not been determined here.

## Corrections to the supplied brief

- [DT-FM's DSP reference](https://github.com/mtalikka/DT-FM/blob/a8ea4d162946321d0ea59ed39a77f04b96512fc0/DSP.md) is real. Its 24 kHz rendering, interpolation and saturation table belong to the **DT-FM modification**, not Elektron's stock effects. The useful lesson is separating audio and slow controls, then measuring savings while voices are sounding.
- The [official Heat +FX manual](https://www.elektron.se/wp-content/uploads/2024/09/Analog_Heat_FX_User_Manual_ENG_OS1.01_240325.pdf) describes analog HEAT alongside digital FX, including a dedicated compressor. Relevant controls include delay filtering, separate reverb decay/spectral shaping and Warble controls. Firmware does not replace measurements of analog saturation circuitry.
- The brief expands into synths, compression and automatic slicing. The present priority is the four requested effects and their signal path; rewriting samplers and synths simultaneously would dilute it.
- MOSS already has callback profiling and hardware stress tests. The next step is candidate-level cost and sound comparison, not another debug screen in the musical UI.
- More controls, float32 and oversampling are not sufficient quality criteria. Require useful behavior/ranges, level-matched comparisons and extreme-value tests.

## Current MOSS audit

Inspected `native/prototype/src/effects.h`, `effect_defs.h` and `engine.h` at checkpoint `e0bf99b`.

| Effect | Current implementation | Character limitations |
|---|---|---|
| ORBIT | Stereo PCM16 delay, eight divisions, smoothed length, cross-feedback | No feedback HPF/LPF, saturation control or separate modulation; hard-limited writes; TIME always bends pitch |
| POND | Four comb branches per channel, followed by two diffusion sections | No predelay or modulation; DECAY is a coefficient, not decay time; the last two lengths stay fixed in samples while the others scale with sample rate; long tails need more density and movement |
| CRUSH | One symmetric nonlinearity, quantization and sample hold | Overdrive tied to digital degradation; no circuit choice, pre/post filters, bias or antialiasing; DRIVE=0 still applies a nonlinearity |
| REEL | Two fixed-frequency periodic oscillations, about 6 ms delay, LPF, soft limiting and noise | WEAR couples spectrum, saturation and level; no separate wow/flutter or low-frequency coloration; delay/nonlinearity remain with all three controls at zero; unaligned dry/wet introduces phase coloration without wow |

After **every active effect**, the signal returns to a bounded integer range. The sum is also limited before master effects. Unrequested technical clipping therefore becomes part of the sound. Separate intentional saturation from level protection before adding more processors.

All five FxState instances reserve every type regardless of the two-active-effect limit: about **8.73 MiB**, including **7.32 MiB** of delay buffers. This is memory, not a CPU measurement. Coefficients are already prepared outside the inner sample loop; disabled effects already skip processing after a fade. Those are existing optimizations.

## Proposed character and controls

These are original MOSS proposals, not claims about Elektron's algorithms. The first bank handles everyday playing; the second shapes character. Physical order remains **Y / B / A / X**, with MIX on the right of bank 1.

| Effect | Sound goal | Bank 1 | Bank 2 |
|---|---|---|---|
| ORBIT | Clear rhythmic echoes through dark, saturated dub repeats | TIME · FEEDBACK · WIDTH · MIX | LOW CUT · HIGH CUT · DRIVE · MOTION |
| POND | Short, springy spaces through wide, gently moving ambient tails | SIZE · DECAY · DAMP · MIX | PREDELAY · DIFFUSION · MOTION · LOW CUT |
| DRIVE | Dense, soft bass through asymmetric grit and fuzz | DRIVE · TONE · OUTPUT · MIX | CIRCUIT · BIAS · BODY · DYNAMICS |
| REEL | Full clean recording through unstable, worn cassette | DRIVE · TONE · BUMP · MIX | WOW · FLUTTER · NOISE · DROP |

**DRIVE becomes a separate effect; CRUSH remains for bit/rate reduction.** Circuit variants must differ in distortion shape and dynamics, not merely gain. Names and numerical ranges remain proposals until listening tests.

- **ORBIT:** feedback filtering/soft saturation, independent slow modulation and smoothed controls. Two meaningful TIME behaviors: pitch-bending glide and crossfaded read heads. Hermite interpolation alone does not remove Doppler shifts. Test large TIME jumps and locks separately.
- **POND:** compare a compact modulated tank with an eight-line FDN; input diffusion, frequency-dependent decay, predelay and stereo output. [Dattorro's paper](https://aes2.org/publications/elibrary-page/?id=10160) provides an engineering starting point, not evidence of Elektron's internals. Tune echo density and motion on percussion and sustained sources. Freeze/shimmer are not mandatory for version one.
- **DRIVE:** prefilter → nonlinearity → DC removal → postfilter → controlled output. Compare local 2× oversampling with [ADAA](https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf), including latency/filtering effects; do not claim alias-free output. Predictable gain compensation must not hide character behind master compression.
- **REEL:** independent slow/fast irregular motion, recording saturation, bass coloration, spectral losses, noise and occasional smooth dropouts. Check dry/wet phase alignment and total latency. [CHOW Tape and its published model](https://github.com/jatinchowdhury18/AnalogTapeModel) explain the phenomena; a full physical hysteresis solver is not mandatory without RG35XX cost measurements.

## Performance and routing

[Latest hardware results](GRANULAR-NATIVE-v1.en.md): four GRANULAR tracks with POND+REEL on every track and master averaged **25.123 ms**, p99 ≤ **26.5 ms**, maximum **30.436 ms**, against **32 ms**. No budget overruns, but this is not 30% free capacity. Worst observed margin is approximately 1.56 ms.

Test the first candidate in the current architecture: four tracks, up to two inserts per track and two master effects. If the desired quality does not fit, the preferred alternative is **one shared ORBIT and one shared POND with track sends**, local DRIVE/REEL/CRUSH, and master control. This avoids five expensive reverbs. Send processors run fully wet; sends/returns set level.

This changes the musical model and must not be introduced silently: tracks lose independent space settings. Step locks would control each track's SEND, not let four tracks compete over shared parameters. Shared settings belong to the sequence; tails continue when sends fall to zero. Bidirectional delay/reverb feedback is unnecessary initially. Choose routing after candidate A/B and cost measurements.

Keep audio at 32 kHz. Do not automatically reduce reverb to 16 kHz to improve CPU results: the user wants bright sounds too, and the bandwidth changes. Test a bounded float32 path, prepared coefficients/tables and local oversampling independently. Save work with every voice sounding, not just in silence. No hidden effect shutdown under load.

## Controls, locks and code structure

FX list: D-pad selects, A opens, B returns; preserve enable/reorder controls. Inside an effect, **L1/R1 switches banks**, as in VOICE. Four face buttons plus arrows edit; horizontal is fine, vertical coarse. L2/R2 remains top-level navigation.

In FX LOCKS, ordinary L1/R1 selects STEP/VOICE/FX/CYCLE; **L1+left/right** selects the effect and **L1+up/down** its bank. The header names both. Bind locks to stable effect/parameter IDs, not chain position. Define switching behavior or lock restrictions for circuit/topology choices; do not interpolate mode numbers.

Limit refactoring to shared DelayLine/Filter/Smoother/DCBlock utilities and separate effect states with prepare/process/reset. One parameter description supplies name, unit, range, step, bank and lockability to UI and DSP. Preallocate buffers; no callback file I/O or allocation. Check persistence, SONG and scenes as parameter storage grows; transitions must not clear large buffers in the audio thread.

## Implementation order and acceptance

1. Capture baseline renders: short hit, bass, acid phrase, noise texture and granular pad. Add per-effect measurements outside the product UI. Separate technical clipping from intentional distortion.
2. Build **ORBIT + DRIVE** as audio candidates; agree on level-matched examples and control extremes.
3. Compare both **POND** candidates for tails, density, stereo and CPU. Settle insert/send routing before UI integration.
4. Build **REEL** from independently controlled mechanisms. Tune neutral behavior, dry/wet, noise and dropout without masking the source.
5. Connect banks, locks, persistence and visualization of real measurements. Repeat maximum-load device tests before installation.

Audio acceptance: level-matched A/B, impulse/decay, high-tone aliasing, DC, mono compatibility, tails across empty steps, and smooth locks/bypass/reorder/SONG transitions. Performance acceptance: all four tracks, every permitted effect active, long patterns, extreme controls, capture and UI. Aim to increase margin, provisionally p99 ≤24 ms and maximum ≤28 ms in the same series; these are targets, not achieved results. Listening chooses sound quality; a zero-overrun counter cannot establish it.
