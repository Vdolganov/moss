# Native MOSS 0.5.1 — RG35XX

This contract supersedes native-v05-interaction.md for the native application only. The browser prototype retains its independent v0.6 contract. Target: original RG35XX, 640×480. Current hardware sessions use GarlicOS 1.4.9 over ADB; GarlicOS 1.3.9 remains a compatibility target, with this binary not yet retested there. Preserve the existing flat graphite identity and the four physical-button colors.

## Capacity and sound

Four tracks per project (explicitly accepted by the user). Each track chooses SAMPLER, MONO, ORGANISM or FM. Up to two of ORBIT / POND / CRUSH / REEL may be enabled per track; the master has the same two-effect limit. All effect settings and identity-based locks persist when bypassed or reordered. Refusing a third active effect explains that one must be bypassed first.

Each SAMPLER instance has four independent sample slots, four simultaneous primary voices, and bounded retrigger fades. The same asset may be assigned more than once with independent pitch/start/end/level. Four named slot cells (1–4 / C3–D#3) sit above its pattern; the neutral border marks the selected slot and the grid shows only that slot's hits. Empty slots remain selectable and are assigned in VOICE. Maximum prepared asset remains 1 MiB; the shared library holds 32 assets. Imported long sources stream for audition.

The audio engine runs at 32000 Hz / 1024 stereo frames. Track cycle remains 1–512 canonical 1/64 ticks; 1/8, 1/16, 1/32 and 1/64 resolution changes the displayed cell count without moving events. There are 16 displayed cells per grid page. Performance evidence is recorded separately; UI review does not establish device performance.

## Controls

Left to right parameter fields: **Y, B, A, X**. Parameter meaning stays in its existing left-to-right order; bindings follow the new button order. Hold a button and press arrows; repeat accelerates. Colors belong to physical buttons: Y #C69B83, B #91AEBB, A #A7BA91, X #ACA0BA. A short press remains confirm/audition/toggle; B back; X contextual preview/bypass; Y has no page-navigation action. A used modifier must never fire its short-press action when released.

- L1 / R1: previous/next parameter bank in PATTERN, synth VOICE, MIX and TAPE. In sample VOICE they select a slot; in SPACE they select an effect. In the project/track lists they move through the list.
- L2 / R2: primary workspaces GRID / MIX / TAPE or track views PATTERN / VOICE / SPACE.
- L1 + arrows: the nested object where relevant — sampler slot, synth CORE/CHARACTER lock bank, or FX LOCKS target. Releasing the modifier must not also change the parameter page.
- L1 + R1: context menu, only if the current state has options. No empty menu.
- START: transport; SELECT + arrows: global project tempo. MENU hold: save and exit when capture is safe.
- In Windows, Z/X/C/V map spatially to Y/B/A/X; Q/E are L1/R1, brackets L2/R2. The RG35XX keeps its physical button map.

Sampler STEP fields: VELOCITY / FADE IN / LENGTH / PROB. LENGTH = FULL (stored as 0) means play the complete sample once. A finite value caps that hit in current grid steps, with a short release to avoid clicks. A short sample always stops naturally; LENGTH never loops it. Existing fade-out values remain in saved data, and TAPE trim retains both fade controls.

## Context menus and recovery

Menus show only relevant actions. Creation has permanent + NEW PROJECT and + ADD TRACK rows on ATLAS and GRID. SLOW SIGNAL is an initial seed only; no DEMO action is exposed. Presets and synth mode stay in VOICE; no app-exit item there. EXIT MOSS is in ATLAS OPTIONS; MENU held for two seconds is the global save-and-exit gesture. Assignment/replacement and removal stay in SAMPLER VOICE; one-shot playback is behavior, not a fake menu action. PATTERN offers length-to-cycle-end on STEP, and clear-current-lock-bank only on lock pages. Resolution presets, redundant toggles, midpoint auto-cuts, duplicated navigation and restart actions are hidden. TAPE has file/slice operations rather than unrelated project operations; save/assign and preview remain direct buttons. Multi-slice operations appear only when useful.

Project and track delete retain confirmation/recovery. Capture writes and waveform preparation run outside UI/audio threads; the immutable capture remains in RAM until success. While saving, TAPE shows SAVING CAPTURE / PLAYBACK CONTINUES. Failure retains PCM for retry or discard. Old MOS5 projects migrate to MOS6: first four tracks, first four assigned sample lanes compacted in order, first two active effects in chain order. Original device project files are backed up before installing; loading alone never overwrites them. New per-hit LENGTH persists in MOS6.

## Names

Every editable name uses the same 10-column, four-row keyboard: 38 characters (A–Z, 0–9, space, hyphen), followed by DEL and SAVE. This covers project creation/rename, track rename, sample rename, one slice, all slices, and import-and-assign. A counter shows the 24-character limit. Arrows move key focus with wrap, A types the focused character (or activates DEL/SAVE), X deletes before the caret, START saves, B/SELECT cancels, and L1/R1 moves the caret. START saves here instead of controlling transport.

Existing text is selected on entry. The first typed character replaces that selection; X clears it. L1/R1 releases the selection and moves to the corresponding edge before normal caret movement. Blank names are rejected. Save errors preserve the typed text for retry; cancel from a slice name returns to the same trim. The panel, field, selected text and focused key use neutral graphite/white states, not parameter colors.

## Runtime and verification boundary

The launcher selects the performance governor with a maximum of 1.008 GHz, without overclocking, then restores the previous governor and frequency bounds on exit. Capture saving uses the completed immutable PCM buffer; writing the WAV and preparing its waveform overview run in the background. Reports are written asynchronously. These are native implementation contracts, not changes to the browser prototype.

Hardware results belong in `docs/NATIVE-PERFORMANCE-051.md`. The final ten-minute four-ORGANISM GUI test, with background I/O and audio-thread priority, completed without audio overruns or slow UI frames. Exact conditions and artifact hashes are recorded there; this does not establish a new run on GarlicOS 1.3.9.

## Visual verification scope

Native 640×480 render captures cover sampler steps/locks, all synth parameter banks, effect chain, resolution, project grid and contextual menu states. No new visual world, illustration, comp, or raster asset is being introduced. Functional graphs remain labeled models; no decorative motion is added.

The completed UI review disposition is SHIP. Captures in `.impeccable/review/native-v051/` include `drums.png`, `name.png` and `nameslice.png` alongside the synth, FX, grid and contextual-menu states. Native parameter fields retain the physical button colors and units while held; diagnostic HOLD timers are absent. No new typography or visual identity is authorized by this pass.
