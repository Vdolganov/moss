# Interface Lab 0.12 · documentation audit

Date: 2026-11-19. Scope: the silent browser surface at `visual prototype/current`. This pass records the implemented extension; it does not regenerate the global design system.

## Files updated

- `design/current/interface-lab-v012.md`: implemented palette, native bitmap provenance and type sizes, fixed geometry, responsive scaling, component behavior and scoped finish evidence.
- `visual prototype/current/README.md`: regression/capture commands, bilingual verification scope and the actual JSON/WAV transfer limitation.
- `visual prototype/current/PARITY.md`: persistence and scaling adaptations, targeted regression coverage and evidence table.
- `.impeccable/review/interface-lab-v012/documentation.md`: this audit record.

## Evidence checked

| Area | Implementation evidence | Finding |
|---|---|---|
| Palette and physical order | `style.css`, `src/model.js`, `src/views.js`, `hardware lab/src/lab_ui.h` | Neutral graphite and the four native physical colors match; Y/B/A/X maps to Z/X/C/V. |
| Typography | `style.css`, `tools/make-font.py`, native glyph patterns, `src/views.js` | Original 5×7 patterns become the bundled WOFF2; 14px text and 21px primary values, with a 14px long-value fallback. |
| Geometry and adaptation | `style.css`, `src/views.js`, scaling in `src/app.js` | Logical 640×480 instrument, four fixed fields, flat square controls; whole-device fitting and a 700px wrapper breakpoint. |
| Parameter contract | `src/native-contract.json`, `tools/export-contract.cpp`, `src/fields.js` | Five instruments and five effects; native header export provides definitions, and browser display conversions preserve physical units. |
| Navigation and feedback | `src/input.js`, `src/app.js`, `src/views.js`, `index.html` | Shared modifier gestures, acceleration, focus exit, blur cleanup, naming panel, pointer selection, visible held states and units. |
| Persistence and simulation | `src/storage.js`, `src/model.js`, `src/app.js`, `src/visuals.js`, wrapper disclosure | Separate IndexedDB; JSON excludes original WAV blobs; new imports do not reconnect earlier cuts. Actual WAV samples support visual inspection; capture and portraits remain simulations. |
| Test instructions | `package.json`, `server.mjs`, test scripts | Model, browser, regression and capture commands exist; local server binds to 127.0.0.1:4174 by default. |

## Verification

`npm test` was rerun during this audit: **16/16 passed**. Existing `work/interface-v012/browser-tests.json` and `regression-tests.json` report **17/17** and **7/7** passed, with empty runtime-error arrays; these browser runs were inspected, not repeated by the documenter. The browser suite includes a trap for attempted audio construction.

The independent final `review.md` records all **23 captures valid**, all **six scored findings resolved**, and no observed regressions from the correction batch. Its **ship** verdict is limited to those findings. This audit checks documentation against code and that evidence; it does not add an independent visual approval, exhaustive parity claim, DSP validation or hardware test.

## Preserved system and uncanonized drift

`DESIGN.md`, `.impeccable/design.json` and `PRODUCT.md` were read for context and left untouched. They retain earlier browser/native version descriptions and older browser typography/slot semantics. This is pre-existing version drift outside the authorized write boundary; the scoped brief identifies current 0.12 behavior without rewriting global history. Utility workbench heading/focus styles, declared but unused tiny text, and synthetic portrait details are not promoted into new global design rules.
