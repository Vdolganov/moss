# MOSS visual prototype · v0.6

[Русский](README.md) · **English**

This is a separate browser prototype without audio. Native 0.5.2 controls and limits are described in [its own guide](../native/prototype/README.en.md).

> Current v0.6 controls: Q/E selects the visible object; [/] switches primary tabs; Q+E opens OPTIONS. Short V switches STEP / VOICE LOCKS / FX LOCKS / CYCLE, or the MIX/TAPE section. ENGINE, PRESET, FX type/bypass, pattern selection and slicing are in OPTIONS. Detailed browser contract: `../docs/INTERACTION-CONTRACT-v06.md` (Russian).

Open START.cmd or index.html. For development, run `node server.cjs`, then open http://127.0.0.1:4173. No audio engines are included.

## Granular study · LENS selected

Open the separate [granular.html](granular.html): SPLICE / FIELD / LENS show one cloud with shared settings. **LENS was selected on 13 November 2026** for the native instrument; all three views remain available. The study is silent, with a synthetic waveform, demo sources and separate storage; it does not modify the main prototype's projects.

Here Z/X/C/V maps to physical **Y/B/A/X**. Hold a key with arrows to edit the four fields; Q/E selects PLAY/SCAN/TEXTURE, short **C / A toggles FREEZE**, Space controls movement, Q+E opens options, Esc cancels, and Ctrl+Z undoes. Pointer users can select a field and use arrows; static mode retains a readable diagram. Switching views preserves settings; reload restores them with transport paused.

The working native GRANULAR is documented in the [separate 0.6.1-dev guide](../docs/GRANULAR-INSTRUMENT.en.md). Its short **X / V toggles FREEZE**, while **A / C auditions or assigns a sample**; study controls do not replace native controls.

## Structure

- **ATLAS:** project selection and creation only. Inside a project, the top tabs are exactly GRID / MIX / TAPE.
- **Project → GRID:** tracks with VOICE and SPACE. Selecting a track opens its editor. The overview has no step editing, mixer or global effects.
- **Track → PATTERN / VOICE / SPACE:** a pattern with persistent selected-step parameters below it; ORGANISM/MONO/SAMPLER; independent ORBIT/POND/CRUSH track effects.
- **Project → MIX → LEVELS / MASTER FX / SCENES:** mixer, effects on the sum of all tracks, and scenes.
- **Project → TAPE:** resampling one track with its SPACE, or the whole project, into sample memory.

Project/track rename, duplication and deletion are in Q+E → PROJECT / TRACK.

## Shared parameter controls

Hold **Z / X / C / V** for the current screen's first / second / third / fourth parameter. In this browser prototype, the device labels are **A / B / X / Y**; the native application uses a different physical order. There is no need to focus a field first. Labels beside values show bindings; the bottom line shows navigation.

| Screen | Z | X | C | V |
|---|---|---|---|---|
| STEP | Note | Velocity | Length in steps | Probability |
| VOICE LOCKS — ORGANISM | Tension | Drift | Chaos | Density |
| VOICE LOCKS — MONO | Cutoff | Resonance | Attack | Release |
| VOICE LOCKS — SAMPLER | Pitch | Start | End | Level |
| VOICE — ORGANISM / MONO | Same four parameters as LOCKS | | | |
| VOICE — SAMPLER | Pitch | Start | End | Level |
| SPACE / MASTER FX — ORBIT | Time | Feedback | Mix | Spread |
| SPACE / MASTER FX — POND | Size | Decay | Damping | Mix |
| SPACE / MASTER FX — CRUSH | Drive | Bits | Rate | Mix |
| FX LOCKS | Four parameters of the selected track effect | | | |
| CYCLE | Length 1–64 | — | — | — |
| MIX — selected row | Level | Pan | Mute | Solo |
| TAPE SOURCE | Source | Length | — | — |
| TAPE EDIT | Start | End | Fade in | Fade out |

With a modifier held, ←/→ changes one unit and ↑/↓ makes a larger change, or an octave for notes. Holding an arrow accelerates changes after 0.6 and 1.6 seconds. Enumerated choices move to the adjacent value; ON/OFF uses right/up for on and left/down for off. Bounds do not wrap. Releasing the modifier finishes the gesture; Tab or Escape cancels it. Ctrl+Z undoes the complete finished gesture. Focus does not move to the edited field.

