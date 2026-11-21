# ORGANISM 0.10 — living drone

[Русский](ORGANISM-v010.md) · **English**

ORGANISM now has four coupled carriers, an independent centred sub, stereo movement, tonal noise and a separate roughness control. It is an original digital drone voice for RG35XX, not a Lyra-8 circuit emulation. MONO, FM and the effects retain their sound engines.

## Sound and controls

Two main carriers morph through sine, triangle, saw and square. Two darker body carriers move from sine towards a rounded triangle and can sit up to an octave above the main pair. DENSITY introduces the additional voices without dividing the root by the voice count. COUPLE is bounded phase modulation: PAIRS connects neighbouring pairs; RING connects the four voices in a loop. All connections use the preceding carrier frame, avoiding evaluation-order bias. The sine SUB sits an octave below the played note, after the moving low-pass, and stays centred.

Independent, non-synchronous slow oscillators and smoothly changing random offsets make sustained notes move. DRIFT controls pitch instability in cents; DETUNE is a fixed spread between carriers. RATE sets the slow oscillators' reference speed, WANDER moves shape/coupling/cutoff, and BREATH varies the four levels independently. NOISE adds a dark-to-bright filtered noise layer. GRIT roughens phase and amplitude; it can add sand without turning up NOISE. A resonant stereo 12 dB low-pass and soft saturation supply colour. SPREAD=0 collapses both carrier placement and filter movement to mono.

Hold **Y / B / A / X**, then left/right for fine changes or up/down for coarse changes. In VOICE, **L1 / R1** selects the five banks; on Windows these are **Q / E**. In PATTERN → VOICE LOCKS, use **L1 + arrows** for the nested voice bank. All twenty fields support ordinary locks, LOCK ONLY and RESET; a lock on a held note changes that voice without restarting its phase or envelope. Mode and presets remain in L1+R1 options.

| Bank | Y | B | A | X |
|---|---|---|---|---|
| CORE | SHAPE 0–100% | COUPLE 0–100% | DENSITY 0–100% | SUB 0–100% |
| TONE | CUTOFF 40–14000 Hz | RESONANCE 0–100% | DRIVE 0–100% | NOISE 0–100% |
| MOTION | DRIFT 0–30 cents | RATE 0.01–2 Hz | WANDER 0–100% | BREATH 0–100% |
| VOICES | DETUNE 0–50 cents | INTERVAL 0–12 semitones | SPREAD 0–100% | GLIDE 0–3000 ms |
| ENVELOPE | ATTACK 0–15000 ms | RELEASE 0–20000 ms | GRIT 0–100% | NOISE TONE 0–100 dark-to-bright |

Attack/release have internal 3/5 ms minima. Release is an exponential decay (specified time to roughly −80 dB), not a hard cut at the displayed time. GLIDE is an approximate settling time. CUTOFF is internally limited to 35% of output sample rate; at production 32 kHz the ceiling is 11.2 kHz. The wider stored range remains usable at higher test rates. RATE changes by 0.01 Hz horizontally and 0.1 Hz vertically. INTERVAL changes by one semitone on either axis.

Try ROOT for a grounded living drone, BREATH for a wide slow pad, TANGLE for rough coupled motion, and BARE for a restrained bass bed. Starting with BARE, raise DENSITY and WANDER, then add a little GRIT. Add REEL and POND after choosing the dry voice. The new presets are intentionally different from the old patches.

## Portrait

Four interwoven contours replace the static routing bars. Their shape, displacement and line weight follow actual carrier modulation, energy and envelope telemetry. DENSITY distinguishes present and dormant carriers. GROW / FADE / REST and the SUB indicator remain visible; STOP gives stationary contours. This is a portrait of the voice's state, not an audio waveform or spectrum. The four field colours and physical Y/B/A/X order are unchanged; the portrait itself is neutral graphite.

## Resource budget and signal limits

The four drone carriers run at half the output rate with conservative harmonic budgets and linear reconstruction. At 32 kHz this means a 16 kHz carrier bank; upper harmonics are deliberately restrained. SUB, noise, envelopes, the resonant filter and saturation run at full rate. This allocation leaves the low register intact and keeps bright texture available through NOISE/GRIT without paying for four full-rate complex oscillators on every track.

Wavetables use eight harmonic budgets; budget changes crossfade over approximately 8 ms. Control values are smoothed and recalculated every 64 output frames. Tanh colour uses first-order antiderivative antialiasing with a midpoint fallback for small differences. These methods reduce aliasing; they do not guarantee an alias-free result under extreme phase modulation. A 3 Hz DC blocker follows the voice. Fixed tables occupy about 53 KiB, shared by all voices within an engine; the render loop allocates no memory and performs no file access.

## Research

| Primary source inspected | Design finding |
|---|---|
| [Caravel drone engine, revision 14678f9](https://github.com/bonszaimusic/caravel/tree/14678f9b6474ef78d0f15d67265193b716b987c0) | Bounded phase coupling, previous-frame snapshots, topology and control smoothing. Experimental GPL-3.0 source; its documentation does not establish hardware/listening validation. |
| [DUODRONE, revision 1968529](https://github.com/pjac1810/duodrone/tree/1968529000cd500715d72434c27d1e7f3222330a) | Actual FM-1 fixed-point DSP: morphing waves, sub, saturation and separate control/audio rates. GPL-3.0; no implementation copied. |
| [Helio FM-1, revision fd5bf46](https://github.com/joehard4712/Helio-FM1/tree/fd5bf46ae9b14fce8be681c723b5ef6a90415270) | Separate slow level movement and random pitch drift in a multi-oscillator drone. Inspected as a reference, not imported. |
| [The Remnant](https://all-the-machines.com/products/the-remnant/) | Product documentation: feedback networks, evolving timbre and a visual identity tied to interacting voices. The closed plugin was not decompiled or auditioned. |
| [Parker, Zavalishin & Le Bivic, DAFx 2016](https://dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf) | Antiderivative antialiasing for memoryless nonlinearities; independent compact LUT implementation in MOSS. |

The sources informed mechanisms and resource choices. No external firmware or VST DSP code is included in MOSS, and no sonic equivalence to these instruments is claimed.

## Projects and verification

New saves use **MS16**, reading **MOS5…MS15**. Old ORGANISM fields and step locks are remapped: TENSION→SHAPE, DRIFT→cents, CHAOS→COUPLE, DENSITY→DENSITY, COLOR→Hz, GRAIN→NOISE, ATTACK/RELEASE→their new bank. New controls start from ROOT defaults. Notes, timing, other instruments and effects are retained. The new engine changes the sound of old ORGANISM patches. Keep the original files for rollback: old binaries cannot read MS16.

[Measurements and hardware evidence](verification/organism-v010/README.md). `node native/prototype/scripts/organism-render.cjs` creates four dry A1 examples and a 40-second held-note study with REEL/POND and parameter locks in `media/organism/`. `python native/prototype/scripts/organism-analyze.py` measures the dry files. Digital checks and resource measurements do not replace listening on the intended headphones.
