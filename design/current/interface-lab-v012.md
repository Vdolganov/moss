# Interface Lab 0.12

Mode: **Operate**. The user requested an interactive frontend reproducing the current MOSS application for subsequent design work. Audio engines were explicitly excluded. Preserve the current native identity and behavior as the baseline; do not redesign or deploy native code in this task.

Surface: `visual prototype/current/index.html`, `style.css`, `src/`.

Authority: current native 0.12 sources and native screenshot fixtures, particularly `.impeccable/review/fm-v012/device-fm.png`, the current native granular/ORGANISM and REEL views, plus generated `src/native-contract.json`. Old v0.6 browser code is historical only. Existing global DESIGN.md and `.impeccable/design.json` contain older browser/native descriptions; this task preserves those global files and records the specific surface here.

Behavior and adaptation boundaries: see `visual prototype/current/PARITY.md`. The new folder preserves the earlier visual prototype and all native/device files. Finish review is scoped to this browser surface; do not infer whole-repository approval.

## Overview

This is a scoped record of the implemented extension, updated on 2026-11-20. The inherited direction remains neutral dark graphite: a dense button-operated instrument, physical Y/B/A/X fields, technical portraits, and explicit state. The browser workbench supplies help, files and scenarios around it. No new visual identity or audio engine is introduced.

## Colors

The following neutral CSS values match the native palette in `hardware lab/src/lab_ui.h`. Shared parameter fields, sample slots and TAPE waveform regions are monochrome.

| Role | Implemented value |
|---|---|
| Instrument background / panel | `--bg: #191a1c` / `--panel: #222326` |
| Rules / primary text / secondary text | `--line: #414348` / `--text: #e8e8e6` / `--dim: #a9abb0` |

**Physical Button Rule (updated 2026-11-19).** Bottom parameter fields are monochrome in every state. Each field has a 24px SVG face-button cluster: Y left, B below, A right, X above. Its associated circle is filled; the other three are outlines. A 12px letter above the cluster reinforces the mapping. To its right are a 12px neutral label, 14px value and 12px unit. Held and pointer-selected fields use neutral outlines; no per-button color enters the field. Sample slots also use monochrome selection and playback cues. TAPE regions use neutral selection with button labels instead of accents.

The utility workbench uses a paper background (`#ededeb`), dark text (`#28292c`) and secondary text (`#606165`). Its browser focus outline (`#77797e`) is a workbench affordance, not a replacement instrument accent.

## Typography

The instrument loads `assets/moss-bitmap.woff2` as `Moss`, with a monospace fallback. `tools/make-font.py` converts MOSS's original 5×7 glyph patterns from `hardware lab/src/lab_ui.h`, using a 700-unit em and 600-unit advance. At logical size 14px the glyph is 10×14px with 12px advance; at 21px it is 15×21px with 18px advance.

User-approved typography: three primary sizes across the instrument, canvas annotations and browser workbench, plus a scoped 8px micro role for duration-line capsules. `--font-heading: 21px` is for screen, instrument, effect and dialog titles; `--font-body: 14px` is for navigation, track and list names, menu items, keyboard characters and parameter values; `--font-small: 12px` is for parameter labels, units, metadata and control hints. Values no longer switch sizes according to string length. Text rows use 16px line-height in the instrument (24px for headings) and 1.5 in the workbench. Canvas labels read the same CSS tokens; numeric readouts use the body size. The added `--font-micro: 8px` is reserved for duration-line capsules; ordinary labels and hints stay at 12px. Proportional device zoom scales all roles together. This is an intentional design-prototype change from native typography; the native application is not changed in this iteration.

## Layout

The device and canvas retain a logical 640×480 rectangle. Shared geometry lives in `src/layout.js`, derived from the CSS spacing scale. Content uses 16px side insets and a 608px width. The header rule is at y=48; main/library tabs occupy y=56–80, followed by the rule at y=88. Local MIX/TAPE tabs occupy y=104–128.

