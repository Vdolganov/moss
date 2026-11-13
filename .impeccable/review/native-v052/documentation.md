# Native 0.5.2 documentation evidence

Updated `PRODUCT.md`, `DESIGN.md` and `.impeccable/design.json` from the implemented extension. Native 0.5.2 is the current contract; native 0.5.1 remains the inherited capacity/audio reference. The browser v0.6 contract stays independent.

- Checked `app.h`, `app_song.h`, `app_song_ui.h`, `parameter_ui.h`, `parameter_spec.h`, `synth_defs.h`, `effect_defs.h`, plus `app_ui.h`, `song.h`, `config.h` and `hardware lab/src/lab_ui.h`. Compared the native-v052 and native-v051 direction contracts.
- Recorded common fine/coarse axes, source-matching lock increments, R1 horizontal paging and safe release, visible MIX banks, SEQUENCES/SONGS library, ordered SONG rows, independent copy versus shared slot reference, common SONG BPM and default bars from the longest cycle rounded up to a whole bar.
- Inspected `song.png` and `mixlevels.png` at 640×480. These are synthetic fixture captures, not evidence of user content or hardware performance.
- Preserved every frontmatter color, typography, radius, spacing and component token. Source confirms native BG/PANEL/LINE/TEXT/DIM, physical Y/B/A/X accents, bitmap 5×7 scales 2/3 and the four 142×89 parameter fields at x=18+153n, y=346. Sidecar extension values and all component HTML/CSS remain unchanged; native descriptions and seven narrative rules now match DESIGN.md.
- Validation: sidecar parses as JSON; seven named rules, overview, characteristics and Do/Don't lists synchronize with DESIGN.md. Only the four authorized documentation files were written. No code or unrelated historical/browser content was changed.

Not canonized or repaired: unrelated historical/browser-native drift and existing component preview limitations remain outside this extension. Synthetic captures do not renew the 0.5.1 hardware result for 0.5.2.
