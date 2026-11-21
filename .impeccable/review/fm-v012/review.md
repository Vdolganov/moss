## verdict

1. Live ratio caption — resolved. lockedratio.png visibly shows `1:1 / LIVE` while the editable base RATIO remains `3:1`. The updated mailbox publishes the effective target ratio, and the caption uses it only while active; idle.png retains the base `3:1 / IDLE`. The focused telemetry assertion also checks that changing the ratio target preserves voice age and envelope.
2. BEND telemetry — resolved. accent.png shows PUNCH 48 and DYNAMICS 100 while BEND fills approximately 43% of its range, consistent with the supplied low-velocity case's approximately 20.75-semitone offset. fm_dsp.h now computes bendSemitones once for actual pitch, and fm_view.h publishes that same value. The focused telemetry assertion covers velocity 24 and the corresponding accented offset.

All thirteen original capture paths and both focused captures were re-opened; they remain valid complete 640×480 views. No regression introduced by this fix batch is visible.

## remaining

Clear. This ship covers the scored fixes, not the whole surface. Physical-device readability, temporal feel and full-load audio performance remain outside this verdict.

disposition: ship
