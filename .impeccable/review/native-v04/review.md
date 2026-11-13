## verdict

1. Resolved — SELECT/Tab cancellation for both new pickers. `App::action` at `native/prototype/src/app.h:756–769` now closes voicePicker on B or SELECT, returns without applying a command, and retains VOICE. The actual `picker-before-tab.png` and `picker-after-tab.png` captures show the instrument picker closing back to ORGANISM. Regression tests exercise SELECT press/release for both instrument and preset pickers, assert the entire Project is unchanged and the page remains Sampler/VOICE, and retain the B cancellation path. The recorded synth verification run passes.

The same seven original image paths were re-read, with the two additional cancellation captures. No regression attributable to the cancellation change is observed.

## remaining

Clear. Ship covers the scored picker-cancellation fix, not a new whole-surface review. The evidence remains native Windows 640×480 captures and exact render fixtures; RG35XX hardware behavior and audio are outside this verdict.

disposition: ship