In PATTERN, select a step. The lower STEP / VOICE LOCKS / FX LOCKS / CYCLE tabs select a bank. STEP has four step properties. CYCLE sets length 1–64 for this track and pattern A–D, using Z+arrows or 16/32/64 presets. Set note LENGTH with C+arrows: for a 16-step drone, enable the first trigger and set LENGTH to 16. In VOICE LOCKS, “—” inherits the instrument value; the first arrow creates a lock and enables the trigger. Merely holding the modifier or switching tabs changes nothing. OPTIONS → CLEAR CURRENT LOCK BANK clears the current bank. FX LOCKS follows one active SPACE slot; select its type in SPACE → Q+E → EFFECT. Other effect types retain their own locks. Instrument base values and neighboring steps are unchanged.

In MIX, select a track row with arrows or by hovering over one of its parameters; the selected row is highlighted. In TAPE, Z+arrows chooses MASTER or a track and X+arrows changes capture length. Both are locked during capture. Focus the tempo in the header and use Z+arrows to edit it.

Short presses without arrows: Z performs an action/toggles a trigger, X goes back, C in PATTERN switches between step and fields, and V cycles STEP → VOICE LOCKS → FX LOCKS → CYCLE. Holding without arrows does not fire the short action. Escape remains available for back/cancel.

| Keys | Action |
|---|---|
| Arrows | Navigate; edit a parameter with Z/X/C/V |
| Enter | Action |
| Q / E | Previous / next visible object |
| [ / ] | Track tabs, or workspaces outside a track |
| Space | Visual transport |
| Tab | Cancel a gesture/menu, otherwise ATLAS |
| Ctrl+Z | Undo an edit |

Dialogs remain for instrument/sample selection, names and confirmations. Numbers and resampling sources are edited in place.

## TAPE and SAMPLER

Select SOURCE and LENGTH, then press CAPTURE. FINISH, or capture completion, opens an EDIT draft; DISCARD cancels it. SAVE stores slices in the shared library. SAMPLER assigns a file to a key, with independent Pitch / Start / End / Level for each assignment. Only explicit TAPE commands slice a source.

Projects, the shared library and drafts are stored in the browser. Exports contain metadata for referenced files. Actual audio and WAV recording are not implemented.

## Persistence and testing

Old v0.1 projects are extended automatically with independent SPACE, capture settings and TAPE memory while retaining steps and old samples. Scenes retain track SPACE and master FX, but do not roll back the sample library. `file://` and localhost have separate storage; transfer projects through JSON.

`node --test model.test.cjs` runs 10 checks, including migration, effect independence and scene persistence. `?test=1` uses separate storage. The logical screen is 640×480 and long lists scroll. Limits: 8 tracks, 4 patterns and 4 scenes. The browser implementation has not been tested as an application on the physical RG35XX.

## TAPE and SAMPLER behavior introduced in v0.4

SAMPLER no longer automatically divides a sound into segments. Select a key, assign a sample, then manually move START (X+arrows) and END (C+arrows) on the schematic waveform. Z changes PITCH; V changes LEVEL. Start and end cannot cross. Each key's settings are independent, even when keys share a source. ONE SHOT ignores gate; GATE and LOOP use it. PATTERN selects only keys with available assigned files. Removed assignments show UNASSIGNED; absent files show MISSING FILE.

TAPE has SOURCE / EDIT / LIBRARY sections. PCM16 WAV import reads headers only, without decoding audio. A new source starts with one range; only explicit SPLIT or + CUT commands divide it. START / END set boundaries manually. SAVE SLICE and SAVE ALL add independent entries to the shared library. The source is unchanged.

The library is shared between projects. Scenes own key assignments and settings, not file copies. Export v2 includes the project and its referenced sample metadata; import creates independent IDs. v1 storage migrates to moss.v2 without overwriting moss.v1. Temporary `?test` sessions use separate storage.

UX-test budgets are 1 MiB PCM16 per sample and 8 MiB per active pattern. These are preliminary budgets, not GarlicOS measurements. A source shared by several keys is counted once. Trimming inside SAMPLER does not reduce the source file's RAM footprint; save a short slice through TAPE to do that.

There is no audio, actual resampling or sliced-WAV output. The library stores metadata and the waveform is schematic. Source WAV contents are not copied into browser storage.

Run all checks with `node --test model.test.cjs samples.test.cjs sequencer.test.cjs visual.test.cjs`. Browser checks also covered manual boundaries, independent keys, sparse KEY selection, locks, ONE SHOT, WAV import, save/reload and exiting capture.