The 36px service footer is anchored to the bottom edge (y=444). The common parameter area is anchored 24px above it: y=376–420, with four 152px columns ending at x=624. Screens with fewer parameters retain those column positions. The letter and 24px face legend occupy the left column; text starts 32px into each field, leaving an 8px icon/text gap and 120px for text. Label line: y=376–392; value line: y=400–416. Units never create a third row. Parameter tabs occupy y=336–360, 16px above the fields; their reserved portrait area ends at y=320, at least 16px before navigation. This replaces the oversized three-line field container.

Use the same bottom anchoring for PATTERN, VOICE, SPACE, MIX, TAPE and SONG. Browser status text and supplementary context can occupy the reserved navigation row when a page has no parameter tabs. Lists use 8px row gaps; dense dialog menus use 4px. 16-step grids, sample slots and the naming keyboard use 8px gaps; 64-step pages use the compact geometry below. The mixer fits all four project tracks. Normal list rows are 48px high, compact rows and keyboard keys are 40px. Dialog outer insets and inner padding are both 16px.

The workbench has a maximum width of 960px. Scale choices are 1×, 1.5× and explicit fit-width overview. Narrow windows scroll the viewport at 1×/1.5× rather than forcing unreadably small text. Below 700px, the top wrapper margin becomes 16px, the key guide becomes one column and utility rows wrap. The instrument does not reflow into a separate mobile layout. These dimensions describe this port and do not supersede other MOSS surfaces.

## Elevation & Depth

The instrument uses flat tonal layers, thin rules and outlines without shadows. Overlays cover the instrument with the same dark background and a secondary-text border; a light toast reverses foreground and background for transient feedback. Portraits are canvas drawings beneath the overlay layer.

## Shapes

Controls and fields keep square corners except rounded navigation selections (12px radius) and 16-step grid cells (4px radius). Compact 64-step pages use 2px cell corners. Selected rows use a panel fill and inset one-pixel outline. A selected step changes its existing one-pixel border to the primary text color, with no external outline. Playback inverts the step number in large cells or the caption in compact cells. Held fields use a neutral outline and panel background.

## Components

**Persistent Unit Rule.** A field keeps its label, exact value and compact unit while held. Units share the value row: `1 stps`, `100%`, `250 ms`, `440 Hz`, `12 st`, `4 gr/s`. Context already carried by the label or tab is omitted from the readout; the accessible name retains the full descriptor. For a long exact position that cannot fit beside its unit, the unit moves beside the label. Neither digits nor units wrap or shrink. The common abbreviations live in `src/fields.js`; native descriptors and saved values are unchanged. `src/fields.js` formats native parameter values and numeric locks; REEL/POND conversions and MOVE/TRIM/FINE units have targeted regression evidence.

The device is one keyboard focus target. Z/X/C/V map to Y/B/A/X; held face keys plus arrows edit in place, Q/E change banks, brackets change major tabs and Q+E opens options. Shift+Tab exits to the surrounding page; blur clears held keys. Pointer field selection, wheel editing, scenario shortcuts and screen buttons are browser adaptations. Names use the shared 40-key character panel, with a 24-character limit; an explicit TYPE ON KEYBOARD mode offers browser text entry.

**Silent Reference Rule.** Transport, previews, capture and instrument/effect portraits represent interface state. Imported WAV samples supply real waveform detail, but capture sources are virtual and portraits are illustrative. Neither is DSP telemetry. The wrapper states these limits, and persistence uses a separate browser database.

## Do's and Don'ts

- Do preserve the physical button order, visible units and native parameter contract when iterating this surface.
- Do document a browser convenience or intentional visual deviation in `PARITY.md` before considering a native transfer.
- Don't interpret synthetic portraits, silent capture, or saved waveform metadata as audio processing.
- Don't promote this surface's geometry, wrapper styling or historical global-document drift into a new repository-wide system.

## Scoped finish evidence

The model suite passes 16/16; saved browser and regression reports record 17/17 and 7/7 with no runtime errors. The independent review at `.impeccable/review/interface-lab-v012/review.md` marks the six scored corrections ship after checking 23 valid captures. This is evidence for those corrections only, not whole-surface, native, or hardware approval. See the adjacent review `documentation.md` for the documentation audit sources and limits.

## Monochrome parameter-field verification

