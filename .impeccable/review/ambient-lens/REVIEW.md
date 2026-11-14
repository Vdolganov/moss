# Ambient LENS · finish review

Scope: native 640×480 and the existing silent browser study. Operate mode; no palette, typography or navigation redesign.

Checked native renderer captures for PLAY, partial range/loops, TEXTURE/JUMP, ENVELOPE, held parameter, empty source, frozen head and voice locks. Device framebuffer captures confirm real sample geometry, playback, JUMP 75→76→75 through B+arrows, FREEZE and START 0→20→0. Browser checks cover the new preset, pointer/arrow edits, range shading and reload persistence, with accessible JUMP text. Existing reduced-motion code remains in place; no responsive CSS or framework changes.

Arc extent, lane and cursor are audio data, not an animation clock. Hollow head and solid read squares are distinct without relying on color. START/END exclude only the waveform; already sounding grains retain their own bounds. Full-region loops are labelled, and effective G/S explains the pool limit. The 16 parameter locks retain the four-bank pattern.

Visual result: ready for user testing. Hardware load evidence is separate in docs/AMBIENT-LENS-v1.md; this review is not an audio-performance claim.

PNG provenance: native `tests/preview.cpp` with the original generated 6.4-second source from `tests/granular_ambient_audio.cpp`, or RGB565 framebuffer captures from the connected RG35XX. No generated decorative artwork or third-party sample-pack audio is included.
