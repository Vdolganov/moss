// Native 640 x 480 renderer. Included after App, inside namespace moss.
inline const char *note(int n) {
  static const char *names[] = {"C3",  "C#3", "D3",  "D#3", "E3",  "F3",
                                "F#3", "G3",  "G#3", "A3",  "A#3", "B3"};
  return names[clamp(n, 0, 11)];
}
inline const char *midiNote(int n) {
  static char names[4][8];
  static unsigned slot = 0;
  const char *pitch[] = {"C",  "C#", "D",  "D#", "E",  "F",
                         "F#", "G",  "G#", "A",  "A#", "B"};
  char *out = names[slot++ % 4];
  snprintf(out, 8, "%s%d", pitch[n % 12], n / 12 - 1);
  return out;
}
inline void drawSampleSlots(lab::Canvas &c, const App &a, int y) {
  using namespace lab;
  const auto &tr = a.project.tracks[a.track];
  for (int i = 0; i < Keys; ++i) {
    int x = 18 + i * 153;
    bool selected = i == a.key;
    c.box(x, y, 145, 42, selected ? TEXT : LINE);
    if (selected)
      c.rect(x, y + 40, 145, 2, TEXT);
    char label[24];
    snprintf(label, sizeof(label), "%d / %s", i + 1, note(i));
    text(c, x + 8, y + 6, label, selected ? TEXT : DIM);
    text(c, x + 8, y + 24,
         tr.keys[i].asset < 0 ? "EMPTY" : a.assetName(tr.keys[i].asset),
         tr.keys[i].asset < 0 ? DIM : TEXT, 2, 10);
  }
}
#include "app_song_ui.h"
#include "app_synth_ui.h"
#include "app_granular_ui.h"
#include "app_tools_ui.h"
inline void draw(lab::Canvas &c, const App &a) {
  using namespace lab;
  char b[128];
  c.clear(BG);
  bool playing =
      __atomic_load_n(&a.engine->shownPlaying, __ATOMIC_ACQUIRE) != 0;

  if (a.page == SongPage)
    drawSong(c, a);
  else if (a.page == Atlas)
    drawAtlas(c, a);
  else {
    snprintf(b, sizeof(b), "%s  %s%s",
             a.captureBusy() ? "REC"
             : playing       ? ">"
                             : "-",
             a.name(), a.dirty ? " +" : "");
    text(c, 18, 18, b, TEXT, 2, 30);
    bool tempoFocus = !a.options && (a.buttons.held & bit(Select));
    if (tempoFocus)
      c.box(468, 9, 154, 30, TEXT);
    snprintf(b, sizeof(b), "SEL %d BPM", a.project.bpm);
    text(c, 478, 18, b, tempoFocus ? TEXT : DIM);
    c.rect(0, 48, 640, 1, LINE);
    const char *tabs[3];
    int active = 0;
    if (a.page >= Pattern) {
      tabs[0] = "PATTERN";
      tabs[1] = "VOICE";
      tabs[2] = "SPACE";
      active = a.page - Pattern;
    } else {
      tabs[0] = "GRID";
      tabs[1] = "MIX";
      tabs[2] = "TAPE";
      active = a.page - Grid;
    }
    if (!a.assignMode && !a.importAssign) {
      text(c, 18, 65, "[", DIM);
      text(c, 610, 65, "]", DIM);
    }
    for (int i = 0; i < 3; i++) {
      text(c, 60 + i * 180, 65, tabs[i], i == active ? TEXT : DIM);
      if (i == active)
        c.rect(56 + i * 180, 88, 140, 2, TEXT);
    }
    c.rect(0, 91, 640, 1, LINE);
    if (a.page == Mix) {
      const char *tabs[] = {"LEVELS", "MASTER FX"};
      bankTabs(c, tabs, 2, a.mixTab);
    }
    if (a.page == Grid) {
      text(c, 30, 111, "TRACK", DIM);
      text(c, 278, 111, "VOICE", DIM);
      text(c, 482, 111, "SPACE", DIM);
      int first = a.track / 5 * 5;
      for (int t = first; t <= a.project.trackCount && t < first + 5; t++) {
        int y = 141 + (t - first) * 53;
        if (t == a.track)
          c.rect(18, y - 10, 604, 49, PANEL);
        if (t == a.project.trackCount) {
          text(c, 30, y + 5, t == Tracks ? "4 TRACKS / FULL" : "+ ADD TRACK",
               t == Tracks ? DIM : TEXT);
          continue;
        }
        snprintf(b, sizeof(b), "%02d  %.13s", t + 1, a.project.tracks[t].name);
        text(c, 30, y + 5, b, TEXT, 2, 17);
        text(c, 278, y + 5, instrumentName(a.project.tracks[t].instrument),
             DIM);
        int effects = 0;
        for (int id = 0; id < 4; id++)
          effects += a.project.tracks[t].fx.active(id);
        snprintf(b, sizeof(b), "%d / %d FX", effects, ActiveEffects);
        text(c, 482, y + 5, b, DIM);
        c.rect(18, y + 39, 604, 1, t == a.track ? TEXT : LINE);
      }
      snprintf(b, sizeof(b), "%d / 4 TRACKS / A OPEN / L1+R1 OPTIONS",
               a.project.trackCount);
      text(c, 18, 417, b, DIM);
    }
    if (a.page == Mix && a.mixTab == 0) {
      for (int t = a.track / 3 * 3;
           t < a.project.trackCount && t < (a.track / 3 + 1) * 3; t++) {
        const auto &tr = a.project.tracks[t];
        int y = 154 + t % 3 * 64;
        if (t == a.track)
          c.box(18, y - 10, 604, 61, TEXT);
        snprintf(b, sizeof(b), "%02d  %.18s", t + 1, tr.name);
        text(c, 30, y, b, TEXT, 2, 23);
        snprintf(b, sizeof(b), "%03d STEPS / %s", tr.cells(),
                 tr.mute ? "MUTED" : instrumentName(tr.instrument));
        text(c, 30, y + 26, b, DIM);
        unsigned peak =
            __atomic_load_n(&a.engine->shownPeak[t], __ATOMIC_ACQUIRE);
        c.rect(360, y + 10, 240, 4, LINE);
        c.rect(360, y + 10, clamp(int(peak) * 240 / 16384, 0, 240), 4, TEXT);
        snprintf(b, sizeof(b), "LEVEL %d / PAN %+d", tr.level, tr.pan);
        text(c, 360, y + 26, b, DIM, 2, 20);
      }
      if (!a.project.trackCount)
        text(c, 18, 190, "NO TRACKS / B GRID / A ADD TRACK", DIM);
    }
    if (a.page == Pattern) {
      const auto &tr = a.project.tracks[a.track];
      const auto &selected = tr.steps[a.step];
      int stride = tr.stride(), selectedCell = a.step / stride;
      int first = selectedCell / 16 * 16;
      if (tr.instrument)
        snprintf(b, sizeof(b), "%02d %.18s / 1:%d", a.track + 1, tr.name,
                 tr.division);
      else
        snprintf(b, sizeof(b), "%s / %.20s / 1:%d", note(a.key),
                 a.assetName(tr.keys[a.key].asset), tr.division);
      if (tr.instrument)
        text(c, 18, 108, b, DIM);
      else
        drawSampleSlots(c, a, 108);
      snprintf(b, sizeof(b), "%02d-%02d", first + 1,
               clamp(first + 16, 0, tr.cells()));
      if (tr.instrument)
        text(c, 524, 108, b, DIM);
      for (int i = 0; i < 16; i++) {
        int cell = first + i, s = cell * stride, x = 18 + i % 8 * 76,
            y = (tr.instrument ? 132 : 164) + i / 8 * (tr.instrument ? 65 : 50);
        int cellHeight = tr.instrument ? 53 : 43;
        bool enabled = s < tr.cycle;
        const auto &st = tr.steps[s < Steps ? s : 0];
        const auto &hit = tr.hits[a.key][s < Steps ? s : 0];
        bool on = enabled && (tr.instrument ? st.on : hit.on);
        bool between = false;
        for (int off = 1; off < stride && s + off < tr.cycle; off++)
          between |= tr.instrument ? tr.steps[s + off].on != 0
                                   : tr.hits[a.key][s + off].on != 0;
        uint32_t ink = on ? BG : DIM;
        c.rect(x, y, 69, cellHeight, on ? 0xd5d6d3 : BG);
        c.box(x, y, 69, cellHeight,
              enabled ? (on ? 0xd5d6d3 : 0x56585e) : LINE);
        snprintf(b, sizeof(b), "%02d", cell + 1);
        text(c, x + 7, y + 7, b, enabled ? ink : LINE);
        if (enabled) {
          text(c, x + 7, y + (tr.instrument ? 29 : 23),
               on ? (tr.instrument ? midiNote(st.note) : note(a.key)) : "--",
               ink);
          bool locked = false;
          if (tr.instrument == GranularInstrument) {
            for (int n : st.grainLocks) locked |= n != Inherit;
          } else if (tr.instrument) {
            for (int n : st.synthLocks[tr.instrument - 1])
              locked |= n != Inherit;
          } else {
            for (int n : hit.locks)
              locked |= n != Inherit;
          }
          for (const auto &row : st.fxLocks)
            for (int n : row)
              locked |= n != Inherit;
          if (between)
            c.rect(x + 7, y + cellHeight - 2, 52, 1, ink);
          if (locked)
            c.rect(x + 59, y + 31, 4, 4, ink);
          const auto &selectedHit = tr.hits[a.key][a.step];
          int length = tr.instrument ? selected.length : selectedHit.length;
          bool sustained = (tr.instrument ? selected.on : selectedHit.on) &&
                           length &&
                           ((s - a.step + tr.cycle) % tr.cycle) < length;
          if (sustained)
            c.rect(x + 4, y + cellHeight - 5, 61, 2, on ? BG : DIM);
          if (playing &&
              unsigned(cell) == __atomic_load_n(&a.engine->shownStep[a.track],
                                                __ATOMIC_ACQUIRE) /
                                    unsigned(stride))
            c.rect(x, y - 6, 69, 2, TEXT);
        }
        if (s == a.step) {
          c.box(x - 3, y - 3, 75, cellHeight + 6, TEXT);
          c.box(x - 2, y - 2, 73, cellHeight + 4, TEXT);
        }
      }
      snprintf(b, sizeof(b), "PAGE %d/%d", selectedCell / 16 + 1,
               (tr.cells() + 15) / 16);
      text(c, 18, 269, b, DIM);
      snprintf(
          b, sizeof(b), "PLAY %02u/%02d",
          __atomic_load_n(&a.engine->shownStep[a.track], __ATOMIC_ACQUIRE) /
                  unsigned(stride) +
              1,
          tr.cells());
      text(c, 186, 269, b, DIM);
      text(c, 454, 269, "R1+L/R PAGE", DIM);
      c.rect(18, 293, 604, 1, LINE);
      snprintf(b, sizeof(b), "STEP %03d / %s", selectedCell + 1,
               (tr.instrument ? selected.on : tr.hits[a.key][a.step].on)
                   ? "ON"
                   : "OFF");
      text(c, 18, 305, b, TEXT);
      int asset = tr.keys[a.key].asset;
      snprintf(b, sizeof(b), "%s / %s",
               tr.instrument ? midiNote(selected.note) : note(a.key),
               tr.instrument ? instrumentName(tr.instrument)
                             : a.assetName(asset));
      text(c, 350, 305, b, DIM, 2, 22);
      const char *banks[] = {"STEP", "VOICE LOCKS", "FX LOCKS", "CYCLE"};
      const int x[] = {18, 100, 280, 430}, width[] = {56, 140, 100, 68};
      for (int i = 0; i < 4; i++) {
        text(c, x[i], 326, banks[i], a.bank == i ? TEXT : DIM);
        if (a.bank == i)
          c.rect(x[i], 343, width[i], 1, TEXT);
      }
      text(c, 520, 326,
           a.bank == 2                    ? fxName(tr.fx.type)
           : a.bank == 1 && a.granularTrack() ? GranularBanks[a.synthBank]
           : a.bank == 1 && tr.instrument ? (a.synthBank ? "CHAR" : "CORE")
                                          : "L1/R1",
           DIM, 2, 8);
    }
    if (a.page == Sampler && !a.project.tracks[a.track].instrument) {
      auto &tr = a.project.tracks[a.track];
      auto &k = tr.keys[a.key];
      snprintf(b, sizeof(b), "TRACK %02d / KEY %s / %s", a.track + 1,
               note(a.key), a.assetName(k.asset));
      text(c, 18, 108, b, DIM);
      drawSampleSlots(c, a, 140);
      if (a.available(k.asset)) {
        auto &sample = a.engine->samples[k.asset];
        int left = 18 + 604 * k.start / 100, right = 18 + 604 * k.end / 100;
        c.rect(left, 235, right - left, 62, PANEL);
        for (int i = 0; i < 64; i++) {
          int h = clamp(sample.peaks[i] * 28 / 8192, 0, 28);
          c.rect(18 + i * 604 / 64, 266 - h, 5, h * 2 + 1, DIM);
        }
        c.rect(left, 235, 1, 62, TEXT);
        c.rect(right - 1, 235, 1, 62, TEXT);
        snprintf(b, sizeof(b), "ONE SHOT / A AUDITION / OPTIONS ASSIGN");
        text(c, 18, 321, b, DIM);
      } else
        text(c, 18, 260, "A ASSIGN SAMPLE / L1+R1 OPTIONS", TEXT);
    }
    drawTools(c, a);
    drawSynth(c, a);
    drawGranular(c, a);
    if (a.page == Pattern || a.page == Sampler ||
        (a.page == Mix && (a.project.trackCount || a.mixTab == 1)) ||
        a.page == Space || (a.page == Tape && a.tapeTab < 2)) {
      View v;
      v.buttons = a.buttons;
      v.holds = a.holds;
      auto &t = a.project.tracks[a.track];
      auto &s = t.steps[a.step];
      auto &k = t.keys[a.key];
      bool cycleOnly = a.page == Pattern && a.bank == 3;
      if (cycleOnly) {
        text(c, 330, 365, "RESOLUTION KEEPS TIME", DIM);
        text(c, 330, 394, "OFF-GRID EVENTS KEPT", DIM);
        if (t.cycle % t.stride()) {
          snprintf(b, sizeof(b), "LAST CELL %d/%d", t.cycle % t.stride(),
                   t.stride());
          text(c, 330, 423, b, DIM);
        } else
          text(c, 330, 423, "MAX 64 EIGHTH NOTES", DIM);
      }
      int fieldCount = cycleOnly                          ? 2
                       : a.fxPage() && !a.fxDetail        ? 0
                       : a.page == Tape && a.tapeTab == 0 ? 2
                                                          : 4;
      for (int i = 0; i < fieldCount; i++) {
        const char *label = "";
        const char *unit = "";
        bool disabled = false;
        if (a.page == Pattern && a.bank == 0 && !t.instrument) {
          const auto &h = t.hits[a.key][a.step];
          const char *l[] = {"VELOCITY", "FADE IN", "LENGTH", "PROB"};
          int vals[] = {h.velocity, h.fadeIn,
                        (h.length + t.stride() - 1) / t.stride(),
                        h.probability};
          label = l[i];
          snprintf(b, sizeof(b), "%d", vals[i]);
          if (i == 2 && !h.length)
            strcpy(b, "FULL");
          unit = i == 1   ? "MS"
                 : i == 2 ? (h.length ? "STEPS" : "ONE SHOT")
                 : i == 3 ? "PERCENT"
                          : "";
        } else if (a.page == Pattern && a.bank == 0) {
          const char *l[] = {"KEY", "VEL", "LENGTH", "PROB"};
          label = l[i];
          if (i == 0 && t.instrument)
            label = "NOTE";
          int val[] = {s.key, s.velocity,
                       (s.length + t.stride() - 1) / t.stride(), s.probability};
          if (i == 0)
            snprintf(b, sizeof(b), "%s",
                     t.instrument ? midiNote(s.note) : note(val[i]));
          else
            snprintf(b, sizeof(b), "%d", val[i]);
          unit = i == 2 ? "STEPS" : i == 3 ? "PERCENT" : "";
          if (i == 2 && !t.instrument && t.keys[s.key].mode == 0) {
            strcpy(b, "OFF");
            unit = "ONE SHOT";
          }
        } else if (a.page == Pattern && a.bank == 3) {
          label = i == 0 ? "CYCLE" : "STEP";
          if (i == 0)
            snprintf(b, sizeof(b), "%d", t.cells());
          else
            snprintf(b, sizeof(b), "1/%d", t.division);
          unit = i == 0 ? "CELLS" : "RESOLUTION";
        } else if (a.fxPage() || (a.page == Pattern && a.bank == 2)) {
          const auto &f = a.page == Mix ? a.project.fx : t.fx;
          label = fxLabel(f.type, i);
          int value = f.values[f.type][i];
          if (a.page == Pattern) {
            value = s.fxLocks[f.type][i];
            unit = "STEP LOCK";
          }
          if (value == Inherit)
            strcpy(b, "--");
          else if (f.type == 0 && i == 0) {
            const char *n[] = {"1/16", "1/8", "3/16", "1/4",
                               "3/8",  "1/2", "3/4",  "1 BAR"};
            snprintf(b, sizeof(b), "%s", n[value]);
          } else
            snprintf(b, sizeof(b), "%d", value);
        } else if (a.page == Tape) {
          if (a.tapeTab == 0) {
            const char *l[] = {"SOURCE", "LENGTH", "--", "--"};
            label = l[i];
            if (i == 0) {
              if (a.captureSource == 0)
                strcpy(b, "MASTER");
              else
                snprintf(b, sizeof(b), "TR %02d", a.captureSource);
            } else if (i == 1)
              snprintf(b, sizeof(b), "%d", a.captureBars);
            else
              strcpy(b, "--");
            unit = i == 1 ? "BARS" : "";
          } else {
            const char *l[] = {"START", "END", "FADE IN", "FADE OUT"};
            label = l[i];
            auto &cut = a.slices[a.sliceIndex];
            int val[] = {cut.start, cut.end, cut.fadeIn, cut.fadeOut};
            if (a.sliceCount)
              snprintf(b, sizeof(b), "%d", val[i]);
            else
              strcpy(b, "--");
            unit = "MS";
          }
        } else if (a.page == Mix) {
          const char *l[] = {"LEVEL", "PAN", "MASTER", "MUTE"};
          label = l[i];
          int vals[] = {t.level, t.pan, a.project.master, t.mute};
          snprintf(b, sizeof(b), "%d", vals[i]);
          unit = i == 3 ? "0 OFF 1 ON" : "";
        } else if (a.granularTrack() &&
                   (a.page == Sampler || (a.page == Pattern && a.bank == 1))) {
          int parameter=i+a.synthBank*4;
          const auto &spec=GranularSpecs[parameter]; label=spec.label;
          int value=a.page==Sampler?t.granular.values[parameter]:s.grainLocks[parameter];
          if (a.page==Sampler && parameter==4 && !(a.buttons.held & bit(Y))) {
            auto view=a.engine->grainView[a.track].read();
            if(view.running) value=int((view.head*1000u+32768u)/65536u);
          }
          unit=a.page==Pattern?"STEP LOCK":spec.unit;
          if(a.page==Sampler && (parameter==0 || parameter==4) && t.granular.values[7]==100) unit="SCAN ANCHOR";
          if(value==Inherit) strcpy(b,"--");
          else if(parameter==0) snprintf(b,sizeof(b),"%.2f",value*.01);
          else if(parameter>=4 && parameter<=6) snprintf(b,sizeof(b),"%.1f",value*.1);
          else snprintf(b,sizeof(b),"%d",value);
        } else if (t.instrument &&
                   (a.page == Sampler || (a.page == Pattern && a.bank == 1))) {
          int type = t.instrument - 1, parameter = i + a.synthBank * 4;
          label = synthLabel(t.instrument, parameter);
          int value = a.page == Sampler ? t.synthParams[type][parameter]
                                        : s.synthLocks[type][parameter];
          unit = a.page == Pattern ? "STEP LOCK"
                                   : synthUnit(t.instrument, parameter);
          if (value == Inherit)
            strcpy(b, "--");
          else if (t.instrument == FmInstrument && parameter == 1)
            snprintf(b, sizeof(b), "%s", ratioName(value));
          else
            snprintf(b, sizeof(b), "%d", value);
        } else {
          const char *l[] = {"PITCH", "START", "END", "LEVEL"};
          label = l[i];
          int val[] = {k.pitch, k.start, k.end, k.level};
          if (a.page == Pattern) {
            const auto &hit = t.hits[a.key][a.step];
            if (hit.locks[i] == Inherit) {
              strcpy(b, "--");
              disabled = true;
            } else
              val[i] = hit.locks[i];
            unit = "STEP LOCK";
          } else
            unit = i == 0 ? "SEMITONES" : "PERCENT";
          if (!disabled)
            snprintf(b, sizeof(b), "%d", val[i]);
          if (a.page == Sampler && k.asset < 0)
            strcpy(b, "--");
        }
        parameterField(c, v, i, label, b, unit);
      }
    }
  }
  c.rect(0, 445, 640, 1, LINE);
  const char *hint =
      a.naming                   ? "A TYPE / X DELETE / START SAVE / B CANCEL"
      : a.importAssign           ? "A NAME + ASSIGN / X PLAY / B VOICE"
      : a.assignMode             ? "A SELECT / X AUDITION / B VOICE"
      : a.fxPage() && a.fxDetail ? "X ON-OFF / L1+ARROWS FX / B CHAIN"
      : a.page == SongPage       ? "Y+UP-DOWN MOVE / R1+L-R PAGE / L1+R1 MENU"
      : a.page == Atlas          ? "A OPEN / L1-R1 TAB / L1+R1 OPTIONS"
      : a.page == Grid           ? "A OPEN / L1 R1 TRACK / L1+R1 OPTIONS"
      : a.fxPage()               ? "A EDIT / X BYPASS / Y+ARROWS ORDER"
      : a.page == Mix            ? "L1/R1 BANK / A MUTE / L1+R1 OPTIONS"
      : a.page == Pattern && a.bank == 2
          ? "L1+ARROWS FX / L1/R1 BANK / L1+R1 OPTIONS"
      : a.page == Pattern && !a.project.tracks[a.track].instrument
          ? "L1+ARROWS SLOT / L1/R1 BANK / L1+R1 OPTIONS"
      : a.page == Pattern && a.bank == 1 && a.granularTrack() ? "L1+ARROWS VOICE PAGE / L1/R1 BANK"
      : a.page == Pattern && a.bank == 1 ? "L1+ARROWS CORE-CHAR / L1/R1 BANK"
      : a.page == Pattern ? "ARROWS STEP / L1/R1 BANK / L1+R1 OPTIONS"
      : a.page == Sampler && a.granularTrack()
          ? (a.available(a.project.tracks[a.track].granular.asset) ? "A AUDITION / X FREEZE / L1/R1 BANK" : "A ASSIGN / L1/R1 BANK / L1+R1 OPTIONS")
      : a.page == Sampler && a.project.tracks[a.track].instrument
          ? "A AUDITION / L1/R1 BANK / L1+R1 OPTIONS"
      : a.page == Sampler && a.project.tracks[a.track].keys[a.key].asset < 0
          ? "ARROWS SLOT / A ASSIGN / L1+R1 OPTIONS"
      : a.page == Sampler ? "ARROWS SLOT / A PLAY / L1+R1 OPTIONS"
      : a.page == Tape && a.tapeTab == 0
          ? "L1/R1 BANK / A CAPTURE / X IMPORT WAV"
      : a.page == Tape && !a.optionCount()
          ? "L1/R1 BANK / A SELECT / X PLAY OR IMPORT"
      : a.page == Tape ? "L1/R1 BANK / ARROWS ITEM / L1+R1 OPTIONS"
                       : "A EDIT / X BYPASS / Y+ARROWS ORDER";
  bool tempoFocus = a.page != Atlas && !a.options && !a.naming &&
                    (a.buttons.held & bit(Select));
  text(c, 18, 457,
       tempoFocus ? "TEMPO / L-R 1 BPM / U-D 10 BPM"
       : (a.buttons.held & 0xf0) && !a.naming && !a.options &&
               !a.sequencePicker && !a.confirmDelete
           ? "L-R FINE / U-D COARSE / HOLD TO REPEAT"
       : a.status[0] && a.nowUs < a.statusUntil ? a.status
                                                : hint,
       DIM, 2, 50);
  if (a.options) {
    c.rect(54, 104, 532, 320, PANEL);
    c.box(54, 104, 532, 320, LINE);
    text(c, 72, 120,
         a.manageMenu == 1   ? "SEQUENCE / B CLOSE"
         : a.manageMenu == 2 ? "TRACK / B CLOSE"
                             : "OPTIONS / B CLOSE",
         DIM);
    if (a.optionCount() > 8) {
      snprintf(b, sizeof(b), "%d/%d", a.option / 8 + 1,
               (a.optionCount() + 7) / 8);
      text(c, 510, 120, b, DIM);
    }
    int first = a.option / 8 * 8;
    for (int i = first; i < a.optionCount() && i < first + 8; i++) {
      int y = 155 + (i - first) * 31;
      if (i == a.option)
        c.box(68, y - 6, 504, 27, TEXT);
      text(c, 80, y, a.optionName(i), i == a.option ? TEXT : DIM);
    }
  }
  drawSequencePicker(c, a);
  drawOverlay(c, a);
  if (a.voicePicker) {
    c.rect(54, 104, 532, 320, PANEL);
    c.box(54, 104, 532, 320, LINE);
    text(c, 72, 120,
         a.voicePicker == 3   ? "ADD TRACK / B CANCEL"
         : a.voicePicker == 1 ? "INSTRUMENT / B CANCEL"
                              : "PRESET / B CANCEL",
         DIM);
    for (int i = 0; i < (a.voicePicker == 2 ? 4 : InstrumentCount); i++) {
      int y = 162 + i * 40;
      if (i == a.voiceChoice)
        c.box(68, y - 8, 504, 33, TEXT);
      text(c, 80, y,
           a.voicePicker == 1 || a.voicePicker == 3
               ? instrumentName(i)
               : presetName(a.project.tracks[a.track].instrument, i),
           i == a.voiceChoice ? TEXT : DIM);
    }
    text(c, 72, 380, "D-PAD SELECT / A APPLY", DIM);
  }
  c.box(0, 0, 640, 480, LINE);
}