Compared 20px, 24px and 28px face glyphs at native logical size; selected 24px. Checked 32 fields across PATTERN, VOICE, MIX, SPACE, TAPE and SONG: one correct filled circle per field, neutral text in all states, physical key editing preserved. Long-label/ten-digit-value checks had no overflow. Evidence: `work/interface-v012/face-fields/checks.json` and the companion captures. This is a browser design iteration, not a native UI deployment.

Parameter-block typography clarification (2026-11-19): the entire field defaults to the smallest 12px role, with only its value explicitly at 14px. STEP/VOICE LOCKS/per-effect LOCKS/TIMING tabs and instrument/effect parameter-bank tabs also use 12px. Main navigation retains the 14px role.

Compact-unit verification (2026-11-19): 150 parameter-bank/state combinations, 598 fields, including native range limits and long source positions. No text overflow or third row; values remain 14px and units 12px. Physical held-key editing passed. Evidence: `work/interface-v012/compact-units/checks.json` and rendered captures.

## Parameter-page navigation (2026-11-19)

PATTERN, synth/granular VOICE and track/master FX share a dedicated page selector at x=16, y=336, width 608px, height 24px. Its compact intrinsic-width labels form one centered group rather than aligning one-to-one with the four fields. L1/R1 controls occupy 48px at either edge; all labels use 12px. The selected page has a neutral light fill (`--text`), inverted dark lettering (`--bg`), a 12px radius, and no underline. Other pages remain dim text, with a neutral hover state. Pages with one bank omit the shoulder affordances. Direct selection and shoulder clicks use the existing navigation behavior.

The first parameter label begins at y=376, leaving 16px after the page selector. PATTERN context, pagination and the 16 / 64 selector share a 24px row 8px below the grid, with text vertically centered 4px into that row. Previous/next arrows and the page count sit beside the size selector. Sampler context ends at y=320, leaving 16px before parameter tabs. `VOICE LOCKS` and `TIMING` display as `V-LOCK` and `TIME`; effect-specific tabs retain full names such as `ORBIT LOCKS`. Accessible names retain the full titles. Portraits use a shared lower boundary at y=320. Their size and placement account for the live drawing, including granular markers and both track/master REEL.

Earlier behavior verification: 45 bank/screen states across PATTERN, sampler, all synths, all track FX and all master FX: no overlapping page names, one active page, 12px lettering. The current spacing pass supersedes the earlier geometry. Keyboard, direct page clicks, shoulder clicks and the 17 browser workflow checks pass. Captures include native logical size and a 390px browser viewport. Evidence: `work/interface-v012/page-navigation/`.

Primary navigation update (2026-11-19): GRID/MIX/TAPE and PATTERN/VOICE/SPACE use the same centered, rounded selection as parameter pages: 24px height, 12px radius, 12px horizontal padding and 4px gaps. Main navigation retains its 14px type and L2/R2 controls; the 24px tab row sits between 8px vertical insets. Active tabs have a light fill (`--text`), dark lettering (`--bg`) and no underline. Verified all six active states, pointer selection and bracket-key navigation; evidence: `work/interface-v012/major-tabs/`.

## Overflowing navigation strips (2026-11-19)

Both main and parameter navigation now share a horizontal viewport, with fixed L2/R2 and L1/R1 controls outside it. Tabs retain their type sizes, height and spacing; main tabs size to their text with 12px horizontal padding. Fitting lists are centered and unmasked. Overflowing lists expose clipped neighboring tabs through a 24px transparent edge mask, enabled independently on each side only when content remains hidden there. A single-page list omits its shoulder controls.

On selection, the strip scrolls only when needed to bring the whole active tab outside the faded area, using a 32px safe margin and a 140ms eased movement. Reduced-motion preference makes this immediate. Scroll positions and in-flight movement survive ordinary screen HTML replacement. Wheel/trackpad/touch scrolling is supported; a click on a visible fragment selects and reveals that tab. Size and font changes recalculate overflow. The shared controller is `src/tab-strips.js`; main and parameter variants use the same markup and edge behavior.

Scope: this is navigation within the current UI-only prototype. No new application pages or parameter banks were added to create artificial overflow. Verification uses the real lists in a narrower test region, plus a separate 30-item fixture. Captures in `work/interface-v012/tab-overflow/` distinguish normal and constrained layouts. `npm run test:tabs` checks first/middle/last masks, wraparound, fast input, preserved scroll after edits, partial-tab clicks, manual scrolling, resizing and reduced motion. Existing 17 browser workflow checks also pass.

