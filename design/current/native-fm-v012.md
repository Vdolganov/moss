# Native FM 0.12 · Operate

Final implementation record, 19 November 2026. This surface extends the established native 640×480 graphite instrument. [DESIGN.md](../../DESIGN.md) remains the shared visual authority; this document replaces the earlier FM CORE/CHARACTER description only for native FM 0.12. The browser prototype has its own contract.

## Overview

**Thesis:** a lively, elastic FM voice whose operator relationship and three independent envelope dimensions can be read while playing. The intended range spans drums, bass, metal, bells and long wide tones. Model:Cycles informs accessible timbral range and per-step variation; DFAM informs pitch, transient and velocity interaction. These are references, without a clone or compatibility claim.

**Own world and form:** the existing MONO screen is the form seed: `native/prototype/build/mono.bmp` and `app_mono_ui.h` establish the title, broad left visual area, right telemetry, bank row and four permanent fields. The supplied Fors Opal Gem reference (`work/fm/research/gem-screen.png`) informs the separation of amplitude and modulation. Its palette, icons and composition are not imported.

**First viewport and story:** track/preset, FM, topology and audition pitch remain above the working area. An audition excites two operator phase portraits. Amplitude scales their footprint; operator relationship shapes them; SPREAD separates them. Ordinary note-off enters RELEASING, followed by a stationary idle pair. The bank and four editable values stay visible throughout.

The [final visual review](../../.impeccable/review/fm-v012/review.md) has disposition **ship for the two scored telemetry fixes**: live ratio and velocity-scaled bend. It does not grant a whole-surface verdict or certify physical-device readability, temporal feel or full-load audio performance.

## Colors

The shared `bg`, `panel`, `line`, `text` and `dim` tokens retain their roles. The physical field order remains Y / B / A / X, using `slot-4 / slot-2 / slot-1 / slot-3`; Windows uses Z / X / C / V in the same spatial order. Button colors stay in labels, held borders and held units. Navigation, phase portraits and envelope bars remain neutral.

FM also reuses the neutral `#96999F` already present in the ORGANISM portrait. It is an existing literal, absent from the global primitive token set; this local implementation record does not promote it to a new system token. See the [documentation audit](../../.impeccable/review/fm-v012/documentation.md) for retained drift.

## Typography

The existing MOSS bitmap 5×7 renderer remains authoritative: scale 2 gives 14 px glyph height for labels, units and navigation; scale 3 gives 21 px for the instrument title and normal numeric values. Values longer than seven characters use scale 2 through the shared field renderer. Labels remain uppercase. A held field preserves its number and unit, with its physical button color marking the gesture.

## Layout

The common header, tabs, 18 px working margin and footer divider at y=445 remain. The title divider is at y=180. The phase area is centered at (216,260), with a horizontal datum from x=26 to x=406. ENVELOPES sits at x=432; AMP, MOD and BEND use three horizontal bars starting at x=498. The ratio/state caption is at (18,310).

Five bank labels start at y=326 with x=18 plus 116 px per bank. The selected bank has a neutral underline at y=342. The shared four fields start at x=18,171,324,477 and y=346, each 142×89 px. The preset picker keeps the common 532×320 panel at (54,104), five rows per page and a selected-item count such as 8/8.

## Elevation & Depth

The surface remains flat. Panel tone, thin neutral rules, the selection outline and the held-control border carry hierarchy. There are no new shadows, gradients, glow or dimensional controls.

## Shapes

Square fields and neutral underlines preserve the incumbent geometry. Each phase portrait is a connected trace with a square endpoint. Its older half is dimmer; the two pairs differ by neutral brightness. Idle uses two stationary square markers. This geometry describes operator state and does not supply decorative motion.

## Components

### Operator phase portrait and envelope indicators

`fm_dsp.h` records 96 recent points for each pair directly from the rendered modulator and carrier values: (ma,a) and (mb,b). `fm_view.h` transfers bounded snapshots to `app_fm_ui.h`. Horizontal position is modulator value; vertical position is carrier value. These are phase relationships between internal operators, not a time-domain output waveform or spectrum. Direct NOISE/CLICK output, TONE filtering, final level processing and post-voice FX are not measured by these traces.

