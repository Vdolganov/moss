# Native GRANULAR documentation handoff · 13 November 2026

Disposition: documented ordinary extension. The [finish review](finish-review.md) is `ship` for native UI finish. Hardware audio/frame-time results and separate installation remain outside this documentation verdict.

Added bilingual `docs/GRANULAR-INSTRUMENT.md` / `.en.md` covering assignment and named import, all 12 parameter ranges, Y/B/A/X and Windows mappings, audition, freeze, C3 transposition, LENGTH/FADE gates, nested locks, actual lens data, the one-source/1 MiB/eight-grain limits, and MOS7 compatibility. Added clearly bounded experimental 0.6.0-dev sections to both native READMEs and a small experimental note to PRODUCT.md. Stable 0.5.2 history and MOS6 contract remain intact.

The intended separate MOSS GRANULAR installation and preservation of original projects/user files are documented as a workflow, not as completed work. Windows regression/integration, UBSan and ARM build status were supplied by the parent implementation task; this pass did not rerun them. The hardware series was in progress, so no performance result is claimed. DSP provenance and the high-pitch linear-interpolation limitation are stated without claiming equivalence to Mangl.

## Evidence and preservation

Inspected `granular_defs.h`, `granular.h`, `engine_granular.h`, relevant `engine.h`, `app.h`, `app_ui.h`, `app_granular_ui.h`, `model.h` and `storage.h`, plus the native surface contract and finish review. Values, source range semantics, skipped births at capacity, two-second audition gate, MOS7 writing and MOS5/MOS6 reading match the documented behavior. The lens uses immutable source peaks and bounded atomic audio snapshots; no synthetic animation clock is documented as live audio.

`DESIGN.md` and `.impeccable/design.json` were preserved byte-for-byte; SHA-256 values:

- `DESIGN.md`: `6C5E4C45A769810C4DBE620B83D2AFD79CDF537569F6CFC227A0E2B5AA1C48C2`
- `.impeccable/design.json`: `43C5B9F7AACC3CBB4DFC5F2799479427648DB5F26162A01A6D650F93F2A62E55`

Pre-existing global scope remains stable 0.5.2: its synth inventory and sidecar previews do not include GRANULAR. The extension's LENS layout, three banks and real audio visualization are recorded in the local contract and new guide, without rewriting global typography, geometry, palette, narratives or component previews. No source or raster asset was edited by this pass.

Validation for this pass: checked the new local links and bilingual sections, verified parameter labels/ranges against source, and rechecked the two global hashes. Existing visual SHIP evidence remains separate from the unfinished hardware series.

Implementation follow-through: separate installation and hardware verification were subsequently completed by the parent. See docs/GRANULAR-NATIVE-v1.md and its English counterpart for both binary hashes, measured scenarios, normal-load timing caveat and preservation checks. The earlier documentation verdict remains scoped to its own pass.