Shell overflow-test control (2026-11-19): a pressed-state toggle above the device narrows both strips to 360px (240px for their scroll viewport), centered in the unchanged 640px screen. A visible shell note explains PATTERN/VOICE and keyboard navigation. Enabling returns keyboard focus to the device; disabling restores the regular geometry. The flag is not persisted or included in project data. Toolbar actions wrap at available widths. Verified the real toggle, unchanged model data, both navigation controls, restored layout and the 390px shell; screenshots: `work/interface-v012/tab-overflow/shell-test*.png`.

Rounded main-navigation refinement (2026-11-19): both tab groups share their shape, spacing and active-color rules. Verified centering, inverted selection, 24px height and 12px radius, plus all seven overflow interaction checks. The user-approved rounded main tabs replace the earlier rectangular variant.

## Unified navigation coverage (2026-11-19)

Library SEQUENCES/SONGS, MIX LEVELS/MASTER FX and every TAPE section now use the same `tabStrip` renderer, `.nav-tab` styles and overflow controller as the main and parameter navigation. The old equal-width/underlined tab renderer and active-tab underline rule were removed. All selected tabs have a light fill with dark lettering, 24px height and 12px radius; groups are centered with 4px gaps. Non-parameter labels use 14px; parameter labels use 12px. Library is at y=56; local MIX/TAPE sections at y=104; both tab rows are 24px tall.

L2/R2 select library/main sections; L1/R1 select local pages. In the master-effect editor, local MIX shoulder hints are hidden because L1/R1 belong to the bottom effect banks; B returns to the chain, and the two MIX tabs remain clickable. This preserves existing keyboard behavior without advertising two simultaneous meanings for the same controls. Mixed TAPE actions retain their IDs, and the overflow controller identifies selection by action plus value so FINE and LIBRARY cannot be mistaken for the same page.

Evidence: 17 browser scenarios, seven earlier regressions and nine navigation checks passed. New navigation checks cover library controls, MIX chain/detail transitions, context-sensitive shoulder hints and every TAPE page in manual/four-part modes with overflow. Captures: `work/interface-v012/tab-overflow/library.png`, `mix.png`, `master-detail.png`, `tape-four.png`, `tape-manual.png`, `tape-overflow.png`. This remains a browser-only design iteration.

## Shared spacing system (2026-11-19)

The approved scale is **4 / 8 / 12 / 16 / 24 / 32px**, defined once as `--space-1` through `--space-6` in `style.css`. `src/layout.js` reads that scale, derives the common device geometry and publishes shared CSS anchors. DOM and canvas use the same boundaries. Border widths, font sizes, control dimensions and drawing geometry are not spacing tokens.

| Space | Use |
|---|---|
| 4px | Tab gaps, letter/icon gap, value/unit gap, dense menu gaps |
| 8px | Label/value and icon/text gaps, grid/slot/keyboard/list gaps, compact padding |
| 12px | Row and tab horizontal padding, shoulder/viewport gap, toast padding |
| 16px | Screen/dialog insets, related group separation, portrait/page and page/field clearance |
| 24px | Parameter block/footer clearance; shell heading gap |
| 32px | Large browser workbench section spacing and desktop outer margin |

The browser workbench uses the same scale: 16px side insets and toolbar gaps; 8px button gaps; 8×12px button padding; 16px spacing around toolbar/transport; 32px between larger sections. Help, scenario controls, overflow-test notes and small viewports use these tokens too. Device zoom remains proportional.

The original audit is retained as historical evidence only at `work/interface-v012/spacing-audit/`. Its old coordinates and spacing inventory are superseded by the Layout and Shared spacing system sections above.

Verification: `npm run test:spacing` covers 29 screen/state layouts, consistent footer anchoring, list/grid gaps, all four mixer rows, seven-item menu pages, the naming keyboard, 120 live/extreme portrait frames, and the shell at 390/800/1200px. Measurements and current captures: `work/interface-v012/spacing-system/`. The additional unit check covers 150 bank/state combinations and 598 fields, including exact ten-digit source positions, without overflow or third lines (`spacing-system/units/checks.json`). Model 16/16, browser workflows 17/17, targeted regressions 7/7 and navigation 9/9 pass. These checks concern the browser prototype only.

