// Compile against both pinned v0.5.0 and current headers. Compare hashes on
// the same CPU/compiler; this exercises changing parameters and bypass states.
#include <effects.h>
#include <stdio.h>
#include <synth.h>
using namespace moss;
static FxState effect;
static SynthTables tables;
static uint32_t add(uint32_t h, int value) {
  return (h ^ uint32_t(value)) * 16777619u;
}
int main() {
  tables.init();
  const unsigned rates[] = {8000, 32000, 44100, 48000, 96000};
  for (unsigned rate : rates) {
    effect.reset();
    Effect fx;
    uint32_t hash = 2166136261u;
    for (unsigned n = 0; n < rate * 3; ++n) {
      if (n % 257 == 0) {
        for (int type = 0; type < 4; ++type) {
          fx.enabledTypes[type] = ((n / 257 + type) % 7) != 0;
          for (int p = 0; p < 4; ++p)
            fx.values[type][p] =
                fxLow(type, p) +
                int((n / 257 + type * 13 + p * 7) %
                    unsigned(fxHigh(type, p) - fxLow(type, p) + 1));
        }
        fx.enabled = fx.enabledTypes[fx.type];
      }
      int l = int((n * 371u) % 65536) - 32768,
          r = int((n * 613u) % 65536) - 32768;
      effect.process(l, r, fx, nullptr, rate, 30 + int(n / 1024) % 211);
      hash = add(add(hash, l), r);
    }
    printf("FX %u %08x\n", rate, hash);
    for (int kind = 1; kind <= 3; ++kind) {
      SynthVoice voice;
      int params[8];
      for (int i = 0; i < 8; ++i)
        params[i] = presetValues(kind, 0)[i];
      voice.on(kind, 48, tables.frequency[48], 127, 512, params, 0, rate, 357);
      hash = 2166136261u;
      for (unsigned n = 0; n < rate * 3; ++n) {
        if (n % 769 == 0) {
          for (int i = 0; i < 8; ++i)
            params[i] =
                synthMin(kind, i) +
                int((n / 769 + i * 17) %
                    unsigned(synthMax(kind, i) - synthMin(kind, i) + 1));
          voice.setTarget(params, int(n / 769) % 2);
        }
        if (n == rate * 2)
          voice.off();
        int l = 0, r = 0;
        voice.render(l, r, tables);
        hash = add(hash, l);
      }
      printf("SYNTH %u %d %08x\n", rate, kind, hash);
    }
  }
}
