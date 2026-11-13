#pragma once
// Included inside namespace moss after App and note().
inline const char *leafName(const char *p) {
  const char *s = strrchr(p, '/');
  const char *w = strrchr(p, '\\');
  if (w && (!s || w > s))
    s = w;
  return s ? s + 1 : p;
}
inline void drawTools(lab::Canvas &c, const App &a) {
  using namespace lab;
  char b[160];
  if (a.fxPage()) {
    const auto &f = a.page == Mix ? a.project.fx : a.project.tracks[a.track].fx;
    const int *p = f.values[f.type];
    if (!a.fxDetail) {
      text(c, 18, 109,
           a.page == Mix ? "MASTER / EFFECT CHAIN" : "TRACK / EFFECT CHAIN",
           DIM);
      for (int i = 0; i < 4; i++) {
        int type = f.order[i], y = 155 + i * 55;
        if (type == f.type)
          c.rect(18, y - 11, 604, 46, PANEL);
        snprintf(b, sizeof(b), "%d  %s", i + 1, fxName(type));
        text(c, 30, y, b, TEXT, 3);
        text(c, 510, y + 4, f.active(type) ? "ON" : "BYPASS",
             f.active(type) ? TEXT : DIM);
        c.rect(18, y + 35, 604, 1, LINE);
      }
      text(c, 18, 389, "A EDIT / X ON-OFF / Y+UP-DOWN ORDER", DIM);
      text(c, 18, 418, "VOICE > 1 > 2 > 3 > 4 > MIX", DIM);
      return;
    }
    if (a.page == Mix)
      text(c, 18, 104, "Y  LEVELS / MASTER FX", DIM);
    else {
      snprintf(b, sizeof(b), "%02d %.24s", a.track + 1,
               a.project.tracks[a.track].name);
      text(c, 18, 108, b, DIM);
    }
    snprintf(b, sizeof(b), "%s / %s", fxName(f.type),
             f.enabled ? "ON" : "BYPASS");
    text(c, 18, 137, b, TEXT, 3);
    text(c, 18, 181,
         a.page == Mix ? "ALL TRACKS > MASTER EFFECT > OUTPUT"
                       : "VOICE > TRACK EFFECT > MIX",
         DIM);
    c.rect(18, 220, 604, 98, PANEL);
    c.rect(34, 299, 572, 1, LINE);
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
        int h = int(67 * expf(-float(i) / (15 + p[1])));
        c.rect(36 + i * 4, 299 - h, 2, h, TEXT);
      }
      snprintf(b, sizeof(b), "DECAY ENVELOPE / DAMP %d%% / SIZE %d%%", p[2],
               p[0]);
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
    } else {
      for (int i = 0; i < 280; i++) {
        float phase = float(i) / 140 - 1;
        c.rect(38 + i * 2, 266 + int(FxState::sine(phase) * p[0] * .25f), 2, 2,
               TEXT);
      }
      snprintf(b, sizeof(b), "SPEED MODEL / WEAR %d%% / NOISE %d%%", p[1],
               p[2]);
    }
    text(c, 18, 328, b, DIM, 2, 50);
  }
  if (a.page != Tape)
    return;
  const char *tabs[] = {"SOURCE", "EDIT", "LIBRARY"};
  if (a.assignMode || a.importAssign) {
    snprintf(b, sizeof(b), "VOICE / %s / %s",
             note(a.importAssign ? a.importKey : a.key),
             a.tapeTab == 1 ? "PREPARE + ASSIGN" : "CHOOSE SAMPLE");
    text(c, 18, 108, b, TEXT);
    c.rect(18, 128, 604, 1, LINE);
  } else {
    text(c, 18, 108, "Y", DIM);
    for (int i = 0; i < 3; i++) {
      text(c, 58 + i * 178, 108, tabs[i], i == a.tapeTab ? TEXT : DIM);
      if (i == a.tapeTab)
        c.rect(56 + i * 178, 128, 130, 1, TEXT);
    }
  }
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
      snprintf(b, sizeof(b), "CAPTURE IN RAM / A RETRY SAVE");
    else
      snprintf(b, sizeof(b), "A CAPTURE / X IMPORT WAV / Y BANK");
    text(c, 18, 259, b, TEXT);
    snprintf(b, sizeof(b), "%d BAR / %.1f S / LIMIT %.1f S", a.captureBars,
             a.captureBars * 240.f / a.project.bpm,
             float(Capture::MaxFrames) / a.engine->rate);
    text(c, 18, 299, b, DIM);
  } else if (a.tapeTab == 1) {
    if (!a.sliceCount) {
      text(c, 18, 184, "NO SOURCE OPEN", TEXT, 3);
      text(c, 18, 236, "Y SOURCE / X IMPORT OR A CAPTURE", DIM);
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
    int count = a.library ? a.library->count() : 0;
    unsigned bytes = a.library ? a.library->bytes() : 0;
    snprintf(b, sizeof(b), "%d / 32 ASSETS / %u KIB PCM", count,
             (bytes + 1023) / 1024);
    text(c, 18, 149, b, DIM);
    if (!count && !a.assignMode) {
      text(c, 18, 217, "EMPTY / IMPORT AND SAVE A SLICE", TEXT);
      return;
    }
    int ids[Assets + 1], rows = 0, selectedRow = 0;
    ids[rows++] = -1;
    for (int id = 0; id < Assets; id++)
      if (a.library->pcm[id].frames)
        ids[rows++] = id;
    for (int i = 0; i < rows; i++)
      if (ids[i] == a.asset)
        selectedRow = i;
    int first = selectedRow / 5 * 5;
    for (int i = first; i < rows && i < first + 5; i++) {
      int id = ids[i], y = 185 + (i - first) * 41;
      if (id == a.asset)
        c.rect(18, y - 8, 604, 37, PANEL);
      if (id < 0)
        snprintf(b, sizeof(b), "+ IMPORT FROM DEVICE");
      else
        snprintf(b, sizeof(b), "%02d  %s", id, a.library->names[id]);
      text(c, 28, y, b, TEXT, 2, 39);
      if (id >= 0) {
        snprintf(b, sizeof(b), "%u K",
                 (a.library->pcm[id].bytes + 1023) / 1024);
        text(c, 524, y, b, DIM);
      }
      c.rect(18, y + 28, 604, 1, LINE);
    }
    text(c, 18, 412,
         a.assignMode ? "A ASSIGN TO KEY / X AUDITION / B CANCEL"
                      : "A EDIT COPY / X AUDITION / OPTIONS MANAGE",
         DIM, 2, 50);
  }
}
inline void drawOverlay(lab::Canvas &c, const App &a) {
  using namespace lab;
  char b[160];
  if (a.filePicker) {
    c.rect(18, 104, 604, 331, PANEL);
    c.box(18, 104, 604, 331, LINE);
    text(c, 32, 122, "IMPORT / A OPEN / X PLAY-STOP / B BACK", TEXT);
    int count = a.library ? a.library->sourceCount : 0;
    if (!count && !a.assignMode) {
      text(c, 32, 192, "NO WAV FOUND", TEXT);
      text(c, 32, 230, "COPY WAV INTO MOSS/imports", DIM);
    }
    for (int i = a.fileCursor / 7 * 7;
         i < count && i < (a.fileCursor / 7 + 1) * 7; i++) {
      int y = 163 + (i % 7) * 35;
      if (i == a.fileCursor)
        c.box(28, y - 7, 584, 31, TEXT);
      text(c, 40, y, leafName(a.library->sourcePaths[i]),
           i == a.fileCursor ? TEXT : DIM, 2, 46);
    }
    snprintf(b, sizeof(b), "%d / %d FILES", count ? a.fileCursor + 1 : 0,
             count);
    if (a.previewing()) {
      unsigned pos =
          __atomic_load_n(&a.engine->stream.shownFrame, __ATOMIC_ACQUIRE);
      snprintf(b, sizeof(b), "PLAY %u / %u S / X STOP",
               pos / a.streamReader.source.rate,
               a.streamReader.source.frames / a.streamReader.source.rate);
    }
    text(c, 32, 413, b, DIM);
  }
  if (a.naming) {
    c.rect(36, 154, 568, 200, PANEL);
    c.box(36, 154, 568, 200, LINE);
    const char *title = a.nameTarget == 3   ? "NEW PROJECT"
                        : a.nameTarget == 1 ? "PROJECT NAME"
                        : a.nameTarget == 2 ? "TRACK NAME"
                                            : "SAMPLE NAME";
    snprintf(b, sizeof(b), "%s / A SAVE / B CANCEL", title);
    text(c, 56, 177, b, TEXT);
    text(c, 56, 224, a.nameEdit, TEXT);
    c.rect(56 + a.nameCursor * 12, 245, 10, 2, TEXT);
    text(c, 56, 286, "LEFT RIGHT POSITION / UP DOWN LETTER", DIM);
  }
  if (a.confirmDelete) {
    c.rect(36, 154, 568, 200, PANEL);
    c.box(36, 154, 568, 200, LINE);
    text(c, 56, 177,
         a.confirmTarget == 1   ? "DELETE PROJECT?"
         : a.confirmTarget == 2 ? "DELETE TRACK?"
                                : "DELETE UNUSED SAMPLE?",
         TEXT);
    text(c, 56, 223,
         a.confirmTarget == 1   ? a.project.name
         : a.confirmTarget == 2 ? a.project.tracks[a.track].name
                                : a.assetName(a.asset),
         TEXT, 2, 38);
    text(c, 56, 286, "A DELETE / B KEEP", DIM);
  }
}