## Grid-cell states (2026-11-19)

PATTERN and sampler grids share the same cell renderer and visual language. In 16-step mode, existing cell dimensions and 8px gaps are preserved; all corners use a 4px radius. The compact 64-step mode is specified below. Three independent cues represent the event, editing cursor and transport position:

| State | Appearance |
|---|---|
| Empty | Dark background, subdued border and 12px step number; no `--` placeholder |
| Note / sampler hit | Gray `#34363b` fill, primary-color 14px note name or `HIT` |
| Lock only | Dark background, 14px `LOCK` label, outlined diamond at top right |
| Note with locks | Note fill and label, plus the same diamond |
| Editing cursor | Existing one-pixel cell border becomes `--text`; no double outline or geometry change |
| Playback position | The 12px number has a `--text` background and `--bg` ink; other state cues remain intact |

The top row is 16px high, with a 32px number area and a 12px SVG diamond. Three-digit step numbers fit beside the lock icon. The diamond follows active event data: voice/sample locks, FX locks, SHIFT and REPEAT, including explicit zero/OFF and RESET. Null inherited values and disabled type-0 events do not light the indicator. An explicitly selected lock-only event retains its icon even before values are assigned. Sampler lane locks are local to that lane; shared track lock events remain visible on each lane and do not replace a simultaneous HIT fill.

The old normalized per-cell duration bars and FX asterisks are removed. Exact LENGTH remains in the selected step's parameters; the connected duration layer is specified below. The small finer-event dot is preserved beside the number when a coarser resolution hides events; it also accounts for the selected sampler lane. There is no glow, color coding, pulse or tween on transport movement. Accessible names distinguish event kind, locks, finer events, cursor and playback, with `aria-current="step"` on the playing cell.

Verification: seven `npm run test:grid` checks cover state combinations, reset/zero/null locks, sampler lane/shared events, cursor plus playback, stop, 15-step wrap, page boundaries, 512-step numbering, glyph clicks and keyboard editing. Browser workflows 17/17, regressions 7/7 and the 29-layout spacing suite pass. Captures and results: `work/interface-v012/grid-cells/`. This is a browser design iteration; native code and device data are untouched.

## Long-note continuation and sampler activity (2026-11-19)

The grid draws a neutral 2px duration line centered 12px above the bottom edge of the relevant cells. A dot marks a trigger; a vertical cap marks the end. Lines pass through lock-only steps unchanged, and a later trigger in the same voice ends the previous planned span. Row/page/loop continuations use incoming and outgoing arrows; no page-local fake endpoint is introduced. The line overlay uses pointer-events:none and preserves cell sizes and grid gaps. The ordinary event row ends 4px above the line center, keeping note/HIT/LOCK text clear of the stroke. This places its top at 24px in a sampler cell and 32px in a taller synth cell. Single-cell notes need no extra line. Partial-cell endpoints retain their timing when the grid resolution changes.

At the first cell of a page carrying a previous note, the note name appears directly on the line inside an oval capsule: 8px text, 12px height, 6px radius, and 24px width for two characters / 32px for three. Capsule fill and line stroke both use `--duration-ink` (the neutral `--dim` color); text uses the dark `--bg` color. The capsule is centered exactly on the line, leaving 6px between its lower edge and the cell bottom. It has no shadow, outline, glow or extra arrow; the line's incoming arrow already supplies direction. This cell stays dark, with no trigger dot. A coincident lock keeps its upper-right diamond; the event row remains empty instead of duplicating LOCK. Other lock-only cells retain LOCK. Editing cursor and playing-number styles remain independent.

A carry too short to fit a complete capsule plus clearance keeps its actual endpoint visible and omits the badge. Its full note/source information remains in the context line and accessible name. The line is never stretched to make a label fit.

The context line identifies the source note, its source cell and effective span length. GO TO NOTE returns to that origin without editing the continuation cell or creating another note. If a finer event is hidden by the current resolution, navigation selects its containing cell and explains that a finer resolution is needed for exact editing; it does not silently alter the project resolution.

