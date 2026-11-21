#pragma once
#include "app_mono_ui.h"
#include "app_organism_ui.h"
#include "app_fm_ui.h"
// Live voice portraits for MONO / ORGANISM; operator routing for FM.
inline void drawSynth(lab::Canvas &c, const App &a) {
  using namespace lab;
  const auto &tr = a.project.tracks[a.track];
  if (a.page != Sampler || !tr.instrument || tr.instrument == GranularInstrument)
    return;
  int type = tr.instrument - 1;
  char b[100];
  snprintf(b, sizeof(b), "%02d %.24s / %s", a.track + 1, tr.name,
           tr.synthPreset[type] < 0
               ? "CUSTOM"
               : presetName(tr.instrument, tr.synthPreset[type]));
  text(c, 18, 108, b, DIM);
  text(c, 18, 142, instrumentName(tr.instrument), TEXT, 3);
  snprintf(b, sizeof(b), "%s / %s", modeName(tr.instrument, tr.synthMode[type]),
           midiNote(tr.audition));
  text(c, 390, 146, b, TEXT);
  c.rect(18, 180, 604, 1, LINE);
  if(tr.instrument==MonoInstrument){drawMono(c,a);return;}
  if(tr.instrument==OrganismInstrument){drawOrganism(c,a);return;}
  drawFm(c,a);
}
