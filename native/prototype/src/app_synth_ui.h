#pragma once
// Included within moss. Diagrams describe current base settings, not audio
// scopes.
inline void drawSynth(lab::Canvas &c, const App &a) {
  using namespace lab;
  const auto &tr = a.project.tracks[a.track];
  if (a.page != Sampler || !tr.instrument || tr.instrument == GranularInstrument)
    return;
  int type = tr.instrument - 1;
  const int *p = tr.synthParams[type];
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
  if (a.synthBank) {
    text(c, 18, 196, "CHARACTER / SIGNAL ROUTE", DIM);
    const char *route = tr.instrument == MonoInstrument
                            ? "OSC + SUB + NOISE > DRIVE > FILTER"
                        : tr.instrument == OrganismInstrument
                            ? "COUPLED OSC + GRAIN > COLOR > ENVELOPE"
                            : "DETUNED OPERATORS + NOISE > BODY > AMP";
    text(c, 18, 235, route, TEXT);
    for (int i = 0; i < 4; i++) {
      int x = 18 + i * 154;
      text(c, x, 280, synthLabel(tr.instrument, i + 4), DIM);
      c.rect(x, 309, 128, 2, LINE);
      c.rect(x, 309, p[i + 4] * 128 / synthMax(tr.instrument, i + 4), 2, TEXT);
    }
  } else if (tr.instrument == MonoInstrument) {
    text(c, 18, 196, "FILTER MODEL / 4 POLES", DIM);
    text(c, 354, 196, "AMP / ATTACK - RELEASE", DIM);
    c.rect(18, 300, 290, 1, LINE);
    c.rect(354, 300, 268, 1, LINE);
    // Small-signal four-pole magnitude with global feedback, log frequency
    // axis.
    for (int x = 0; x < 290; x++) {
      double f = 20 * pow(600.0, x / 289.0), w = f / p[0], w2 = w * w;
      double real = 1 - 6 * w2 + w2 * w2 + p[1] * .039,
             imag = 4 * w - 4 * w * w2;
      double magnitude = (1 + p[1] * .039) / sqrt(real * real + imag * imag);
      int y = clamp(int(232 - 20 * log10(fmax(.001, magnitude))), 217, 299);
      c.rect(18 + x, y, 1, 2, TEXT);
    }
    int attack = 10 + p[2] * 80 / 5000, release = 12 + p[3] * 130 / 10000;
    for (int x = 0; x < 268; x++) {
      float e = x < attack        ? float(x) / attack
                : x < attack + 30 ? 1
                                  : expf(-4.6f * (x - attack - 30) / release);
      c.rect(354 + x, 300 - int(e * 70), 1, 2, TEXT);
    }
    text(c, 18, 309, "20 HZ            12 KHZ", DIM);
    text(c, 354, 309, "A       GATE       R", DIM);
  } else if (tr.instrument == OrganismInstrument) {
    text(c, 18, 196, "COUPLING MODEL / BASE SETTINGS", DIM);
    const char *ratios[] = {"1.00", "1.00", "1.50", "2.00"};
    for (int i = 0; i < 4; i++) {
      int y = 225 + i * 25;
      snprintf(b, sizeof(b), "V%d %s", i + 1, ratios[i]);
      text(c, 18, y, b, DIM);
      int w = i == 0 ? 100 : clamp(p[3] * 3 - (i - 1) * 100, 0, 100);
      c.rect(150, y + 5, 210, 2, LINE);
      c.rect(150, y + 5, w * 210 / 100, 2, TEXT);
      snprintf(b, sizeof(b), "> V%d",
               tr.synthMode[type] ? (i + 1) % 4 + 1 : (i ^ 1) + 1);
      text(c, 390, y, b, TEXT);
      if (i == 0)
        text(c, 496, y, "DRIFT", DIM);
      if (i == 1) {
        snprintf(b, sizeof(b), "%d%%", p[1]);
        text(c, 496, y, b, TEXT);
      }
      if (i == 3) {
        snprintf(b, sizeof(b), "FB %d%%", p[2]);
        text(c, 496, y, b, DIM);
      }
    }
  } else {
    text(c, 18, 196, "OPERATOR MODEL / DECAY 0-10 S", DIM);
    const char *route = tr.synthMode[type] ? "4 > 3 > 2 > 1 > OUT"
                                           : "2 > 1 > OUT / 4 > 3 > OUT";
    text(c, 18, 235, route, TEXT);
    snprintf(b, sizeof(b), "MOD:CARRIER %s / FEEDBACK %d%%", ratioName(p[1]),
             p[2]);
    text(c, 18, 270, b, DIM);
    c.rect(18, 313, 604, 1, LINE);
    for (int x = 0; x < 604; x++) {
      float e = expf(-4.60517f * x * 10000.f / (603 * p[3]));
      c.rect(18 + x, 313 - int(e * 23), 1, 1, TEXT);
    }
  }
  text(c, 18, 332,
       a.synthBank ? "2/2 CHARACTER / L1/R1 CORE / A AUDITION"
                   : "1/2 CORE / L1/R1 CHARACTER / A AUDITION",
       DIM);
}