For SAMPLER, the grid shows the selected lane only. Switching the four slots preserves the page and cursor. Each slot's note badge independently inverts while its voice is active; the selected slot has a separate light background with dark text. Audition belongs to the originally triggered slot even after selection changes. Playback and preview positions stop at the one-shot end, rather than visually looping.

`src/note-timing.js` derives planned spans and tracks silent sample activity from source frames/rate, START/END, pitch, FULL or gated LENGTH, and pitch/trim/RESET locks. Same-slot retriggers replace that voice; other slots remain independent. Planned spans assume written events fire and use nominal grid timing; they do not fluctuate with probability or model release/FX tails. Live activity follows accepted scheduler events, swing/SHIFT, actual tempo including SONG, sample completion, mute, reassignment and stop. This is UI timing, not an audio engine or native telemetry.

The workbench's **Длинные ноты / Long notes** action creates or reopens the editable LONG NOTES study: a 12-step MONO note beginning at step 13, plus a sampler with two overlapping virtual one-shots. Both tracks have 32 steps. Use Space to play, E+Right to visit page two, and slot selection to inspect both parts. The study uses separate synthetic sources and does not modify existing projects.

Verification: `npm run test:notes` passes 10 model checks for continuation, retrigger, cyclic spans, tempo/trim/pitch/gates, lock-only source changes, four independent voices, probability and SONG transitions. `npm run test:duration` passes eight browser checks for cross-page labels, lock combinations, simultaneous badges, source navigation, original-slot audition and narrow screens. Existing model 16/16, browser 17/17, regressions 7/7, grid 7/7, navigation 9/9 and spacing checks pass. Captures and report: `work/interface-v012/note-duration/`.

Duration-capsule refinement: 6px and 8px Moss lettering were compared at native 640×480; 8px was selected for the sharp sign and octave digit. `--duration-inset:12px` is shared by the DOM capsule and SVG geometry. `--duration-label-height:12px` defines its fixed vertical size. Captures and measured center alignment: `work/interface-v012/duration-capsules/`. The capsule uses the existing cell click action; it does not introduce an independent control. Checked sampler/synth continuation, coincident locks, short endpoints, original-note text clearance and a 390px shell. Existing duration interactions, grid-state and spacing checks pass.


## 16 / 64 steps per page (2026-11-19)

PATTERN has exactly two page sizes, **16** and **64**, selected on the right of the context row. The earlier whole-pattern ALL mode and text-free dense maps are removed. New browser sessions default to **64**; the choice is shared across track navigation for the session and does not enter musical data or exports. A manual choice of 16 stays selected until changed. Reload starts in 64.

Both buttons use the established 24px rounded inverted selection and 12px type. Previous/next arrows and the current page count sit immediately to their left, on both synth and sampler grids. Disabled arrows indicate first/last pages. Switching sizes preserves the selected cell, slot, parameter bank, transport and stored events; the destination is the page containing the same cell. A 512-step track has 32 pages at size 16 and eight pages at size 64. Final pages contain only their real cells.

`src/grid-layout.js` supplies rendering, duration geometry, vertical arrow stride and page size. The grid keeps its existing 608px width and height: 136px for synths, 120px beneath sampler slots. Sixteen-step pages use eight columns, two rows, 8px gaps and 4px corners. The 64 mode uses sixteen columns, up to four rows, 4px gaps and 2px corners. Tracks of sixteen steps or less retain large cells even when 64 is selected. A partial last page retains the geometry of the other pages; it does not stretch its remaining cells.

Compact cells retain 12px note/slot captions, numbers on empty cells and lock diamonds. Three-digit numbers fit without introducing another text size. Empty borders use #34363a; note fills #4b4e54. The selected border and inverted playing caption remain independent. Duration lines are 1px with smaller start/end/continuation marks, using the same cyclic spans as the large grid. They cross 64-step page boundaries correctly and omit crowded carry capsules; full continuation information remains in the selected-step context and GO TO NOTE. Large cells retain the existing 2px lines and 8px capsules.

Left/right navigate individual steps; up/down follow visible rows. **E+left/right moves by the selected page size (16 or 64)**. Pointer arrows do the same. Parameter modifiers and Q+arrows are unchanged. **Q+E → GRID VIEW → C** toggles the size through the first menu item; the equivalent device mapping is L1+R1 → A. No extra face-key action is introduced.

