# Native ORGANISM 0.10 · Operate

Implemented surface, documented after visual **SHIP** review on 18 November 2026. Scope: native ORGANISM VOICE and its nested VOICE LOCKS pages at 640×480. This local extension preserves the approved graphite system, other instruments, header, footer, bitmap type, 18 px margins, four fields at y=346 and physical Y/B/A/X colours.

## Portrait and states

The user asked for a deep, living, sometimes sandy analogue-like drone with a less technical, more engaging visual. Four intertwined neutral contours form a portrait of the voice state. Their shape, displacement and line weight follow actual carrier modulation, energy and envelope telemetry; DENSITY distinguishes present and dormant carriers. This is not a waveform or spectrum. The brighter principal contour establishes hierarchy without using the four parameter accents. The 4 VOICES label and SUB indicator remain visible.

GROW means an active gated voice. Ordinary note-off or audition expiry enters FADE; contours continue following the sounding release until it ends. REST is inactive, dim and stationary. Transport STOP forces a short 128-sample anti-click fade (about 4 ms at 32 kHz), then REST; it does not wait for the full RELEASE setting. No wall-clock animation, gradients, glow, knobs or new accent palette is introduced.

## Banks and controls

Five banks remain visible together above the four fields; a neutral underline marks the selected bank. The twenty parameters retain their physical columns:

| Bank | Y | B | A | X |
|---|---|---|---|---|
| CORE | SHAPE | COUPLE | DENSITY | SUB |
| TONE | CUTOFF | RESONANCE | DRIVE | NOISE |
| MOTION | DRIFT | RATE | WANDER | BREATH |
| VOICES | DETUNE | INTERVAL | SPREAD | GLIDE |
| ENVELOPE | ATTACK | RELEASE | GRIT | NOISE TONE |

Short L1/R1 changes VOICE banks. PATTERN retains STEP / VOICE LOCKS / FX LOCKS / TIMING as its outer banks; L1+arrows selects the nested voice-lock bank. The selected step, LOCK ONLY state, nested bank and four fields stay visible together. Numeric values and units remain visible while held, with the established fine/coarse axes. All twenty parameters support ordinary locks, LOCK ONLY and RESET; `--` remains distinct from a numeric lock or RESET. Mode and presets remain in L1+R1 options.

## Implementation and authority

Sources: `native/prototype/src/app_organism_ui.h`, `organism_view.h` and `organism_dsp.h`. Telemetry crosses the audio/UI boundary through a bounded atomic mailbox. Four contours use fixed point counts and table-based trigonometry; the portrait has no shipping raster asset or new font, palette or spacing token.

Control ranges and DSP tradeoffs: [Русский](../../docs/ORGANISM-v010.md) / [English](../../docs/ORGANISM-v010.en.md). Existing browser diagrams and historical native contracts retain their original scope. This record supersedes the routing-bar and CORE/CHARACTER descriptions only for native ORGANISM 0.10.

## Русский

Локальное расширение ORGANISM 0.10 сохраняет графит, bitmap-шрифт, поля и порядок физических кнопок Y/B/A/X. Четыре переплетающихся нейтральных контура показывают реальные медленные модуляции, энергию и огибающую голосов; это портрет состояния, не осциллограмма. GROW обозначает звучащий голос с открытым gate, FADE — отпускание ноты до конца RELEASE, REST — неподвижное приглушённое состояние. STOP завершает голос коротким защитным затуханием в 128 отсчётов и переводит портрет в REST, не ожидая полного RELEASE.

CORE / TONE / MOTION / VOICES / ENVELOPE видны одновременно над постоянными четырьмя полями. L1/R1 выбирает страницу в VOICE, L1+стрелки — вложенную страницу VOICE LOCKS. Все двадцать параметров поддерживают обычные локи, LOCK ONLY и RESET; значения, единицы, выбранный шаг и название банка остаются видимыми. Палитра и исторический браузерный контракт не меняются.

## Evidence and limits

[Visual review](../../.impeccable/review/organism-v010/review.md): **SHIP**, no material fixes. The reviewer opened all eight complete native-renderer captures individually: `core.png`, `tone.png`, `motion.png`, `voices.png`, `envelope.png`, `idle.png`, `release.png`, `locks.png`, plus the contact sheet. Their eight hashes match [provenance.json](../../.impeccable/review/organism-v010/provenance.json). These deterministic 640×480 fixtures use actual 32 kHz voice state and are QA evidence, not shipping art.

The surface has no HTML/CSS detector. This visual verdict checks the handheld composition and supplied states; still images do not establish animation cadence, audio quality, held-button interaction, all parameter extremes or device timing. Hardware measurements remain separate in `docs/verification/organism-v010/`; this record makes no hardware-performance claim.
