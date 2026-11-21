# LIVE — interaction proposal and three visual studies

Status: browser study, **not implemented in the native app**. 21 November 2026.
Entry: `visual prototype/current/live.html`. Mode: Operate. The existing monochrome MOSS design system remains authoritative. The user will select a composition before native implementation. Current preference: Score; requested revision applied with one connected D-pad and larger circular face buttons in the RG35XX layout. Score is the default preview, not approval to port.

## Scope and visual alternatives

Play the selected instrument with the eight front buttons while the sequence loops; start/stop a performance take at any time. Preserve expressive onset timing and held lengths, then optionally quantize in PATTERN.

All alternatives share one state and interaction model, at 640×480:

- **Пульт / Console:** large physical button map; the current held pitch in the middle; pattern cells below in two rows at 16 steps or up to four compact rows at 64. Best for learning the layout and playing without editing.
- **Партитура / Score:** eight pitch lanes with true onset and duration positions above one connected D-pad and circular face buttons in the physical RG35XX arrangement. Best for inspecting the recorded phrase. Risk: more visual density.
- **Петля / Loop:** one revolution per selected track cycle; arcs show note duration, start marks distinguish retriggers from continuations. Two physical maps flank it; the step grid remains below. Risk: arc pitch is less explicit than a piano roll.

No accent colors, decorative oscilloscopes or synthetic audio claims. Key inversion means physically held. Playback cursor and recorded notes are separate. 21 / 14 / 12 px text; existing 4 / 8 / 12 / 16 / 24 / 32 px spacing. A dedicated performance screen has no four-parameter editor. The footer stays at y444.

User additions: 16 / **up to 64** steps per page, default 64, automatically follow playback with a partial last page. View changes do not change the cycle. A reversible 76-step example demonstrates 64+12; 15/16/32/64/128-step lengths are also selectable. Score uses actual MIDI pitch rows; out-of-range recorded events have HIGH/LOW counts. Pressing a note moves the pitch focus to that note; changing octave recenters the viewport only when no note is held. Held pads retain their original pitch label and gate across octave changes.

## Musical objects

- Sequence owns root and scale. Up to eight assigned degrees, optional repeated octave root within those eight. A pentatonic map uses six buttons, including the octave; the remaining two visibly disabled. A custom ordered scale is a later native setup option.
- LIVE owns selected track and performance octave. Root/scale changes affect future input, not the pitches already recorded. This prevents accidental rewriting of a composition.
- The left cluster is ascending **left, down, right, up**; right cluster **Y, B, A, X**. In C minor: C3 D3 Eb3 F3 / G3 Ab3 Bb3 C4. Labels always show actual pitches.
- A take is a reversible group of note-on/note-off events. Existing parameter locks remain intact. Multiple passes add events; exact coincident onsets resolve latest-take first. Monophonic voices play chronological triggers; a new trigger replaces the prior gate. Held live input takes priority over sequenced triggers on that track. Other tracks keep playing.
- Eight input buttons do not mean eight new synth voices. Keep four tracks, current synth/cloud voice counts, eight grains per granular track, and the existing sample voice pool. The sampler should initially pitch one slot selected before LIVE; a four-pad kit performance mode is separate from an eight-note scale and must be labelled as such.
- Selected-track cycle defines its grid/circle. Other track cycles continue independently; do not reset a 15-step track because another uses 16. The transport position is shared, modulo each track's cycle.

## Proposed controls

| Action | Console | Desktop |
|---|---|---|
| Play eight mapped notes | D-pad + Y/B/A/X | arrows + Z/X/C/V |
| Start / stop | START | Space |
| Record on / off | SELECT + START | Tab + Space |
| Metronome on / off | SELECT + R1 | Tab + E |
| Track | L2 / R2 | [ / ] |
| Octave | L1 / R1 | Q / E |
| Undo last take | L1 + R1 | Q + E |
| Leave LIVE, retain take | short MENU | Escape |
| Save and exit app | MENU held 2 s | Escape held 2 s, native app |

Entry proposal: OPTIONS → LIVE on a track. SELECT + START from GRID/VOICE/SPACE enters LIVE and starts recording directly, except while a dialog, file job or naming panel owns input. The browser study is a dedicated linked page, so global entry is not claimed implemented.

In LIVE all eight front buttons are notes; A/B no longer confirm/back, SELECT+arrows does not edit BPM. Note-on happens on key-down, note-off on key-up; ignore OS repeat. Shoulder actions occur on release so L1+R1 never transposes first. A press retains its original note identity across octave changes. Preparation settings outside LIVE release held inputs before changing root, scale or BPM; recorded pitch identities remain unchanged. Empty scale positions never produce a note.