The GRID · 64 / 128 / 512 workbench studies remain editable and open at size 64 without autoplay. Existing sequences are preserved. The examples retain their eight-bar cycles at 1/8, 1/16 and 1/64; changing the view never changes musical division or duration.

`npm run test:pages` checks default 64, both page counts, cursor/data preservation, actual keyboard/pointer paging, step 512 editing, transport indication, timing changes, partial pages, three-digit captions, cross-page sustain, sampler lanes and the 390px shell. Captures and results: `work/interface-v012/grid-pages/`. The former grid-overview captures are historical only. This remains browser-only design work.


## Parameter navigation and sampler slots (2026-11-19)

`src/nested-pages.js` owns the navigation model. PATTERN has STEP, V-LOCK, one LOCKS tab per enabled track effect in chain order, then TIME. Example: STEP / V-LOCK / ORBIT LOCKS / REEL LOCKS / TIME. There is no generic FX-LOCK or concatenated cross-effect page stack. Effect identity is independent of tab index. Reordering preserves the selected effect and its parameter page. Bypassing removes its tab and falls back to STEP if needed; it never erases stored locks. Re-enabling makes those locks editable again. With no enabled effects, the three base tabs remain. Effect locks are shared across sampler lanes; TRACK in the context line makes this scope explicit.

Short L1/R1 turns horizontal tabs. L1+left/right selects a sampler slot in every pattern tab and sampler VOICE, or a TAPE part/slice. It never selects an effect or TIME/voice-lock bank. L1+up/down turns parameter pages within the current tab, wrapping previous/next. In synth VOICE, track/master FX editors and TAPE MOVE/TRIM/FINE it changes the visible parameter bank. Unmodified synth VOICE arrows retain note/octave audition. R1+left/right still turns grid pages. L1+R1 still opens options.

Hidden stacks in V-LOCK, effect LOCKS tabs and TIME use an 8×16px vertical-chevron SVG when inactive. The active rounded pill includes a 12px current/total counter and two 12×12px clickable chevrons. Counts refer only to this tab: ORBIT 2, POND 2, CRUSH 1, REEL 4, DRIVE 2. Single-page tabs omit counters/arrows. The vertical rail at x=4–12, y=376–420 has one 2px tick per page, inactive 4px wide and active 8px. Existing overflow masks reveal the whole active group including the arrows; long effect names remain complete and scroll into view. Visible flat editor rows need no duplicate rail.

TAPE uses a horizontal PART/SLICE selector at x=16, y=336, width184, with clickable 24px arrows. It reflects selection rather than parameter pages. Changing part through L1+left/right uses the common selection action, cancels its old preview and recenters FINE. Vertical arrows change MOVE/TRIM/FINE while retaining the selected part; manual EDIT has no other parameter pages. TAPE region appearance is defined in the monochrome waveform section below.

Holding L1 replaces the footer with available horizontal SLOT/PART/SLICE and vertical PAGE hints. These clear on release or blur and are hidden under dialogs, capture, face edits and tempo edits. A hold of at least 350ms does not turn a tab on release. Navigation does not write musical data or create undo checkpoints.

Sampler slots share the rounded monochrome selection language. Each 146×48px slot uses a 12px radius, 8px horizontal padding and a 4px internal gap. The note is 14px; the sample name 12px. Idle slots have neutral outlines; the selected slot has `--text` fill and `--bg` text. Playing slots invert only the 40×16px note badge: light/dark on idle slots, dark/light on the selected slot. Thus selection and simultaneous playback remain distinguishable. Names truncate with ellipsis; accessible names retain the full text and SELECTED/PLAYING state. No per-button color enters the slots.

Verification: `npm run test:nested` covers nine scenario groups: dynamic tabs, independent axes, effect reordering/bypass with retained locks, zero/single-page states, all editor contexts, TAPE/FINE, simultaneous sampler activity, overflow and narrow layouts. Browser 17/17, regression 7/7, navigation 9/9, duration 8/8 and spacing checks (29 layouts, 120 portrait frames, three shell widths) pass. Captures: `work/interface-v012/navigation-axes/`. These are browser-prototype changes only.

