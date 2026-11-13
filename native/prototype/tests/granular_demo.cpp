#include "../src/storage.h"
#include <stdlib.h>
// Generates a portable single-track audition sequence; never overwrites a file.
int main(int argc, char **argv) {
  using namespace moss;
  if (argc < 2 || argc > 4 || exists(argv[1])) return 2;
  int asset = argc >= 3 ? atoi(argv[2]) : 1;
  if (asset < 0 || asset >= 32) return 2;
  Project p;
  snprintf(p.name, sizeof(p.name), "%s", argc == 4 ? argv[3] : "LENS STUDY");
  p.trackCount = 1;
  p.bpm = 70;
  p.master = 85;
  auto &t = p.tracks[0];
  snprintf(t.name, sizeof(t.name), "LENS");
  t.instrument = GranularInstrument;
  t.granular.asset = asset;
  t.cycle = 128;
  t.level = 90;
  t.steps[0].on = 1;
  t.steps[0].note = GranularRootNote;
  t.steps[0].length = 128;
  t.steps[0].velocity = 110;
  t.fx.enabled = 1;
  t.fx.enabledTypes[0] = t.fx.enabledTypes[1] = 1;
  t.fx.values[0][1] = 55;
  t.fx.values[0][2] = 15;
  t.fx.values[1][0] = 70;
  t.fx.values[1][1] = 70;
  t.fx.values[1][3] = 28;
  return saveProject(argv[1], p) ? 0 : 1;
}
