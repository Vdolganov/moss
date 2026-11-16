# MONO 0.9 · sound and controls

[Русский](MONO-v09.md) · **English**

MONO is rebuilt as a compact monophonic subtractive synth for RG35XX: solid bass, gently moving sustained tones and resonant acid lines. It is independent MOSS DSP, not a component-level Mother-32 emulation or a port of closed firmware. ORGANISM and FM keep their engines.

## Sound engine

Two bandlimited SAW/PULSE oscillators, a sine sub one octave down and noise feed a resonant four-stage low-pass. DETUNE=0 uses one main oscillator; detuning gradually introduces the second. Two bounded iterations solve the saturating feedback input. The voice runs at 2× rate and returns to 32 kHz through a half-band FIR. Saturation/coefficient tables are prepared ahead of time; rendering allocates no memory.

The previous resonance compensation restored only part of the lost fundamental. The new filter preserves more body, with a 3 Hz DC blocker. At identical dry E1/A1/E2 settings and RESONANCE=85, the fundamental is **6.11–6.14 dB** above the previous version; at zero resonance, the increase is **1.46–1.57 dB**. This is a specific digital voice-output comparison, not a headphone-output measurement or a listening-quality score.

Amplitude now has ADSR; the filter has its own DECAY and bipolar envelope depth. Overlapping notes with GLIDE retain envelopes and move smoothly in pitch. Active retriggers retain oscillator phases and filter state. Note-free locks modify the sounding voice without restarting it.

## Five banks

In VOICE, tap **L1 / R1** (**Q / E** on Windows) to change banks. Four fields keep physical **Y / B / A / X** order: hold the field button and use left/right for fine edits, up/down for coarse edits. In PATTERN → VOICE LOCKS, use **L1 + arrows** for voice banks; L1/R1 selects STEP / VOICE LOCKS / FX LOCKS / TIMING.

| Bank | Y | B | A | X |
|---|---|---|---|---|
| CORE | CUTOFF 20–12000 Hz | RESONANCE 0–100% | ATTACK 0–5000 ms | RELEASE 0–10000 ms |
| COLOR | ENV AMT −100…100% | SUB 0–100% | NOISE 0–100% | DRIVE 0–100% |
| OSC | PULSE W 5–95% | DETUNE 0–30 cents | GLIDE 0–2000 ms | DRIFT 0–100% |
| ENV | DECAY 10–8000 ms | SUSTAIN 0–100% | FLT DECAY 10–8000 ms | KEY TRACK 0–100% |
| MOTION | LFO RATE 0.01–20 Hz | PWM 0–100% | VCF MOD 0–100% | VIBRATO 0–100 cents |

Select SAW/PULSE in L1+R1 options. PULSE W and PWM affect PULSE. LFO RATE edits by 0.01 Hz / 0.1 Hz. DETUNE offsets the second oscillator; VIBRATO is periodic pitch depth, while DRIFT adds slower random motion. One LFO drives PWM, VCF MOD and VIBRATO. Both envelopes share ATTACK; filter DECAY is independent of amplitude DECAY. KEY TRACK=100% moves cutoff one octave per note octave relative to C3/MIDI48.

Internal minimum attack/release times are 3/5 ms, avoiding hard amplitude steps. DECAY and GLIDE represent approximate settling times, not linear arrival at absolute zero. SUB, DRIVE and RESONANCE interact; equal control positions do not imply equal loudness across patches.

All twenty parameters support locks, including LOCK ONLY and RESET. Try **SOFT** in PULSE mode with PWM/VCF MOD for movement, **SUB** for bass, or **ACID** for short resonant lines. The four preset names are retained, with updated MONO settings.

## Display

The waveform uses actual voice samples **before effects and track level**, at fixed amplitude scale. Its window is 32 ms at 32 kHz. Current modulated cutoff and two envelope meters sit alongside GATE / RELEASE / IDLE state. Stop produces a flat line. This is a compact, sparsely sampled scope, not a spectrum analyser; it cannot measure high-frequency content reliably.

## Research and implementation boundaries

| Primary source | Findings used |
|---|---|
| [Moog Mother-32 manual](https://cdn.inmusicbrands.com/Moog/Mother32/Mother_32_Users_Manual.pdf) | 24 dB ladder, resonance, pulse width and LFO/EG routing. Mother-32 is analogue; no digital oscillator was extracted from its firmware. |
| [Melodee FM-1 source](https://github.com/keremimo/melodee/tree/165d177a425944f3d30b0b669255a6ed0050c3a5) | Engine/resource separation, tables and cached coefficients in the experimental Prophet filter. Experimental code does not establish listening quality. |
| [Kurzhaar FM-1 source](https://github.com/kurzhaar168/kurzhaar/tree/4098f4e7ff4a10ab268d47848c801fdc455ead74) | `firmware/engine/be.c`: sub/noise, nonlinear filter, 4× processing and division cost. MOSS uses an independently written 2× path and a different solver. |
| [amsynth source](https://github.com/amsynth/amsynth/tree/1ac557ffe62ef819e239c9d9e96f70e4fb242b69) | `src/core/synth/LowPassFilter.cpp`: coefficient/render separation and filter cascades; not a Mother-32 model. |
| [u-he Diva manual](https://u-he.com/downloads/manuals/plugins/diva/Diva-user-guide.pdf) | Feedback, voice variation and the pre-effects scope on page 48 informed the functional visual direction. |
| [TAL-BassLine-101 manual](https://tal-software.com/downloads/docs/TAL-BassLine-101-UserManual.pdf) | Source grouping, SUB/noise, PWM and envelopes. Documentation was studied, not the VST binary. |

Open FM-1/amsynth code was inspected; closed Moog/u-he/TAL binaries were not decompiled. No GPL code from those projects was copied into MOSS. These are code/documentation findings, not claims of comparative audition of every referenced instrument.

## Persistence and verification

New saves use **MS15**, reading **MOS5…MS14**. Old MONO patches receive neutral added modulation, 50% pulse width and 100% sustain; filter decay is derived from the previous RELEASE coupling. The new filter still changes their sound. Older binaries cannot read MS15; rollback also requires the original project backup.

Checks cover both waves, parameter extremes, rapid locks, 8/32/44.1/96 kHz, glide/legato, release, all twenty locks and an actual MS14 layout. [Evidence and short RG35XX measurements](verification/mono-v09/README.md).

Run `node native/prototype/scripts/mono-render.cjs`, then `python native/prototype/scripts/mono-analyze.py` to generate four dry 12-second audition WAVs in `work/mono-research/audio/`. Numerical checks do not replace your judgement of the sound.
