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
inline void drawSampleSlots(lab::Canvas &c,const App &a,int y){
  const auto &t=a.project.tracks[a.track];char label[32];
  unsigned live=__atomic_load_n(&a.engine->shownSampleKeys[a.track],__ATOMIC_ACQUIRE);
  for(int i=0;i<Keys;++i){int x=16+i*154;bool selected=i==a.key;
    ui::round(c,x,y,146,48,12,selected?lab::TEXT:lab::BG);
    uint32_t ink=selected?lab::BG:lab::DIM;
    snprintf(label,sizeof(label),"%d / %s",i+1,note(i));ui::text(c,x+12,y+8,label,ink,ui::Small);
    ui::text(c,x+12,y+28,t.keys[i].asset<0?"EMPTY":a.assetName(t.keys[i].asset),ink,ui::Small,12);
    if(live&(1u<<i)){ui::round(c,x+108,y+6,26,16,8,selected?lab::BG:lab::TEXT);ui::text(c,x+113,y+8,note(i),selected?lab::TEXT:lab::BG,ui::Small);}
  }
}
#include "app_pattern_ui.h"
#include "app_song_ui.h"
#include "app_synth_ui.h"
#include "app_granular_ui.h"
#include "app_reel_ui.h"
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
    ui::tabs(c,56,tabs,3,active,true,!a.assignMode&&!a.importAssign);
    c.rect(0,88,640,1,LINE);
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
        int y = 144 + (t - first) * 56;
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
        for (int id = 0; id < EffectTypes; id++)
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
      for (int t = 0; t < a.project.trackCount; t++) {
        const auto &tr = a.project.tracks[t];
        int y = 152 + t * 56;
        if (t == a.track)
          c.box(18, y - 10, 604, 44, TEXT);
        snprintf(b, sizeof(b), "%02d  %.18s", t + 1, tr.name);
        text(c, 30, y, b, TEXT, 2, 23);
        snprintf(b, sizeof(b), "%03d STEPS / %s", tr.cells(),
                 tr.mute ? "MUTED" : instrumentName(tr.instrument));
        ui::text(c, 30, y + 22, b, DIM,ui::Small);
        unsigned peak =
            __atomic_load_n(&a.engine->shownPeak[t], __ATOMIC_ACQUIRE);
        c.rect(360, y + 10, 240, 4, LINE);
        c.rect(360, y + 10, clamp(int(peak) * 240 / 16384, 0, 240), 4, TEXT);
        snprintf(b, sizeof(b), "LEVEL %d / PAN %+d", tr.level, tr.pan);
        ui::text(c, 360, y + 22, b, DIM,ui::Small,24);
      }
      if (!a.project.trackCount)
        text(c, 18, 190, "NO TRACKS / B GRID / A ADD TRACK", DIM);
    }
    if(a.page==Pattern)drawPattern(c,a);
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
    if(a.page==Pattern||a.page==Sampler||(a.fxPage()&&a.fxDetail)){
      c.rect(0,326,640,44,BG);
      drawParameterTabs(c,a);
    }
    if (a.page == Pattern || a.page == Sampler ||
        (a.page == Mix && (a.project.trackCount || a.mixTab == 1)) ||
        a.page == Space || (a.page == Tape && a.tapeTab < 2)) {
      View v;
      v.buttons = a.buttons;
      v.holds = a.holds;
      auto &t = a.project.tracks[a.track];
      auto &s = t.steps[a.step];
      auto &k = t.keys[a.key];
      int fieldCount = a.fxPage() && !a.fxDetail ? 0
                       : a.page==Tape && a.tapeTab==0 ? 2 : 4;
      for (int i = 0; i < fieldCount; i++) {
        const char *label = "";
        const char *unit = "";
        bool disabled = false;
        if (a.page == Pattern && a.bank == 0 && !t.instrument) {
          const auto &h = t.hits[a.key][a.step];
          const char *l[] = {"VELOCITY", "FADE IN", "NOTE LEN", "PROB"};
          int vals[] = {h.velocity, h.fadeIn,
                        (h.length + t.stride() - 1) / t.stride(),
                        h.on==EmptyEvent && s.on==LockEvent?s.probability:h.probability};
          label = l[i];
          snprintf(b, sizeof(b), "%d", vals[i]);
          if (i == 2 && !h.length)
            strcpy(b, "FULL");
          unit = i == 1   ? "MS"
                 : i == 2 ? (h.length ? "STEPS" : "ONE SHOT")
                 : i == 3 ? "PERCENT"
                          : "";
        } else if (a.page == Pattern && a.bank == 0) {
          const char *l[] = {"KEY", "VEL", "NOTE LEN", "PROB"};
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
          const char *labels[]={"CYCLE","DIV","REPEAT","TYPE"}; label=labels[i];
          if(a.timingBank && i==0) { label="SWING"; snprintf(b,sizeof(b),"%d%%",t.swing); }
          else if(a.timingBank && i==1) {
            label="SHIFT";
            if(s.shift==Inherit) strcpy(b,"--"); else snprintf(b,sizeof(b),"%+d%%",s.shift);
          }
          else if(i==0) snprintf(b,sizeof(b),"%d",t.cells());
          else if(i==1) snprintf(b,sizeof(b),"1/%d",t.division);
          else if(i==2) {
            if(s.repeat==Inherit) strcpy(b,"--"); else if(!s.repeat) strcpy(b,"OFF"); else snprintf(b,sizeof(b),"1/%d",s.repeat);
          } else strcpy(b,selectedEvent(t,a.step,a.key)==LockEvent?"LOCK":eventName(selectedEvent(t,a.step,a.key)));
          unit=i==0?"TRACK CELLS":i==1?"RESOLUTION":i==2?"TRACK AUDIO":"STEP EVENT";
          if(a.timingBank && i<2) unit=i==0?"WHOLE TRACK":"STEP LOCK";
        } else if (a.fxPage() || (a.page == Pattern && a.bank == 2)) {
          const auto f = a.effectView();
          int parameter=i+clamp(a.fxBank,0,fxBanks(f.type)-1)*4;
          label = fxLabel(f.type, parameter);
          unit = FxSpecs[f.type][parameter].unit;
          int value = f.values[f.type][parameter];
          if (a.page == Pattern) {
            value = s.fxLocks[f.type][parameter];
            unit = FxSpecs[f.type][parameter].unit;
          }
          if (value == Inherit)
            strcpy(b, "--");
          else if(value==ResetLock) strcpy(b,"RESET");
          else if (f.type == Orbit && parameter == 0) {
            const char *n[] = {"1/16", "1/8", "3/16", "1/4",
                               "3/8",  "1/2", "3/4",  "1 BAR"};
            snprintf(b, sizeof(b), "%s", n[value]);
          } else if(f.type==Pond && parameter==1) {
            snprintf(b,sizeof(b),"%.1f",pondSeconds(value));
          } else if(f.type==Drive && parameter==4) {
            snprintf(b,sizeof(b),"%s",driveCurveName(value));
          } else if(f.type==Orbit && parameter==4 && value==0) {
            strcpy(b,"OFF");
          } else if(f.type==Reel) {
            if(parameter==ReelHiss) {
              if(!value)strcpy(b,"OFF");else snprintf(b,sizeof(b),"%d",value-73);
            } else if(parameter==ReelWowRate)snprintf(b,sizeof(b),"%.2f",value*.01);
            else if(parameter==ReelTone||parameter==ReelBump||parameter==ReelOutput||
                    parameter==ReelWow||parameter==ReelFlutter||parameter==ReelFlutterRate)
              snprintf(b,sizeof(b),"%.1f",value*.1);
            else snprintf(b,sizeof(b),"%d",value);
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
          } else if(a.fourSlices && a.sliceBank==0) {
            const char *l[]={"PART 1","PART 2","PART 3","PART 4"};label=l[i];
            snprintf(b,sizeof(b),"%d",a.slices[i].start);
            unit="MS";
          } else if(a.fourSlices && a.sliceBank==2) {
            const char *labels[]={"START","END","ZOOM","VIEW"};label=labels[i];
            if(i<2){snprintf(b,sizeof(b),"%u",i?a.sliceEnd(a.slices[a.sliceIndex]):a.sliceStart(a.slices[a.sliceIndex]));unit="FRAMES";}
            else if(i==2){snprintf(b,sizeof(b),"%d",1<<a.fineZoom);unit="X";}
            else {unsigned first,last;a.fineWindow(first,last);snprintf(b,sizeof(b),"%u",unsigned(uint64_t(first)*1000/a.source.rate));unit="MS";}
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
          if(i==3)strcpy(b,t.mute?"ON":"OFF");
          unit=i==0||i==2?"%":i==1?"L/R":"";
        } else if (a.granularTrack() &&
                   (a.page == Sampler || (a.page == Pattern && a.bank == 1))) {
          int parameter=i+a.synthBank*4;
          const auto &spec=GranularSpecs[parameter]; label=spec.label;
          int value=a.page==Sampler?t.granular.values[parameter]:s.grainLocks[parameter];
          if (a.page==Sampler && parameter==4 && !(a.buttons.held & bit(Y))) {
            auto view=a.engine->grainView[a.track].read();
            if(view.running) value=int((view.head*1000u+32768u)/65536u);
          }
          unit=spec.unit;
          if(value==Inherit) strcpy(b,"--");
          else if(value==ResetLock) strcpy(b,"RESET");
          else if(parameter==0) snprintf(b,sizeof(b),"%.2f",value*.01);
          else if(parameter>=4 && parameter<=6) snprintf(b,sizeof(b),"%.1f",value*.1);
          else snprintf(b,sizeof(b),"%d",value);
        } else if (t.instrument &&
                   (a.page == Sampler || (a.page == Pattern && a.bank == 1))) {
          int type = t.instrument - 1, parameter = i + a.synthBank * 4;
          label = synthLabel(t.instrument, parameter);
          int value = a.page == Sampler ? t.synthParams[type][parameter]
                                        : s.synthLocks[type][parameter];
          unit = synthUnit(t.instrument, parameter);
          if (value == Inherit)
            strcpy(b, "--");
          else if(value==ResetLock) strcpy(b,"RESET");
          else if (t.instrument == FmInstrument && parameter == 1)
            snprintf(b, sizeof(b), "%s", ratioName(value));
          else if((t.instrument==MonoInstrument&&parameter==16)||(t.instrument==OrganismInstrument&&parameter==OrgRate)||(t.instrument==FmInstrument&&parameter==FmRate)){snprintf(b,sizeof(b),"%.2f",value*.01);unit="HZ";}
          else
            snprintf(b, sizeof(b), "%d", value);
        } else {
          const char *l[] = {"PITCH", "START", "END", "LEVEL"};
          label = l[i];
          int val[] = {k.pitch, k.start, k.end, k.level};
          if (a.page == Pattern) {
            const auto &hit = t.hits[a.key][a.step];
            if (!lockValue(hit.locks[i])) {
              strcpy(b,hit.locks[i]==ResetLock?"RESET":"--");
              disabled = true;
            } else
              val[i] = hit.locks[i];
            unit = i == 0 ? "ST" : "%";
          } else
            unit = i == 0 ? "SEMITONES" : "PERCENT";
          if (!disabled)
            snprintf(b, sizeof(b), "%d", val[i]);
          if (a.page == Sampler && k.asset < 0)
            strcpy(b, "--");
        }
        if(a.page==Pattern && a.bank==0 && selectedEvent(t,a.step,a.key)==LockEvent && i<3) { strcpy(b,"--"); unit="NO NOTE"; }
        parameterField(c, v, i, label, b, unit);
      }
    }
  }
  c.rect(0, ui::Footer, 640, 36, BG);
  c.rect(0, ui::Footer, 640, 1, LINE);
  const char *hint =
      a.naming                   ? "A TYPE / X DELETE / START SAVE / B CANCEL"
      : a.filePicker ? "A OPEN / X PLAY / B UP / R1+L-R PAGE"
      : a.page==Tape && a.tapeTab==1 && a.fourSlices ? "TAP Y B A X PLAY / L1+R1 OPTIONS / SEL BACK"
      : a.page==Tape && a.tapeTab==2 ? "A OPEN / X PLAY / B BACK / R1+L-R PAGE"
      : a.importAssign           ? "A NAME + ASSIGN / X PLAY / B VOICE"
      : a.assignMode             ? "A SELECT / X AUDITION / B VOICE"
      : a.fxPage() && a.fxDetail ? "L1/R1 PAGE / B CHAIN"
      : a.page == SongPage       ? "Y+UP-DOWN MOVE / R1+L-R PAGE / L1+R1 MENU"
      : a.page == Atlas          ? "A OPEN / L2-R2 TAB / L1+R1 OPTIONS"
      : a.page == Grid           ? "A OPEN / D-PAD TRACK / L1+R1 OPTIONS"
      : a.fxPage()               ? "A EDIT / X BYPASS / Y+ARROWS ORDER"
      : a.page == Mix            ? "L1/R1 BANK / A MUTE / L1+R1 OPTIONS"
      : a.page == Pattern && a.bank == 2
          ? "L1+U-D PAGE / L1-R1 TAB / R1+L-R GRID PAGE"
      : a.page == Pattern && a.bank == 3 ? "L1+U-D PAGE / L1-R1 TAB / R1+L-R GRID PAGE"
      : a.page == Pattern && !a.project.tracks[a.track].instrument
          ? "L1+L-R SLOT / L1-R1 TAB / R1+L-R GRID PAGE"
      : a.page == Pattern && a.bank == 1 && a.granularTrack() ? "L1+U-D PAGE / L1-R1 TAB / R1+L-R GRID PAGE"
      : a.page == Pattern && a.bank == 1 ? "L1+U-D PAGE / L1-R1 TAB / R1+L-R GRID PAGE"
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
  ui::text(c, 16, 456,
       tempoFocus ? "TEMPO / L-R 1 BPM / U-D 10 BPM"
       : (a.buttons.held & bit(L1)) && !a.options ? "L-R SLOT / U-D PAGE / R1 OPTIONS"
       : (a.buttons.held & 0xf0) && !a.naming && !a.options &&
               !a.sequencePicker && !a.confirmDelete
           ? "L-R FINE / U-D COARSE / HOLD TO REPEAT"
       : a.status[0] && a.nowUs < a.statusUntil ? a.status
                                                : hint,
       DIM, ui::Small, 59);
  if(a.options||a.sequencePicker||a.filePicker||a.naming||a.confirmDelete||a.voicePicker||a.lockMenu>=0)
    c.rect(0,0,640,480,BG);
  if(a.options){
    ui::box(c,16,16,608,448,0,LINE,BG);
    const char *title=a.manageMenu==1?"SEQUENCE":a.manageMenu==2?"TRACK":a.page==Pattern?"STEP OPTIONS":a.page==Tape?"TAPE OPTIONS":a.page==Sampler?"VOICE OPTIONS":a.page==SongPage?"SONG OPTIONS":"OPTIONS";
    ui::text(c,32,32,title,TEXT,ui::Heading);
    snprintf(b,sizeof(b),"%d / %d",a.option+1,a.optionCount());ui::text(c,608-ui::textWidth(b,ui::Small),38,b,DIM,ui::Small);
    int first=a.option/7*7;
    for(int i=first;i<a.optionCount()&&i<first+7;++i){int y=80+(i-first)*44;ui::row(c,y,i==a.option,40);ui::text(c,32,y+13,a.optionName(i),i==a.option?TEXT:DIM,ui::Body,47);}
    ui::text(c,32,428,"A SELECT / B CLOSE / R1+L-R PAGE",DIM,ui::Small);
  }
  drawSequencePicker(c, a);
  drawOverlay(c, a);
  if(a.voicePicker){
    ui::box(c,16,16,608,448,0,LINE,BG);ui::text(c,32,32,a.voicePicker==3?"ADD TRACK":a.voicePicker==1?"INSTRUMENT":"PRESET",TEXT,ui::Heading);
    int count=a.voicePicker==4?ReelPresetCount:a.voicePicker==2?synthPresetCount(a.project.tracks[a.track].instrument):InstrumentCount;
    int first=a.voiceChoice/5*5;
    for(int i=first;i<count&&i<first+5;++i){int y=104+(i-first)*56;ui::row(c,y,i==a.voiceChoice);ui::text(c,32,y+17,a.voicePicker==4?ReelPresetNames[i]:a.voicePicker==1||a.voicePicker==3?instrumentName(i):presetName(a.project.tracks[a.track].instrument,i),i==a.voiceChoice?TEXT:DIM,ui::Body,47);}
    ui::text(c,32,428,"D-PAD SELECT / A APPLY / B CANCEL",DIM,ui::Small);snprintf(b,sizeof(b),"%d / %d",a.voiceChoice+1,count);ui::text(c,608-ui::textWidth(b,ui::Small),38,b,DIM,ui::Small);
  }
  if(a.lockMenu>=0){
    ui::box(c,16,16,608,448,0,LINE,BG);ui::text(c,32,32,a.lockMenu==4?"PARAMETER":"STEP LOCK",TEXT,ui::Heading);
    const char *actions[]={"SET / CURRENT BASE VALUE","CLEAR / INHERIT","RESET / BASE AT THIS STEP"};
    const char *fields[]={"Y / FIRST PARAMETER","B / SECOND PARAMETER","A / THIRD PARAMETER","X / FOURTH PARAMETER"};
    for(int i=0;i<(a.lockMenu==4?4:3);++i){int y=112+i*56;ui::row(c,y,i==a.lockAction);ui::text(c,32,y+16,a.lockMenu==4?fields[i]:actions[i],TEXT);}
    ui::text(c,32,428,"A APPLY / B CLOSE / ONLY THIS PARAMETER",DIM,ui::Small);
  }
  c.box(0, 0, 640, 480, LINE);
}
