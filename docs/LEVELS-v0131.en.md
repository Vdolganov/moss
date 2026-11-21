# MOSS 0.13.1 — SAMPLER and GRANULAR levels

[Русский](LEVELS-v0131.md) · **English**

Track playback was quieter than file preview: the mix was multiplied by MASTER / 300, followed by 180% monitor gain. Even at 100% on every control, this delivered 60% of source amplitude (−4.44 dB). Default track/master levels of 75% and slot level of 80% attenuated it further. GRANULAR also had a nominal 0.75 gain.

The master now uses MASTER / 100 with no additional monitor gain. A 100% path has unity gain below the existing soft peak-protection knee. Summing headroom is retained through the master effects; the earlier hard clamp before the master chain is removed. Output soft protection remains.

- New tracks and MASTER default to **100%**. New sampler slots, including four-slot kit assignments, default to **100%**.
- A-button instrument audition uses full velocity **127**. Sequenced notes retain their individual velocity; new steps still default to **90**.
- GRANULAR nominal gain is 1.0 instead of 0.75. Grain overlap/window compensation remains. No additional voices, buffers, file normalization or input-dependent granular compression are introduced.
- Saved mixer, velocity and lock values are not rewritten. Existing projects sound louder through the corrected common bus, including synth tracks.
- Slicing preserves PCM. MASTER capture records the audible output including peak protection; file auditions remain unity and are excluded from capture.

To compare with the original: bypass FX, use slot/track/master at 100%, centered PAN, velocity 127, PITCH 0 and no fades or shortened LENGTH. GRANULAR envelopes, density, filtering, pitch and source content still affect perceived loudness; waveform identity is not expected.

## Measurements

| Scenario | Before | After |
|---|---:|---:|
| SAMPLER, all levels 100%, velocity 127 | −4.44 dB | **0.00 dB** |
| Fresh note, default levels, velocity 90 | −14.38 dB | **−2.99 dB**, velocity only |
| Dense GRANULAR cloud, calibration signal | −7.46 dB | **−0.51 dB** |

Relative to source, below the protection knee, FX bypassed. The granular test uses decorrelated band-limited material after cloud warmup, and gains approximately 6.95 dB in this scenario. Windows and ARM measurements match. Tests also cover 50%/25% gain, silence controls, 16 simultaneous loud samples, saved settings and exact resampling.

[Device report and short stress checks](verification/levels-v0131/README.md).