Metronome uses the sequence's common quarter-note beat in 4/4, independent of track cycle or swing. First beat is accented. The setting is persistent; toggling it neither changes octave nor interrupts a held note, transport or REC. Holding SELECT reveals START/REC and R1/MET hints. The silent prototype shows four beat marks and ON/OFF state. Native audio implementation will use a short precomputed click on a monitor-only bus, excluded from internal resampling/export; this avoids accidentally recording a click into samples. Sound and monitor routing are planned, not implemented by this browser study.

Record while playing begins immediately. Record from stop starts at tick zero, with no compulsory count-in; an optional count-in can be explored separately. A key held before recording is not retriggered or silently written. Ending recording closes the written gate at that instant but does not stop the sequence or retrigger the held sound. START stops transport and finishes the take. Track changes are blocked while recording with a clear instruction; switch off REC first. MIDI velocity is a fixed chosen level because device buttons do not measure force.

## Recording and quantization

The prototype stores fractional musical ticks and duration, retains original onset, and limits each track to 512 recorded events. It uses an isolated localStorage key; it never writes current browser projects or MS18 files. Export is labelled an interface study. Full capacity stops recording while allowing free play. Undo stores whole takes (and quantization operations), not each input edge.

Native implementation needs a deliberate event model: the existing Step structure has one melodic trigger per canonical tick, while SHIFT is relative to a displayed cell. It cannot losslessly represent arbitrary fast live input or independently shifted sampler hits just by rounding to a grid cell. Plan a bounded performance event layer with fixed-point onset and gate duration, preserving original timing. Do not silently discard multiple events in one cell. GRID must show finer events and allow selecting them. A file format revision and load/save/undo tests precede native recording.

Quantization is outside LIVE in PATTERN, scoped to the take or track: grid 1/8–1/64, strength 0–100%, onset only by default, optional gate quantization separately. Original timing is retained so repeated adjustments are not cumulative. Respect track SWING by quantizing to its swung target positions; compute residual per-event SHIFT after that. Avoid applying swing twice to a recorded performance. The browser study demonstrates straight-grid 50/100% onset quantization only; native swing-aware quantization is a planned step.

## State and interruption rules

| State | Input / feedback | Persistence / recovery |
|---|---|---|
| Empty pattern, stopped | Map active, free playing; empty grid | No note is written until REC |
| Playing | Moving cursor, PLAY | Existing events untouched |
| Recording | Explicit REC with filled dot; notes grow during hold | One undo group per take |
| Note crosses loop | Tail continues at beginning without new attack | One event, not duplicated notes |
| Existing dense pattern | Add take, deterministic voice priority | Undo restores previous complete pattern |
| No track/source | Explain missing track/sample and disable REC | Return to setup; never create silent fake success |
| Capacity reached | REC stops, message, free playing continues | Previous and accepted new events retained |
| Keyboard focus lost / tab hidden | Prototype releases holds and stops transport/REC | No stuck notes or hidden recording |
| Interrupted native input / device error | Drain held notes, finish take, stop safely | Retain in-memory draft; save outside audio callback |
| Save failure | Keep take; show retry/export | Never mark unsaved performance saved |

Page-level keyboard traversal remains conventional outside the focused device. Shift+Tab leaves the performance surface; shortcuts do not intercept typing in settings. Pointer pads and physical keys share the same handler. Variant selection changes only composition. The prototype has no permission dialog or network dependency.

## Native implementation gates, after visual selection

1. Confirm map, entry/exit, REC and undo in the browser with real users. Add the chosen visual as the only native LIVE composition.
2. Measure input-to-audio latency. Current 1024-frame blocks at 32 kHz are 32 ms, before queue/device buffering. Do not promise an instrument response from animation alone. Compare a smaller block or bounded sub-block event dispatch on the device under full load; sample timestamps do not eliminate buffering latency.
3. Add timestamped performance input queue, stable note IDs, note-off priority and overflow recovery. No allocation, disk I/O, project copies or locks on the audio thread. Maintain all existing DSP limits.
4. Add bounded event storage, save format migration, microtiming/loop-wrap handling and undo. Solve collisions and per-slot sampler timing before claiming lossless recording.
5. Integrate selected visual and GRID editing/quantization. Recover focus/input interruptions, mono priority, stopped playing, pre-held keys, overdubs, loop-crossing gates, 15/16 cycles, fast notes, record toggles and sustained granular notes.
6. Short console tests, normally four 20-second profiles: all tracks/FX, live input at event capacity, recording + visual updates, then save/reload and deterministic offline comparison. Measure latency and missed deadlines separately; reduce work before changing sound limits. No hardware tests in this design-only iteration; user disconnected console to audition 0.13.1.
