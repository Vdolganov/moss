# Native ambient update — direction contract

Mode: Operate. Original RG35XX, 640×480, physical Y/B/A/X and D-pad; existing graphite tokens, bitmap type and navigation remain authoritative. Source reference: GrainBrick author's tutorial, 4:16–4:20, https://www.youtube.com/watch?v=Iy-IeH4_EvQ&t=256s (visually inspected 2026-11-16).

## Direction contract

- REEL's first viewport is dominated by two large flat reel outlines, broad three-blade hubs and a shallow suspended tape arc with a central square. Remove the separate oscilloscope-like speed chart. A single speed/status line remains readable. Four parameter tabs sit above the fixed Y/B/A/X fields. Motion follows the existing audio snapshot, freezes on STOP, dims on BYPASS. Slight tape displacement is bounded by measured amplitude, not random decoration.
- GRANULAR keeps LENS and adds CLOUD as a fifth page: REVERSE probability / DETUNE cents / SIZE VAR / JITTER. L1/R1 cycles the pages, held face button plus horizontal/vertical direction keeps fine/coarse edits. Numeric locks use the same page. Reverse belongs to each grain, independently of scan direction; changing it affects future grains and leaves living grains intact. Reverse arcs represent the source interval and markers follow real playback. F/R count is an observed state, not another setting.
- POND has TAIL and SPACE pages, four controls each. DECAY is displayed as a nominal low-frequency time in seconds; damping and interpolation can shorten high-frequency decay. Its plot is explicitly a design envelope, not a live measurement. Existing enable, reorder, bypass and lock controls are reused.
- Empty sample, stopped, playing, frozen, bypass, full pool and missing asset use existing recovery paths. No new modal required to edit parameters. Undo covers ordinary gestures. Format MS14 adds fields; MS13 loads new cloud controls at zero and no invented locks.
- UI performs bounded drawing only for the visible screen. DSP retains eight grains per instance, four tracks and two inserts per rack. Real-device short profiling gates installation. Do not silently reduce fidelity or change routing to pass the budget.
