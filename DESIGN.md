---
name: MOSS
description: "Approved monochrome UI: Interface Lab and native 0.13, 640×480."
colors: {paper: "#EDEDEB", ink: "#28292C", muted: "#606165", bg: "#191A1C", panel: "#222326", line: "#414348", text: "#E8E8E6", dim: "#A9ABB0"}
typography: {heading: {fontFamily: "MOSS bitmap", fontSize: "21px"}, body: {fontFamily: "MOSS bitmap", fontSize: "14px"}, small: {fontFamily: "MOSS bitmap", fontSize: "12px"}, duration: {fontFamily: "MOSS bitmap", fontSize: "8px"}}
spacing: {micro: "4px", compact: "8px", inner: "12px", inset: "16px", section: "24px", large: "32px"}
rounded: {cell: "4px", capsule: "12px"}
---

# MOSS · approved monochrome interface

Frozen by user approval on 2026-11-20. Current authorities: [browser surface](design/current/interface-lab-v012.md), [accepted workflow fixes](design/current/ux-fixes-2026-11-20.md), [native transfer](design/current/native-ui-v013.md). The native record defines hardware adaptations. Prior palette, typography and navigation rules are archived in [design/history/pre-ui-v013](design/history/pre-ui-v013); they are not current instructions.

## Visual system

Neutral graphite, no colored parameter/slot/region accents. Selection must be distinguishable from playback and lock state. No shadows or simulated physical depth. Existing synthesis/effect portraits remain tied to the real native signal or telemetry; the browser explicitly simulates them.

Use 21 / 14 / 12 px for headings / values and primary text / labels and hints. Only duration capsules use 8 px. The browser wrapper follows the same sizes. Native text uses the original bitmap alphabet rasterized at these roles.

640×480, 16 px side insets and 608 px content width. Main tabs at y56, local tabs y104, parameter tabs y336; fields y376/400. Footer starts y444. Portrait content ends by y320. Use 4 / 8 / 12 / 16 / 24 / 32 px spacing. Menus use 40 px rows with 4 px gaps; lists 48 px rows with 8 px gaps.

## Components and control rules

- Tabs: centered rounded white active background and dark label; no underline. Partial overflow fades at either edge. Fixed shoulder legends remain readable.
- Four fields: Y / B / A / X physically, 24 px monochrome button cluster, 12 px letter and label, 14 px value with 12 px compact unit. Exactly two text rows. No hold timers.
- Grid: 16 or up to 64 cells per page, default 64. Rounded 4 px full cells (2 px compact), note fill, lock diamond, independent selected border and inverted playing caption. Duration lines sit above cell edges, carry across pages and use 8 px inverted note capsules where space allows.
- Sampler: four monochrome slots above grid, actual native voice activity independent of editing selection. TAPE: monochrome regions with physical labels and independent preview cursor.
- L2/R2: main tabs. L1/R1: visible local tabs. L1+left/right: slot/region. L1+up/down: nested parameter page. R1+left/right: grid/list page. L1+R1: contextual options. Modifier release does not trigger a second action.
- Each enabled effect has its own LOCKS tab in chain order. Disabled effects retain data. Per-field SET/CLEAR/RESET is explicit; numeric ranges clamp. Navigation does not alter musical data or undo.
- Field + left/right adjusts fine; up/down coarse. Lock fields use the same units as the source parameter. Menus isolate background gestures; save failure retains edits. Kit saving and assigning are explicit separate intents.

## Scope

The browser shell, pointer controls and file JSON are desktop adaptations, not new device requirements. Native WAV import, PCM limits, audio scheduling and project MS18 remain native. Short hardware checks and software regressions are recorded in [verification](docs/verification/ui-v013/README.md); design approval alone is not a performance claim.
