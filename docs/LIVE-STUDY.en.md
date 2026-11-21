# LIVE — performance and recording interface study

[Русский](LIVE-STUDY.md) · **English**

This is an interactive **silent browser study**, not a feature of native MOSS 0.13.1. Choose a visual direction before native implementation.

Run `visual prototype/current/START.cmd`, then follow the LIVE link above the device. Direct URL with the server running: [LIVE](http://127.0.0.1:4174/live.html).

## Three compositions

| Direction | Emphasis |
|---|---|
| Console / Пульт | Large physical button map, held pitch and compact step grid |
| Score / Партитура | Pitch rows, durations and off-grid timing above the physical button maps |
| Loop / Петля | One track cycle around a circle, duration arcs, flanking key maps and a grid below |

All share the same draft, monochrome style and 640×480 viewport. Changing the composition does not clear the take. Inversion means a physically held key; the cursor separately indicates playback.

**Score** is the current preferred direction and default preview. Beneath its pitch rows are one connected D-pad and large circular face buttons in the RG35XX arrangement: X above, A right, B below and Y left. The final visual choice still precedes native implementation.

## Proposed controls

| Action | RG35XX | Desktop |
|---|---|---|
| Eight pitches | left, down, right, up / Y B A X | arrows / Z X C V |
| Play / stop | START | Space |
| Record on / off | SELECT + START | Tab + Space |
| Metronome | SELECT + R1 | Tab + E |
| Track | L2 / R2 | [ / ] |
| Octave | L1 / R1 | Q / E |
| Undo take after recording | L1 + R1 | Q + E |
| Leave LIVE | short MENU | Escape |
| Return keyboard focus to browser | — | Shift + Tab |

Focus the screen first. Free play works while stopped. Record from stop starts the cycle at zero; record during playback starts immediately without restarting the sequence. Toggling REC off leaves playback running. The eight front buttons are reserved for pitches; A/B and SELECT+arrows do not navigate or change tempo in LIVE.

Root and scale belong to the sequence. Up to eight pitches, for example C3 D3 Eb3 F3 / G3 Ab3 Bb3 C4. Pentatonic plus octave uses six buttons; unused buttons are visibly disabled. Scale changes affect future input, not recorded pitches. The selected instrument keeps its existing polyphony limit.

## Working scenarios

Holds record duration; presses preserve fractional onset timing. A note crossing the loop remains one event. REC can start/stop during playback; the target track cannot change while recording. Takes are undoable.

Preparation and quantization are outside LIVE, in the browser wrapper. Onset quantization offers 1/8–1/64 grids and 50/100% strength, preserving duration and original timing. Repeated quantization does not accumulate rounding. The study includes an expressive example, undoable clear, 15/16/32/64/76/128-step cycles, separate local browser persistence and JSON export. It never writes current MOSS projects. Window focus loss releases held inputs and stops recording/transport.

**16 / up to 64 steps per page** changes the view, not the pattern length. Pages follow playback automatically; an undoable 76-step example shows pages of 64 + 12. The metronome toggles with Tab+E, the wrapper button or MET on the screen. Four visual beats follow the shared tempo and accent beat one, regardless of track cycle. Its setting persists. Native clicks are planned on a monitor-only bus, excluded from resampling; the study remains silent.

Octave changes preserve a held note's pitch, label and complete gate. Score rows represent actual recorded pitches; HIGH/LOW counts disclose events outside the eight-row pitch viewport.

## Native implementation gates

Measure input-to-audio latency before assuming responsive visual feedback means playable audio. Current blocks are 32 ms, plus queue/device buffering. Keep existing four-track, voice/grain and two-effect limits.

Native recording needs a bounded precise event layer: simply rounding input into existing step slots loses fast notes. Plan save-format migration, GRID selection of finer events, note identity and release handling, loop-wrap gates, undo, lock preservation and deterministic overdub priority. Quantize against track SWING without applying it twice. The first melodic SAMPLER mode can pitch a preselected slot; a four-pad kit is a separate mapping, not eight new sample slots.

See the [interaction proposal and implementation gates](../design/current/live-study.md). No audio-engine or device-installation changes are part of this study.
