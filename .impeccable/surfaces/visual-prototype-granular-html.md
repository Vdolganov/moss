---
version: 1
slug: "visual-prototype-granular-html"
primary_target: "visual prototype/granular.html"
related_targets: ["visual prototype/granular.js","visual prototype/granular.css"]
---

# Granular VOICE comparison study

Scope: local extension of the established MOSS VOICE surface, delivered as a separate page inside the browser prototype. Operate mode. The initial request compared several interactive design options before native integration; all three remain available in this silent study.

Selection, 13 November 2026: the user chose **LENS / Линза** and requested a working native instrument. Native implementation is governed separately by [native-granular.md](../../design/current/native-granular.md); usage and format differences are in [GRANULAR-INSTRUMENT.md](../../docs/GRANULAR-INSTRUMENT.md). The study remains synthetic and silent. Its short C / A freeze action does not change to the native X / V action.

## Direction contract

THESIS: One small sample becomes a living cloud. Compare three visual explanations of the same state, rather than three different instruments.

OWN-WORLD: Inherit the neutral graphite, flat strokes, monospace instrument labels and physical Y/B/A/X colour mapping. No glow, depth, ornamental noise or new navigation palette.

STORY: Start with a prepared demo, change size/density, freeze the scan, explore the range, then switch views without losing parameters. The magic comes from seeing grains form and dissolve.

FIRST VIEWPORT: A working 640×480 VOICE with source at the top, a central data-driven diagram, PLAY/SCAN/TEXTURE/ENVELOPE banks and four fixed fields at the bottom. Outside the device, three equal comparison controls and a concise key guide; no decorative hero.

FORM: Code-led local extension, no concept roll: the shell and interaction grammar are already specified. SPLICE uses source-aligned grain windows; FIELD uses birth position, age and pitch; LENS wraps source position into a circle. Their geometry must be distinguishable at thumbnail size.

FINISH: unreviewed and undocumented is unfinished; this build ends with the finish review, the verdict, DESIGN.md, and every shipping raster carrying its provenance

## Interaction and state

- One demo source, one cloud, maximum eight live grains. Synthetic waveform and animation only, no audio, import, native DSP or project-format migration.
- Z/X/C/V map to physical Y/B/A/X. Hold a face key and use arrows: horizontal fine, vertical coarse; repeats accelerate. Q/E change banks. Short C/A freezes the scan; a used parameter gesture never toggles freeze. Esc cancels the active edit or closes the source/options dialog. Ctrl+Z undoes edits.
- All variants share parameters, source, freeze, scan position and transport. Switching never resets them. Study state uses its own storage key; existing projects are untouched. Reload restores settings, with transport paused.
- Pointer users can select a parameter and use the same arrows; visible transport, source and freeze controls also work by click. Source picker returns focus; empty source offers assignment. Replacement and removal can be undone.
- Local storage failure retains an in-memory working state and explains that it cannot persist. No fabricated file-loading or permission state: this study uses bundled synthetic fixtures.
- Reduced-motion/static mode stops continuous animation but retains readable diagram state and parameter feedback. Hidden tabs pause the simulation; resuming never fast-forwards.
- Native integration now uses the selected LENS view, native short X for FREEZE and the ranges in its own contract. Hardware performance is verified separately; this study and its visual SHIP verdict do not establish that result.

## Cloud follow-up · 0.6.1

The study now mirrors normalized SCATTER, per-grain pan, selected-range reading and four banks. At 100%, birth areas cover the selected source independently of the anchor. LENS displays birth marks and read markers; the anchor marker returns below 100%. Grain opacity follows the latched GRAIN IN/OUT envelope. LEVEL and NOTE IN/OUT remain silent visual controls; there is still no audio engine. Existing study data receives default SCATTER and maps former FADE to both note fades. Native controls remain documented separately.