The display scales both axes with the published amplitude envelope times voice gain; SPREAD changes the pair separation. AMP shows that same envelope/gain product. MOD shows `MOD HOLD + (1 − MOD HOLD) × modulation envelope`, independently of AMP; it is not an output loudness or FM-index meter. BEND shows the magnitude of the actual velocity-scaled pitch offset against a 48-semitone range. PUNCH retains the signed setting in its field; the BEND bar does not encode direction.

**The Live Operator Rule.** While active, the caption uses the effective RATIO target and says LIVE, including a ratio changed by a lock. The editable field remains the base parameter. `lockedratio.png` therefore correctly shows `1:1 / LIVE` with base RATIO `3:1`. Idle returns to the base ratio and IDLE. BEND publishes the same computed pitch offset used by the voice, including DYNAMICS/velocity scaling; it does not substitute raw PUNCH.

EXCITED means an active gated voice; RELEASING means an active voice after note-off; inactive shows A / STRIKE and IDLE. The three indicators clear in idle. Motion comes from audio state, without a UI oscillator, wall-clock phase, random drawing or FFT. Still captures do not establish temporal quality.

### Parameter banks and physical controls

| Bank | Y | B | A | X |
| --- | --- | --- | --- | --- |
| CORE | INDEX | RATIO | FEEDBACK | DECAY |
| BODY | DETUNE | NOISE | ATTACK | BODY |
| HIT | PUNCH | SWEEP | CLICK | DYNAMICS |
| COLOR | MOD DECAY | MOD HOLD | TONE | SPREAD |
| MOTION | RATE | MOTION | RELEASE | GLIDE |

Short L1/R1 cycles and wraps the five VOICE banks; L2/R2 changes the main tab. A tap auditions. L1+R1 opens OPTIONS, including preset selection and the PAIRS/STACK mode switch. Hold a physical Y/B/A/X button and use left/right for a fine change or up/down for a coarse change; repeat accelerates while held. Percent-style controls use 1/10, millisecond controls 1/100 ms, PUNCH 1/12 semitones, and RATE 0.01/0.10 Hz. RATIO advances one entry on either axis through the 16 defined musical ratios. RATE is displayed in Hz with two decimals.

### Modes and presets

PAIRS uses two carrier/modulator pairs. STACK adds serial interaction between the pairs while retaining a stereo intermediate tap. The topology name and audition note remain visible above the portrait. Both modes use the same twenty fields and phase display.

The eight factory presets are GLASS, BELL, HAZE, METAL, THUMP, SNAP, RUBBER and CLOUD. GLASS / BELL / HAZE / THUMP / CLOUD use PAIRS; METAL / SNAP / RUBBER use STACK. Applying a preset sets its twenty values and topology. The picker shows GLASS through THUMP on its first page, SNAP / RUBBER / CLOUD on its second. D-pad selects, A applies, B cancels; selection is bounded to the eight entries and the counter preserves position. Parameter edits are represented by CUSTOM in the existing header.

### Existing nested lock navigation

PATTERN keeps its outer STEP / VOICE LOCKS / FX LOCKS / TIMING banks, cycled by short L1/R1. Within VOICE LOCKS, L1+arrows selects the nested CORE / BODY / HIT / COLOR / MOTION bank, bounded at either end. The current nested bank name stays beside the outer row; the footer names VOICE PAGE. This is distinct from wrapping VOICE bank navigation.

All twenty FM parameters, including the enumerated RATIO, use the existing locks and the same fine/coarse steps as their base fields. A value, RESET and `--` remain distinct: RESET restores the parameter base; `--` adds no lock event. LOCK ONLY retains the outlined diamond and LOCK label, and applies parameter state without retriggering note envelopes. The existing clear/reset-current-lock-bank actions concern the four visible fields. FX LOCKS and TIMING retain their own nested navigation.

## Do's and Don'ts

- **Do** preserve the physical colors, neutral navigation, persistent units and shared field geometry.
- **Do** keep AMP, MOD and BEND independently readable; use active voice telemetry for the live ratio and actual pitch bend.
- **Do** check active, release, idle, held, preset-page and nested-lock states when changing this surface.
- **Don't** describe the phase portrait as the final output waveform, noise meter, filter response or FX monitor.
- **Don't** infer a new global token, new typography or a different navigation model from this local extension.
- **Don't** treat the scored-fix visual SHIP or device framebuffer captures as an audio/performance or temporal-feel result.
