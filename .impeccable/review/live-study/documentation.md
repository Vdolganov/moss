# LIVE study — documentation verification

Date: 2026-11-21. Result: documentation matches the scoped browser study after the small corrections below. This is documentation verification, not native implementation approval or a new visual-system specification.

## Authority and inherited-world decision

Reviewed `reference/document.md` from the impeccable skill, root `PRODUCT.md`, root `DESIGN.md`, root `README.md`, the LIVE surface brief and both LIVE guides, `visual prototype/current/README.md`, `live.html`, `live.css`, `src/live.js`, and `src/live-model.js`.

This is an ordinary extension of the approved monochrome MOSS world. `DESIGN.md` (approved 2026-11-20) remains the visual authority; the root README identifies current native MOSS 0.13.1 and MS18. `PRODUCT.md` still contains older version headings and historical palette/navigation information. Those are not a reason to restore old colors or rewrite the approved system for this study. The LIVE brief explicitly defers native implementation until visual selection.

No refresh, replacement, or merge of `DESIGN.md` was needed or performed. No system-wide LIVE composition or new design token was promoted into it. `.impeccable/design.json` was also preserved. Surface-specific diagrams and button geometry remain documented in `design/current/live-study.md`.

## Evidence checked

| Documented behavior | Implementation evidence | Result |
|---|---|---|
| Three silent compositions share one draft | `live.html` offers Console, Score and Loop and says the demonstration is silent; `live.js` holds one `LiveSession` and changes only `variant`; the schema and `moss-live-design-1` persistence are isolated | Match |
| Score is preferred/default, pending final visual choice | `live.html` selects Score; `live.js` initializes `variant = 'score'`; the brief and prototype README state preference without native-port approval | Match |
| Connected D-pad and round physical face buttons | `hardwarePads()` builds a single cross silhouette and circular Y/B/A/X controls at left/bottom/right/top; Score calls that function | Match |
| Actual pitch rows, including older recorded pitches | `pitchRows()` combines the mapped and recorded MIDI notes; `score()` locates each event by `e.note`, labels each row by the same pitch, and displays HIGH/LOW counts | Match |
| Held pitch, label and gate survive octave changes | `changeOctave()` updates future mapping without releasing held inputs; each hold stores its own note; `padNote()` favors that stored pitch | Match |
| 16 or up to 64 cells, default 64, with automatic pages and partial tail | HTML defaults to 64; `livePage()` derives the page from transport modulo the track cycle and clamps its span; the 304-tick example gives 76 steps and pages of 64 + 12 | Match |
| Visual metronome is optional and persistent | Default is off; `toggleMetronome()` changes only that setting; the header shows four common-tempo beats and first-beat emphasis; serialization retains the setting | Match |
| Tab+E / SELECT+R1 does not transpose | `action()` consumes R1 for the chord and suppresses its ordinary octave change on release | Match |
| Tab+Space / SELECT+START toggles recording | `action()` routes START with SELECT to `toggleRecord()`; the model starts from zero only when stopped and leaves playback running when recording ends | Match |
| No native functionality claim | Both guides explicitly identify a silent browser study; native event storage, format migration, monitor-only click, latency and device checks are future gates; no native file is part of this documentation pass | Match |

Read `.impeccable/review/live-study/checks.json`: all recorded browser assertions are true and `pageErrors` is empty, including held octave, actual pitch rows, 64-cell auto-follow, partial page, page wrap, metronome and no metronome transposition. Read the corresponding model/browser test assertions. The parent task reports 41 passing model checks, including 15 LIVE checks; this documentation pass did not rerun tests or claim an independent hardware/audio validation. Screenshot artifacts are present in the same review directory; the concurrent visual reviewer owns visual disposition.

## Exact documentation corrections

- `design/current/live-study.md`: replaced Console's unconditional “two rows” with two rows at 16 cells or up to four compact rows at 64; replaced Score's generic compact maps with the connected D-pad and circular physical layout; clarified that pressing a note changes pitch focus and idle octave selection recenters it; narrowed held-note preservation from arbitrary settings to octave changes and stated that root/scale/BPM preparation releases live input while preserving recorded pitches.
- `docs/LIVE-STUDY.md`: added Score's current preference/default status, connected D-pad, large circular buttons and physical X/A/B/Y arrangement, explicitly leaving final selection pending; completed the selectable cycle list with 76 and 128.
- `docs/LIVE-STUDY.en.md`: made the same default/geometry/pending-selection and cycle-list additions; removed the obsolete “small” description of the Score button maps.

The guides already correctly document recording, optional visual metronome, auto-follow pages, actual pitch rows, octave hold preservation, silence, and the future native boundary. Those statements required no behavioral change.

## Remaining notes

No unresolved documentation discrepancy blocks comparing the three variants. The historical PRODUCT version header remains outside this task. LIVE's local CSS has extra neutral shades for panel/line contrast; this pass records no decision to change global palette tokens. The older review artifact may temporarily refer to the pre-fix pitch-row and octave-release problems while the final visual reviewer refreshes its disposition; the current source and checks inspected here contain the fixes.

Preserved authority file SHA-256 values, verified before and after this pass:

- `DESIGN.md`: `2E8F3633D687F9D179842545C1DDC6F0C0415D92980BF30EA06B91109AF53561`
- `.impeccable/design.json`: `3F1E8FB2E1C39A463EE16A5DC87CC0B12CF31042A7D3EB25DD629EF9B90CE226`
