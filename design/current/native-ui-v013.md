# Native UI 0.13 — approved interface transfer

The 2026-11-20 user approval freezes `visual prototype/current` as the visual and interaction reference for the native application. This supersedes historical colored fields and underlined navigation for the native surface. Exact browser and native source snapshots: `work/ui-port-2026-11-20/before`.

640×480, neutral graphite, monochrome fields and waveform regions. Typography: 21/14/12px with 8px reserved for duration capsules. Spacing: 4/8/12/16/24/32px. Centered rounded tabs at y56 (main), y104 (local), y336 (parameters); fields y376 and footer y444. Physical Y/B/A/X field order. 16/64-step views preserve cursor and canonical timing. Native synthesis and its telemetry remain real.

Behavior contract: `interface-lab-v012.md` and `ux-fixes-2026-11-20.md`. Browser wrapper, pointer editing, simulated audio and JSON-only import are browser adaptations. Preserve native WAV processing, bounded audio engine and project formats. Back up device data before installation; run short full-load profiles and an ordinary launcher check.

## Implemented scope

Shared tab and field primitives, GRID 16/64 geometry, duration continuations, sampler activity, monochrome TAPE regions, paginated menus/pickers and the naming keyboard now use the frozen contract. Native portraits still consume real audio/voice telemetry. No raster screenshot is used as a shipping screen.

Main navigation follows L2/R2; local tabs L1/R1; nested axes are L1+left/right for slot/region and L1+up/down for parameter page. Effects have separate enabled LOCKS tabs with persistent page memory. Modal key release is isolated. Single-field lock actions are explicit; undo is checkpointed only after accepted mutation. Kit saving/assignment, validated slice drafts, scoped clears and SONG recovery are implemented in native workflows.

Browser adaptations remain outside native scope: wrapper controls, pointer interaction, JSON-only imports, browser focus/accessibility semantics and simulated audio. Native menu availability retains the existing capability checks; disabled reasons are reported by the device action where relevant rather than reproducing browser focusable disabled rows. The 512-tick timing model and MS18 format are unchanged.

## Verification and installation

Host regressions cover musical timing, synths/FX, sample files, and 15 additional UI/data scenarios. Hardware evidence includes four-track full-load profiles, 512-step patterns, 240 BPM, two enabled effects on each track and master, all PCM cache slots, capture and UI changes. One initial mixed REEL/POND run exceeded 32 ms by 0.323 ms. Removing full-project copies from repeated UI edits eliminated that failure in subsequent 30- and 20-second runs. The entire gesture still has one undo checkpoint.

The installed app was opened through its normal Garlic launcher, navigated through existing projects and exited with status 0. Projects/library/captures were compared by SHA-256 before and after the check. Detailed results, build hashes, bounds and screenshots: [native UI verification](../../docs/verification/ui-v013/README.md). The final local installation is MOSS 0.13.0; GitHub publication is a separate action.
