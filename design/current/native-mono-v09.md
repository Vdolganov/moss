# Native MONO 0.9 · Operate

Preserve the approved 640×480 graphite system, Y/B/A/X colors, four value fields, L1/R1 banks and L2/R2 main navigation. Scope: MONO DSP, voice/lock controls and the MONO visualization. ORGANISM and FM keep their UI and synthesis.

## Direction contract

THESIS: show what the voice is doing while the musician edits it. The existing static filter drawing is replaced by actual pre-effect voice samples and modulation telemetry; no simulated audio or animated decoration.

OWN-WORLD: the established flat graphite, bitmap type, neutral selection and physical-button colors. The source is an instrument panel used on a small handheld, often in dim light. No knobs, gradients, virtual wood or new accent colors.

STORY: select MONO, audition/play, shape the sound across five visible banks, and hear/see the result. The same twenty controls are available in VOICE LOCKS. Held keys retain values and units; cancelling options preserves the sound.

FIRST VIEWPORT: existing title and track/preset line; a large waveform occupies the left of the middle region. On the right, actual cutoff and two envelope bars establish activity. A short state line distinguishes GATE/RELEASE/IDLE and LFO. Five compact bank tabs sit above the four existing parameter fields.

FORM: extension of the approved native VOICE surface; code-led, no concept roll. References: Diva manual scope (pre-effects signal), Mother-32 source/filter/envelope grouping and TAL-BassLine-101 panel grouping. The waveform is measured; this is not a spectrum or a promise of analogue circuit identity.

FINISH: unreviewed and undocumented is unfinished; this build ends with the finish review, the verdict, DESIGN.md, and every shipping raster carrying its provenance

## Implemented surface · 16 November 2026

This is a local extension of the existing system in `DESIGN.md`. Its palette, bitmap typography, square geometry, neutral selection and physical-button colors remain authoritative and unchanged; `.impeccable/design.json` is also preserved. No new visual world, reusable token or shipping raster asset is introduced.

The MONO VOICE middle region now shows actual voice output before effects. The left scope reads 64 retained DSP samples, sampled every 16 audio frames; at 32 kHz its displayed window is 32 ms. It uses a fixed amplitude scale, not an auto-normalized illustration or spectrum. The right column shows instantaneous modulated cutoff as VCF NOW, plus separate AMP and FILTER EG levels. The LFO marker reports the current modulation value. All of these come through the bounded engine mailbox. PRE FX / GATE, RELEASE or IDLE names the voice state. In IDLE, the waveform is flat, both envelope bars are empty, the LFO is centered, and VCF NOW displays the base CUTOFF setting rather than stale active telemetry.

Five bank names remain visible above the existing four fields. The active bank uses the incumbent neutral underline. The fields retain their established positions and units; colors stay attached to physical Y / B / A / X (Windows Z / X / C / V).

| Bank | Y | B | A | X |
| --- | --- | --- | --- | --- |
| CORE | CUTOFF · Hz | RESONANCE · percent | ATTACK · ms | RELEASE · ms |
| COLOR | ENV AMT · signed percent | SUB · percent | NOISE · percent | DRIVE · percent |
| OSC | PULSE W · percent | DETUNE · cents | GLIDE · ms | DRIFT · percent |
| ENV | DECAY · ms | SUSTAIN · percent | FLT DECAY · ms | KEY TRACK · percent |
| MOTION | LFO RATE · Hz, two decimals | PWM · percent | VCF MOD · percent | VIBRATO · cents |

On VOICE, short L1/R1 cycles these five banks, A auditions, and L1+R1 opens options. L2/R2 retains the main PATTERN / VOICE / SPACE navigation. In PATTERN / VOICE LOCKS, L1+arrows selects the nested MONO bank while short L1/R1 continues to switch STEP / VOICE LOCKS / FX LOCKS / TIMING. The nested name appears beside those four pattern banks. The same twenty parameter definitions and four physical-button assignments are used in both places. Inherited locks remain `--`, reset remains `RESET`, and lock fields state STEP LOCK; an explicit LFO RATE lock retains its two-decimal Hz display. Holding a physical button and using arrows retains the established fine/coarse editing model.

The shared header and track/preset context, 18 px edges, parameter row at y=346, and footer at y=445 are retained. ORGANISM and FM continue their CORE / CHARACTER diagrams; MONO alone uses this telemetry surface.

## Finish record

The independent visual review returned **SHIP** for CORE, COLOR, OSC, ENV, MOTION, idle, release and VOICE LOCKS at 640×480. See [review](../../.impeccable/review/mono-v09/review.md) and [documentation/evidence record](../../.impeccable/review/mono-v09/documentation.md). The reviewed PNGs are native-renderer QA captures, with provenance recorded there; they are not new runtime art assets.

Visual finish and documentation are complete for this surface. Separate [device verification](../../docs/verification/mono-v09/README.md) is now complete for the reported binary and conditions: six uninterrupted stress profiles, 105 seconds and 3289 callbacks, with a maximum of 30.078 ms against the 32 ms budget and no over-budget callbacks, late intervals or stream underruns. The separate interactive session retains one late interval and its documented qualification. The verification record also covers bank navigation, physical-button fine/coarse editing, stop/idle behavior, installation and return to Garlic. These are bounded device results; the visual SHIP verdict remains a visual assessment, not an exhaustive audio-quality or performance guarantee.
