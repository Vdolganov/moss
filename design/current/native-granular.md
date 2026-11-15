# GRANULAR / LENS — native extension

User selected the browser LENS study on 13 November 2026 and requested a working instrument.

THESIS: One prepared sample becomes a sustained, sequenced cloud; the lens shows each grain's source extent and actual reading.

OWN-WORLD: Preserve the MOSS graphite 640×480 renderer, bitmap labels, physical Y/B/A/X colours and existing VOICE/PATTERN/SPACE hierarchy. No glow, depth, decorative motion or performance timers.

STORY: Choose GRANULAR, assign/import one sample, audition, shape four banks, sequence notes and per-step locks. Source remains capped at1MiB,8grains/track. START/END constrain births and each new grain's reading; existing grains retain their bounds. Pattern notes transpose relative to C3; LENGTH holds the ASR gate.

FIRST VIEWPORT: Instrument/source above a circular peak waveform. Excluded source regions are dim. Stable nested lanes carry each grain's source arc and actual read square; envelope brightness communicates life without moving the lane. A hollow outer square is the scan head. Four fixed parameter fields below. No audio means no invented grain movement.

FORM: L1/R1 = PLAY/SCAN/TEXTURE/ENVELOPE; Y/B/A/X+arrows edits, horizontal fine/vertical coarse. A assigns when empty, otherwise auditions. X toggles FREEZE without stopping grain births. Pattern VOICE LOCKS uses the same16parameters and L1+arrows for nested banks. Existing FX and TAPE flows are reused.

FINISH: Build both platforms; test persistence, asset protection/import, locks/gates, SONG and real device maximum load. Review native captures at640×480; preserve global design tokens. Browser study remains a separate silent exploration.

## Ambient revision · 0.6.4-dev (current)

SIZE spans 1–8000 ms. Actual births are evenly spaced at min(requested DENSITY, eight / duration), and G/S reports that effective rate. Note retriggers preserve scheduler phase. The eight-voice and 1 MiB source limits do not change. SIZE never replaces the step's note LENGTH. Audition allows two grain lengths plus NOTE IN, bounded to 2–20 seconds, followed by NOTE OUT.

TEXTURE / B JUMP replaces timing IRREGULAR without adding a bank or changing gestures. It controls random displacement width within START–END, at independent randomized 0.75–1.75 second intervals. SPEED continues between jumps. FREEZE pauses both head motions; births and reading continue. SCATTER remains a separate birth-distribution parameter. At full scatter, the head remains visible even though births no longer depend on it.

Each grain retains birth, source range, pitch, duration, envelope and visual lane until it finishes. Its arc wraps within its latched range; full passes show the entire arc and LOOP GRAINS. The solid marker is sample position, not an age animation. The waveform outside current bounds dims; old grains may legitimately finish outside newly narrowed bounds. Empty, stopped, frozen, full-cloud, long-loop and partial-region views share the renderer. Colours and navigation remain unchanged.

New persistence is MS10. Older IRREGULAR values/locks are cleared on load instead of reinterpreted as jumps; all other compatible data survives. Loading does not overwrite originals. Current guide: docs/GRANULAR-INSTRUMENT.md; qualification: docs/AMBIENT-LENS-v1.md.

## Cloud revision · 0.6.1-dev (history; visual rules superseded above)

SCATTER replaces millisecond JITTER with 0–100% of the selected region. At full scatter, prioritize unoccupied birth regions and hide the common anchor marker; label FULL CLOUD / ALL REGIONS. The active parameter locks feed displayed bounds and mode. STEREO means independent latched grain pan. PLAY and SCAN retain their existing controls; TEXTURE ends with LEVEL. ENVELOPE contains GRAIN IN / GRAIN OUT / NOTE IN / NOTE OUT. Grain fades use percentages of SIZE, note fades use milliseconds. Whole-source, partial-range, zero-scatter, empty, held, lock and envelope states use the same renderer and colors.

Parameter-only step events remain a documented proposal, not a shipping state. Global DESIGN.md and .impeccable/design.json remain authoritative and unchanged; this is an extension, not a visual rebrand.


## Native 0.6.5 · LOCK ONLY / REPEAT

PATTERN keeps four banks: STEP / VOICE LOCKS / FX LOCKS / TIMING. TIMING uses Y CYCLE, B STEP, A REPEAT, X TYPE. Repeat is a whole-track audio fragment (OFF / 1/8 / 1/16 / 1/32 / 1/64), before FX; it never calls noteOn. LOCK ONLY is an outlined diamond and LOCK label; note-only fields show NO NOTE. RESET is visually distinct from `--`; the latter adds no event. Existing button colors, graphite palette and navigation remain unchanged. [Current behavior](../../docs/TRIGLESS-LOCKS-PLAN.en.md).

## Native 0.6.6 · SWING / SHIFT

TIMING has two nested pages, selected by L1+arrows and shown as 1/2 or 2/2. Page 1 preserves Y CYCLE / B STEP / A REPEAT / X TYPE; page 2 uses Y SWING / B SHIFT / A REPEAT / X TYPE. WHOLE TRACK and STEP LOCK distinguish scope; signed percentages distinguish advance/delay. Y and B keep their established physical colors. CLEAR STEP SHIFT only removes local timing, and a timing-only empty step remains silent. Shared sampler SHIFT covers all slots at one timestamp. The footer names TIME PAGE; sampler-slot navigation remains in other banks. No new modal, visual theme or diagnostic value. Current behavior: docs/SWING-v1.en.md.
