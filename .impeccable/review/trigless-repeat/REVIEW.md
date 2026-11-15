# LOCK ONLY / REPEAT · native interaction review

Scope: native 640×480 RG35XX/Windows interface. Preserve the existing graphite, type and Y/B/A/X field colors. No browser redesign.

Five renderer captures cover the note-less STEP state, four-field TIMING, individual RESET, contextual actions and sampler lane state. Inspected at native resolution: no field or menu text clipping. Filled NOTE and hollow-diamond LOCK ONLY are distinct without color. Note-specific fields display NO NOTE. TIMING labels distinguish track cycle/resolution, shared track-audio REPEAT and step-event TYPE.

The existing four-bank navigation remains. REPEAT and TYPE use held A/X plus arrows; key-up does not trigger their short actions. Context actions are available only where relevant. RESET differs from `--`, and CLEAR CURRENT LOCK BANK removes only its visible four fields. Physical gesture and model tests cover automatic creation, final-lock removal, conversion and clearing; undo uses the existing project snapshot and stops transport.

The sampler row selects voice-lock scope; FX/REPEAT address its combined track audio. The grain view continues to show the running voice before audio repeat/FX. Repeat has an explicit OFF event to return to that live sound.

Fixtures are rendered by `native/prototype/tests/preview.cpp` (`event-*`). Device captures and measured load results are recorded separately; this visual review is not a performance claim.

Installed-device pass: LOCK STUDY plays with the expected outlined events; A+right on the note-less LENGTH field does not add a note. In TIMING, physical A changes 1/16 → 1/32 and back; physical X converts LOCK → NOTE → LOCK while stopped. The TEXTURE bank displays CUTOFF RESET. MENU exits normally. The saved demo matches its original bytes after these edits are restored. See `docs/TRIGLESS-REPEAT-v1.md` for the separately reported normal-session timing counters.
