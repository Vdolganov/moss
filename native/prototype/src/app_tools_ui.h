#pragma once
// Included inside namespace moss after App and note().
inline const char *leafName(const char *p) {
  const char *s = strrchr(p, '/');
  const char *w = strrchr(p, '\\');
  if (w && (!s || w > s))
    s = w;
  return s ? s + 1 : p;
}
 #include "app_sample_folders_ui.h"
inline void drawTools(lab::Canvas &c, const App &a) {
  using namespace lab;
  char b[160];
  if (a.fxPage()) {
    const auto f = a.effectView();
    const int *p = f.values[f.type];
    if (!a.fxDetail) {
      if (a.page != Mix)
        text(c, 18, 109, "TRACK / EFFECT CHAIN", DIM);
      for (int i = 0; i < EffectTypes; i++) {
        int type = f.order[i], y = 145 + i * 45;
        if (type == f.type)
          c.rect(18, y - 9, 604, 38, PANEL);
        snprintf(b, sizeof(b), "%d  %s", i + 1, fxName(type));
        text(c, 30, y, b, TEXT, 3);
        text(c, 510, y + 4, f.active(type) ? "ON" : "BYPASS",
             f.active(type) ? TEXT : DIM);
        c.rect(18, y + 30, 604, 1, LINE);
      }
      text(c, 18, 389, "A EDIT / X ON-OFF / Y+UP-DOWN ORDER", DIM);
      snprintf(b, sizeof(b), "%d/2 ACTIVE / VOICE > FX CHAIN > MIX", f.count());
      text(c, 18, 418, b, DIM);
      return;
    }
    if (a.page != Mix) {
      snprintf(b, sizeof(b), "%02d %.24s", a.track + 1,
               a.project.tracks[a.track].name);
      text(c, 18, 108, b, DIM);
    }
    snprintf(b, sizeof(b), "%s / %s", fxName(f.type),
             f.enabled ? "ON" : "BYPASS");
    text(c, 18, 137, b, TEXT, 3);
    if(f.type!=Reel)text(c, 18, 173,
         a.page == Mix ? "ALL TRACKS > MASTER EFFECT > OUTPUT"
                       : "VOICE > TRACK EFFECT > MIX",
         DIM);
    if(f.type!=Reel)c.rect(18, 220, 604, 98, PANEL);
    if(f.type!=Reel)c.rect(34, 299, 572, 1, LINE);
    if (f.type == 0) {
      float level = 1;
      for (int i = 0; i < 9; i++) {
        int x = 38 + i * 63, h = int(64 * level);
        c.rect(x, 299 - h, 2, h, TEXT);
        level *= p[1] / 100.f;
      }
      const char *times[] = {"1/16", "1/8", "3/16", "1/4",
                             "3/8",  "1/2", "3/4",  "1 BAR"};
      snprintf(b, sizeof(b), "TAP SPACING %s / FEEDBACK %d%%", times[p[0]],
               p[1]);
    } else if (f.type == 1) {
      for (int i = 0; i < 140; i++) {
        int h = int(67 * expf(-6.907755f*float(i)/140));
        c.rect(36 + i * 4, 299 - h, 2, h, TEXT);
      }
      snprintf(b, sizeof(b), "TAIL %.1f S / PRE %d MS / DESIGN ENVELOPE",pondSeconds(p[1]),p[4]);
    } else if (f.type == 2) {
      c.rect(320, 228, 1, 78, LINE);
      for (int i = 0; i < 280; i++) {
        float x = (i - 140) / 140.f, v = x * (1 + p[0] * .15f);
        v = v / (1 + fabsf(v));
        int levels = 1 << (p[1] - 1);
        v = roundf(v * levels) / levels;
        c.rect(38 + i * 2, 268 - int(v * 37), 2, 2, TEXT);
      }
      snprintf(b, sizeof(b), "TRANSFER CURVE / %d BIT / HOLD 1:%d", p[1], p[2]);
    } else if(f.type==Drive) {
      c.rect(320,228,1,78,LINE);
      float gain=(1+63*(p[0]/100.f)*(p[0]/100.f))/(p[4]==0?1.5f:p[4]==1?4.f:8.f);
      float bias=p[5]*.003f,offset=DriveDSP::shape(bias,p[4]);
      for(int i=0;i<280;++i) {
        float v=DriveDSP::shape((i-140)/140.f*gain+bias,p[4])-offset;
        c.rect(38+i*2,268-int(fxClamp(v,-1,1)*37),2,2,TEXT);
      }
      snprintf(b,sizeof(b),"%s / TRANSFER CURVE / OUTPUT %d%%",driveCurveName(p[4]),p[2]);
    } else {
      drawReel(c,a,f);
      return;
    }
    text(c, 18, 328, b, DIM, 2, 50);
  }
  if (a.page != Tape)
    return;
  if(a.fourSlices&&(!a.assignMode||a.importAssign)){
    const char *tabs[]={"SOURCE","MOVE","TRIM","FINE","LIBRARY"};
    int active=a.tapeTab==0?0:a.tapeTab==2?4:1+a.sliceBank;
    ui::tabs(c,104,tabs,5,active);
  }else{const char *tabs[]={"SOURCE","EDIT","LIBRARY"};ui::tabs(c,104,tabs,3,a.tapeTab);}
  if (a.tapeTab == 0) {
    text(c, 18, 151, "TAPE / RESAMPLE", TEXT, 3);
    text(c, 18, 196, "TRACK + SPACE OR MASTER + ALL EFFECTS", DIM);
    unsigned state = a.captureState(),
             frames = __atomic_load_n(&a.engine->capture.shownFrames,
                                      __ATOMIC_ACQUIRE);
    unsigned limit =
        a.captureBusy()
            ? __atomic_load_n(&a.engine->capture.shownLimit, __ATOMIC_ACQUIRE)
            : 0;
    c.rect(18, 233, 604, 5, LINE);
    if (limit)
      c.rect(18, 233, int(uint64_t(frames) * 604 / limit), 5, TEXT);
    if (state == 1)
      snprintf(b, sizeof(b), "RECORDING %u.%u S / A FINISH",
               frames / a.engine->rate, (frames * 10 / a.engine->rate) % 10);
    else if (state == 2)
      snprintf(b, sizeof(b),
               a.captureWritePending ? "SAVING CAPTURE / PLAYBACK CONTINUES"
                                     : "CAPTURE IN RAM / A RETRY SAVE");
    else
      snprintf(b, sizeof(b), "A CAPTURE / X IMPORT WAV / L1/R1 BANK");
    text(c, 18, 259, b, TEXT);
    snprintf(b, sizeof(b), "%d BAR / %.1f S / LIMIT %.1f S", a.captureBars,
             a.captureBars * 240.f / a.project.bpm,
             float(Capture::MaxFrames) / a.engine->rate);
    text(c, 18, 299, b, DIM);
  } else if (a.tapeTab == 1) {
    if(a.fourSlices){drawFourSlices(c,a);return;}
    if (!a.sliceCount) {
      text(c, 18, 184, "NO SOURCE OPEN", TEXT, 3);
      text(c, 18, 236, "L1 SOURCE / X IMPORT OR A CAPTURE", DIM);
      return;
    }
    const auto &s = a.slices[a.sliceIndex];
    text(c, 18, 149, leafName(a.source.path), TEXT, 2, 49);
    snprintf(b, sizeof(b), "SLICE %02d/%02d / %u HZ / %u CH", a.sliceIndex + 1,
             a.sliceCount, a.source.rate, a.source.channels);
    text(c, 18, 177, b, DIM);
    int duration = a.durationMs();
    int left = 18 + int(int64_t(s.start) * 604 / (duration ? duration : 1)),
        right = 18 + int(int64_t(s.end) * 604 / (duration ? duration : 1));
    c.rect(left, 211, right - left, 62, PANEL);
    int maximum = 256;
    for (int v : a.source.peaks)
      if (v > maximum)
        maximum = v;
    for (int i = 0; i < 64; i++) {
      int h = clamp(a.source.peaks[i] * 28 / maximum, 0, 28);
      c.rect(18 + i * 604 / 64, 242 - h, 4, h * 2 + 1, DIM);
    }
    c.rect(left, 206, 1, 73, TEXT);
    c.rect(right - 1, 206, 1, 73, TEXT);
    if (a.previewing() && !strcmp(a.streamReader.source.path, a.source.path)) {
      unsigned pos =
          __atomic_load_n(&a.engine->stream.shownFrame, __ATOMIC_ACQUIRE);
      int x = 18 + int(uint64_t(pos) * 603 / a.source.frames);
      c.rect(x, 205, 2, 75, TEXT);
    }
    unsigned bytes = a.sliceFrames(s) * a.source.channels * 2;
    snprintf(b, sizeof(b), "%u / 1024 KIB%s", (bytes + 1023) / 1024,
             a.previewing()
                 ? (a.previewWhole ? " / SOURCE PLAY" : " / SLICE PLAY")
                 : " / READY");
    text(c, 18, 294, b, TEXT);
    text(c, 18, 326,
         a.importAssign ? "A NAME + ASSIGN / X PLAY / B CANCEL"
                        : "A NAME + SAVE / X PLAY-STOP / OPTIONS",
         DIM);
  } else {
    drawSampleLibrary(c,a);
  }
}
inline void drawOverlay(lab::Canvas &c, const App &a) {
  using namespace lab;
  char b[160];
  if (a.filePicker) drawFolderPicker(c,a);
  if(a.naming){
    ui::box(c,16,16,608,448,0,LINE,BG);
    const char *title=a.nameTarget==8||(a.nameTarget==5&&a.fourSlices)?"KIT NAME":a.nameTarget==6?"NEW SONG":a.nameTarget==7?"SONG NAME":a.nameTarget==3?"NEW SEQUENCE":a.nameTarget==1?"SEQUENCE NAME":a.nameTarget==2?"TRACK NAME":"SAMPLE NAME";
    ui::text(c,32,32,title,TEXT,ui::Heading);
    snprintf(b,sizeof(b),"%d / %d",int(strlen(a.nameEdit)),NameLimit);ui::text(c,608-ui::textWidth(b,ui::Small),38,b,DIM,ui::Small);
    c.box(32,80,576,40,LINE);if(a.nameSelected)c.rect(40,88,int(strlen(a.nameEdit))*12,20,TEXT);
    ui::text(c,40,92,a.nameEdit,a.nameSelected?BG:TEXT);if(!a.nameSelected)c.rect(40+a.nameCursor*12,110,10,2,TEXT);
    for(int i=0;i<NameKeyCount;++i){int x=32+(i%NameColumns)*58,y=144+(i/NameColumns)*48;
      ui::box(c,x,y,50,40,0,i==a.nameKey?TEXT:LINE,i==a.nameKey?TEXT:BG);
      char letter[2]={i<NameDelete?NameCharacters[i]:char(0),0};const char *label=i==NameDelete?"DEL":i==NameSave?"SAVE":letter[0]==' '?"SPC":letter;
      ui::text(c,x+(50-ui::textWidth(label))/2,y+13,label,i==a.nameKey?BG:TEXT);
    }
    ui::text(c,32,392,"A TYPE / X DELETE / START SAVE",DIM,ui::Small);ui::text(c,32,428,"L1/R1 CURSOR / B CANCEL",DIM,ui::Small);
  }

  if (a.confirmDelete) {
    c.rect(36, 154, 568, 200, PANEL);
    c.box(36, 154, 568, 200, LINE);
    ui::text(c, 56, 177,
         a.confirmTarget==7?"CLEAR ALL AT STEP?":a.confirmTarget==8?"CLEAR PATTERN?":a.confirmTarget==9?"REPLACE FOUR SLOTS?":a.confirmTarget==10?"DELETE KIT FOLDER?":a.confirmTarget == 3   ? "DELETE SONG?"
         : a.confirmTarget == 1 ? "DELETE SEQUENCE?"
         : a.confirmTarget == 2 ? "DELETE TRACK?"
                                : "DELETE UNUSED SAMPLE?",
         TEXT,ui::Heading);
    text(c, 56, 223,
         a.confirmTarget>=7?a.project.tracks[a.track].name:a.confirmTarget == 3   ? a.song.name
         : a.confirmTarget == 1 ? a.project.name
         : a.confirmTarget == 2 ? a.project.tracks[a.track].name
                                : a.assetName(a.asset),
         TEXT, 2, 38);
    text(c, 56, 286, "A CONFIRM / B CANCEL", DIM);
  }
}
