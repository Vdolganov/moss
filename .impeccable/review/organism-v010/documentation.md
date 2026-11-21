# ORGANISM 0.10 · documentation handoff

Status: documented after the supplied visual **SHIP** review, 18 November 2026. Scope is native ORGANISM VOICE and VOICE LOCKS only. This pass records the implemented local extension; it does not replace the incumbent design world or repeat the hardware assessment.

## Recorded surface

- Five visible banks, twenty fields, physical Y/B/A/X order, existing bitmap type, graphite palette, 18 px margins and four fields at y=346.
- Four intertwined neutral contours driven by actual carrier modulation, energy and envelope telemetry; the portrait is not a waveform or spectrum.
- Explicit GROW / FADE / REST states. Ordinary note-off or audition expiry follows RELEASE. STOP uses a 128-sample anti-click fade, then stationary REST; a full RELEASE tail on STOP is not claimed.
- L1/R1 for VOICE banks, L1+arrows for nested VOICE LOCKS; visible units, numeric values, LOCK ONLY, RESET and `--` retain their meanings.
- No new palette, type, spacing primitive, shipping raster or replacement browser component.

## Files updated

- `design/current/native-organism-v010.md`: implemented English/Russian surface contract, twenty-field map, state distinction, provenance and limits.
- `DESIGN.md`: one scoped native ORGANISM subsection in Components, including the Voice Portrait rule. Existing frontmatter, browser guidance and historical sections are retained.
- `.impeccable/design.json`: matching native motion extension and verbatim narrative rule, plus dated title metadata. Existing primitives, component previews and browser motion are retained.
- `native/prototype/README.md` and `README.en.md`: current navigation references now name instrument banks and distinguish five-bank MONO/ORGANISM/GRANULAR from two-bank FM. Only these navigation descriptions changed in the manuals.

## Evidence used

Read PRODUCT.md, DESIGN.md, the approved surface brief, the bilingual ORGANISM guides, native portrait/mailbox source, provenance and [review.md](review.md). The documenter inspected the contact sheet. The reviewer separately opened all eight complete 640×480 captures and verified all eight hashes against [provenance.json](provenance.json): CORE, TONE, MOTION, VOICES, ENVELOPE, idle, release and locks. The captures are deterministic native-renderer QA fixtures, not shipping art. No new image assets were created.

No visual repairs were required by the SHIP review. Still captures do not establish animation cadence, sound quality, all parameter extremes, held-button behavior or hardware performance. Device results remain owned by `docs/verification/organism-v010/` and are not asserted here.

Documentation checks: JSON parses, the new narrative rule matches DESIGN.md verbatim, all ten existing component previews remain present, the eight capture hashes match provenance, and the scoped tracked-file diff has no whitespace errors. No code or runtime test was needed for these documentation-only edits.

## Scope notes and remaining drift

The historical browser ORGANISM routing bars and native 0.5.2 CORE/CHARACTER descriptions remain intentionally unchanged; the new subsection explicitly supersedes them only for native ORGANISM 0.10. DESIGN.md's older global version labels and PRODUCT.md's 0.9.0 summary were not refreshed in this bounded pass.

Both native README navigation paragraphs still call PATTERN's fourth outer bank CYCLE, whereas the current supplied locks capture and later manual text call it TIMING. Both manuals also retain “both parameter banks” in the track-duplication description. These are pre-existing general navigation/documentation drift outside the authorized ORGANISM-only corrections; report them separately rather than silently broadening this change.

## Русский

Задокументировано только нативное расширение ORGANISM: пять банков, двадцать параметров и портрет из четырёх контуров по реальной телеметрии. GROW / FADE / REST, отпускание ноты и короткое защитное затухание STOP описаны отдельно. Сохранены палитра, шрифт, геометрия, физические цвета кнопок, браузерный контракт и история. Визуальный SHIP не выдаётся за проверку звука или производительности устройства; оставшиеся общие устаревшие формулировки перечислены выше.
