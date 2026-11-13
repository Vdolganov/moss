# Native FX extension · 0.6.2-dev

Scope: experimental MOSS-GRANULAR. The 0.5.2 stable UI and silent browser study remain separate.

Five named effects in one chain list: ORBIT / POND / CRUSH / REEL / DRIVE. All rows fit at 640×480; the same list serves TRACK and MASTER. Two active inserts per rack. A edits, X toggles, Y+up/down reorders. Disabled settings and locks survive reorder and bypass.

DRIVE and ORBIT expose two four-field banks, CORE / CHARACTER, using the existing neutral tab underline and fixed Y/B/A/X colors. L1/R1 changes banks inside an effect; B returns to the list. L1+left/right chooses the adjacent effect. No modal editor or extra face-button navigation.

In PATTERN / FX LOCKS, L1/R1 retains top-level bank navigation. L1+left/right chooses FX and L1+up/down chooses its parameter bank. Show the FX name and bank count. Inherit remains `--`; clearing the current bank affects four fields only. Parameter locks attach to FX identity, not its order.

Parameter metadata is centralized in `effect_defs.h`. Horizontal edits are one unit; vertical edits are 10 for percentages, 100 Hz for LOW CUT, 1000 Hz for HIGH CUT, and one choice for TIME/CURVE. LOW CUT zero reads OFF. The first ORBIT bank is now TIME / FEEDBACK / WIDTH / MIX; old stored MIX/SPREAD values migrate.

DRIVE's graphic is a static transfer model from CURVE/DRIVE/BIAS, not an oscilloscope or measured signal. BODY/TONE/DYNAMICS are temporal/frequency-dependent and are not represented by that static curve. ORBIT's diagram shows repeat level decay with a labeled time division. No decorative animation, glow, new color or diagnostic timers.

Manual screenshots: `.impeccable/review/native-fx-character/`. Audio and hardware evidence are separate in `docs/FX-CHARACTER-v1.md` and its English translation. No UI screenshot establishes an audio-performance result.
