# Sample folders 0.11 · documentation handoff

Documented on 18 November 2026 after the supplied **SHIP** review of three corrected findings. This pass records the completed native extension; it neither changes the global visual world nor certifies deployment/hardware performance.

## Written

- [Scoped bilingual contract](../../../design/current/native-sample-folders-v011.md): import and prepared-library paging, four-part creation, MOVE/TRIM directions, audition/edit distinction, named-folder save, assignment scope, recovery, storage and evidence boundaries.
- This handoff records sources, preservation decisions and remaining drift. No source code, user guide, global design file or token was changed by this pass.

The contract explicitly preserves physical Y/B/A/X colours, flat graphite, bitmap type, neutral focus, visible values/units, fine left/right and coarse up/down editing, outer L1/R1 banks and R1+left/right pages. It records the reviewed corrections: clamped L1 navigation, L1+R1 OPTIONS, a named active stop button, destination track plus four-slot replacement scope, and the ineligible-track recovery instruction. TRIM taps still audition the four corresponding parts.

Save/load failure and undo semantics are included: no overwrite of existing folders, no publication of an incomplete four-file set, retained editing state on save failure, unchanged assignments on a failed load, and one undo restoring prior slots. Equal initial quarters are explicitly distinguished from the 1 MiB per-range cap. Catalog capacity is separated from resident PCM capacity.

## Sources and verification

Read PRODUCT.md, DESIGN.md, `.impeccable/design.json`, the document reference, `app_sample_folders.h`, `app_sample_folders_ui.h`, `sample_catalog.h`, `library_kits.h`, relevant input/service/render paths, the existing Russian/English SAMPLE-FOLDERS guides, the fixture source and [review.md](review.md). The existing context load was reused; it was not run again.

The documenter opened five complete 640×480 PNGs: `four-parts.png`, `trim-part.png`, `playing-part.png`, `kit-folder.png`, `kit-ineligible.png`. They support the recorded physical colour mapping, stable state labels, explicit stop action and destination/recovery copy. No image assets were created. The reviewer describes ten supplied recaptures; the folder also contains the later supplemental `device-folders.png`. That extra capture is not silently included in the earlier ten-image SHIP claim.

The fixture source contains assertions for paging, four auditions, movement, held MOVE/TRIM selection, named B STOP, atomic save/load, IDs above 31 and the unchanged cache bound. These assertions were inspected, not rerun by this documentation pass. The prior review's scope remains three scored fixes, not a fresh full-surface review. Hardware readability, sound, I/O latency and sustained performance remain outside this evidence; final deployment evidence is pending here.

Documentation checks: local Markdown links resolve, both new files are UTF-8 and have no trailing whitespace. DESIGN.md and `.impeccable/design.json` were not written in this pass. No runtime test is needed for these documentation-only additions.

## Preserved global records and drift

No global reference was needed: this ordinary extension reuses the existing primitives and conventions. DESIGN.md and its sidecar were preserved together without adding a new narrative rule or component preview.

PRODUCT.md's current native summary still names 0.9.0; DESIGN.md's global description and many headings describe browser v0.6/native 0.5.2, and the sidecar retains those historical labels plus later scoped extensions. These are pre-existing documentation/version drift, not permission to replace the global design record. The native ORGANISM 0.10 scoped contract remains authoritative for that separate extension. The documentation index's published stable 0.9 reference is not a publication error: no newer release publication is established by this pass. No unrelated drift was repaired.

## Русский

Создан локальный двуязычный контракт: папки импорта, страницы библиотеки, MOVE/TRIM, прослушивание четырёх частей, именованный набор, явная замена четырёх слотов, ошибки/повтор/Undo и пределы памяти. Палитра, физические цвета кнопок, шрифт и общие жесты сохранены. DESIGN.md и sidecar не менялись; старые глобальные версии отмечены, но не исправлялись. SHIP относится к трём проверенным исправлениям. Документирование не подтверждает установку и аппаратную производительность.
