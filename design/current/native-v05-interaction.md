# Native 0.5 interaction contract

Scope: original RG35XX; ARM build targets the GarlicOS 1.3.9 ABI. User-confirmed
hardware evidence is for 0.4.0 on 1.3.9. The 0.5.0 build and planned GarlicOS
1.4.9 recovery still require a new physical-console run. Extend the existing
graphite screen and four parameter colors; no visual identity change. Browser
prototype is historical. Operating instructions: native/prototype/README.md.

## Navigation and editing

- D-pad: select item/cell. A: activate/toggle; B: back. X: audition where relevant.
- A/B/X/Y held + arrows: the four displayed parameters, with acceleration.
- Y tap: parameter category (STEP, VOICE LOCKS, FX LOCKS, CYCLE); in a synth,
  switches CORE / CHARACTER. L1+arrows selects a nested object: an assigned
  sampler key in PATTERN outside FX LOCKS, a synth parameter bank in VOICE or
  VOICE LOCKS, or an effect in the chain/settings or FX LOCKS. In sampler VOICE,
  unmodified arrows select all keys, including empty destinations. Modifier release
  must not also change track or navigate a cell.
- L1/R1 tap: track/object. L2/R2: workspace tabs. L1+R1: options.
- SPACE opens a four-row effect chain. A opens settings; X toggles the selected
  effect; Y+up/down moves it. B from settings returns to the chain. Locks refer
  to effect identity, so reordering does not reassign locks.

## Sampler

12 assigned-note lanes on one track, independently populated and polyphonic.
Select a lane with L1+arrows; show note and sample name above its grid. STEP
slots: velocity, fade in, fade out, probability. VOICE LOCKS: pitch, start,
end, level per hit. Effect locks affect the shared track bus; simultaneous
lanes see the same effect locks, with no ambiguous per-voice bus settings.
All sample playback is one-shot. Re-triggering the same key crossfades the old
voice over 128 audio frames; other keys continue. Legacy GATE/LOOP values do
not change this behavior. Delay/reverb run over rests. FX locks remain latched
through rests until the next successful trigger; a trigger with inherited
fields restores those fields to base values. Synth voice locks follow the same
trigger-based lifetime. Sampler voice locks belong to the individual key hit.

Assignment picker includes IMPORT FROM DEVICE. Import captures the destination
track/key, enters source browser then trim. A Save opens the existing text
editor, confirms a named library asset, assigns it, and returns to VOICE.
Cancel naming keeps the trim. Cancel import returns without altering the key.
Disk/format/full-library failures keep the source/range/name for retry.

## Time

User confirmed 8/16/32/64 means step denominator, not pattern length. Changing
resolution preserves musical duration AND event positions. Internal timeline
uses 1/64-note ticks, up to 512 ticks (64 eighth notes). A coarser view indicates
events between gridlines and preserves them. Per-track cycle and playhead are
independent. Note duration and fades are not stretched by changing the view.
A+arrows in CYCLE edits duration in current cells; B+arrows edits resolution.
Fifteen eighth-note cells therefore become 30 sixteenths or 120 sixty-fourths.
The displayed LENGTH rounds up to current cells without changing stored time;
a partial final cell is identified separately. Hidden events beyond a shortened
cycle are retained. Return to a fine view to edit events between coarse gridlines.

## Sound

Monitor output +35%, bounded peak protection, applies to audition too. Resampling
is before monitor gain to avoid repeatedly baking gain into the library.
Two banks of four parameters per synth, same banks available for voice locks.
Presets set all eight base parameters, preserving mode and locks. CHARACTER:
MONO ENV AMT/SUB/NOISE/DRIVE; ORGANISM COLOR/GRAIN/ATTACK/RELEASE;
FM DETUNE/NOISE/ATTACK/BODY. MODEL is a settings diagram, not an audio scope.
Four unique effects in a reorderable chain: ORBIT, POND, CRUSH, REEL. REEL is
original compact tape coloration (WOBBLE/WEAR/NOISE/MIX), inspired by RC-20,
not a port/emulation. All four are available on each track and the master.
DSP storage is allocated once per effect type per track.

## Persistence and scope

MOS5 replaces MOS4 without automatic migration. The authorized upgrade may
reset old projects only after backup; prepared samples, source WAVs and captures
are preserved. Prepared library limits remain 32 assets at 1 MiB PCM each;
full-source audition streams through the unchanged 256 KiB buffer. Native
version numbering is separate from the historical browser v0.6 design context.

## Validation gates

Round-trip codec and invalid data; same-step drum polyphony; one-shot end;
independent track timelines; resolution round trip with off-grid events;
named import success/cancel/error; modifier release order; lock banks and FX
reorder; audible tails; finite bounded DSP at parameter extremes; ARM memory
and imports; actual Windows UI flows. Hardware performance remains pending
until user runs the new build on the console.