## Monochrome TAPE waveform (2026-11-19)

MOVE, TRIM, FINE and manual EDIT use the same neutral region renderer in `src/tape-visual.js`. Outside regions the wave uses #626469. Prepared regions have an opaque #222326 background and --dim wave; the selected region uses #34363a with --text wave. Identical opaque fills prevent overlap from accumulating brightness. The selected region is drawn last, preserving a clear waveform and boundaries. Source samples, slice positions, memory limits and editing increments are unchanged.

Labels occupy a 24px band above the wave, with an 8px gap. The selected four-part label is a 48×24px inverted rounded pill with the shared physical face-button glyph and Y/B/A/X letter; inactive labels are 12px letters. It stays visible for subpixel regions and is connected to its actual span by a short leader. Inactive labels yield when too narrow or colliding with the selected label. Manual slices use numeric labels (including 10–16), not repeated face assignments. All labels remain within the 608px content width.

Real active boundaries are 2px inward-facing brackets; the START or END being edited becomes 3px with a small center handle. MOVE highlights both. Preview is independent: a thin light line with dark clearance and a triangular head. It follows the original source-frame interval and disappears when expired, cancelled or outside the current view. Selecting another part and editing retain existing preview-cancellation behavior. In FINE, an offscreen boundary is represented by an outward chevron at the view edge, never a false START/END bracket. Panning entirely away from a cut preserves its label and shows the direction back to it.

Overview label/wave positions are y=176/208; FINE preserves its frame range at y=176 and uses label/wave y=208/240. Both end at y=320, keeping the existing context/parameter spacing. Stereo channels and imported per-frame waveform detail use the existing waveform reader. No audio engine or native code changes.

`npm run test:tape` checks nine groups: MOVE editing, four-button preview and expiration, START/END emphasis, FINE offscreen boundaries, subpixel cuts, overlaps, sixteen manual slices, real WAV detail/neutral pixels, and narrow shell. Captures and results: `work/interface-v012/tape-monochrome/`.

## Monochrome direction confirmed (2026-11-20)

The interface remains monochrome. Accent-colour studies were exploration only and are not approved tokens or implementation requirements. Preserve the current neutral selection, parameter, sampler-slot and TAPE-region styles; distinguish states through fill, outline, shape and contrast.

## Reliability contract · 2026-11-20

See [implemented audit fixes](ux-fixes-2026-11-20.md) for U01–U13. This record takes precedence over earlier browser behavior where the two differ; native behavior is unchanged.

- Musical transactions snapshot only an actual accepted change. One held gesture = one undo; navigation, no-op and failure do not overwrite it. Saves are serialized and acknowledged after completion; failed exit stays in the editing context.
- Draft cuts and prepared assets are distinct. Drafts persist by source ID; their status is shown in the workbench. SAVE KIT never silently replaces slots. Explicit one-slot/four-slot assignment preserves an atomic undo.
- Every lock field preserves the parameter's physical unit. -- means no event, a diamond marks a value, hollow diamond plus RST marks a base reset. Face + MENU and OPTIONS → PARAMETER expose SET/CLEAR/RESET. Bank operations remain distinct.
- L1/R1 selects visible tabs/banks, never a hidden row selection. D-pad chooses rows; L1 horizontal arrows choose slot/part; L1 vertical arrows choose nested pages. R1 horizontal arrows page lists/grids. Positions are remembered per track and effect context.
- Persistent header context identifies the track; step metadata labels slot/track scope. NOTE LEN, DIV, CYCLE and PAGE/16/64 remain separate concepts. Empty SONG slots do not expose a fictitious editable POSITION. Footer labels and pointer commands agree.
- Menus show scope, item count and overflow arrows; disabled commands explain the reason. Modals make the background inert and restore focus. Desktop typing requires an explicit mode, preserving console keybindings.

Verification: 19 targeted browser scenarios plus error monitoring, existing browser/regression suites, model and note-timing cases, 29 screen layouts and 120 portrait frames. See `work/ux-fixes-2026-11-20/` and the existing `work/interface-v012/` reports. Physical ergonomics and screen-reader testing remain separate checks.
