// Existing graphite list and physical Y/B/A/X parameter vocabulary.
inline void drawAtlas(lab::Canvas &c, const App &a) {
  using namespace lab;
  char b[100];
  bool songs = a.atlasTab != 0;
  text(c, 18, 18, "MOSS / LIBRARY", TEXT);
  text(c, 520, 18, Version, DIM);
  c.rect(0, 48, 640, 1, LINE);
  const char *tabs[]={"SEQUENCES","SONGS"};ui::tabs(c,56,tabs,2,a.atlasTab,true);c.rect(0,88,640,1,LINE);
  int cursor = songs ? a.songCursor : a.projectCursor,
      count = songs ? a.songCount : a.projectCount;
  int first = cursor / 5 * 5;
  for (int row = first; row <= count && row < first + 5; ++row) {
    int y = 116 + (row - first) * 56;
    if (row == cursor)
      c.box(18, y - 8, 604, 48, TEXT);
    if (row == count) {
      text(c, 30, y, songs ? "+ NEW SONG" : "+ NEW SEQUENCE", TEXT);
      text(c, 30, y + 23,
           songs ? "BUILD AN ORDERED LIST"
                 : "EMPTY OR COPY AN EXISTING SEQUENCE",
           DIM);
    } else {
      int id = songs ? a.songRows[row] : a.projectRows[row];
      text(c, 30, y, songs ? a.songNames[id] : a.projectNames[id], TEXT, 2, 40);
      bool broken = songs ? a.songBroken[id] : a.broken[id];
      if (broken)
        strcpy(b, "DAMAGED / FILE PRESERVED");
      else if (songs)
        snprintf(b, sizeof(b), "%d SLOTS / %d BPM", a.songSizes[id],
                 a.songTempos[id]);
      else
        snprintf(b, sizeof(b), "%d TRACKS / %d BPM", a.projectTracks[id],
                 a.projectTempos[id]);
      text(c, 30, y + 23, b, DIM);
    }
  }
  snprintf(b, sizeof(b), "%d / 32 %s / R1+L-R PAGE", count,
           songs ? "SONGS" : "SEQUENCES");
  text(c, 18, 423, b, DIM);
}
inline void drawSong(lab::Canvas &c, const App &a) {
  using namespace lab;
  char b[120];
  bool playing = a.songPlaying();
  snprintf(b, sizeof(b), "%s %.24s%s", playing ? ">" : "-", a.song.name,
           a.songDirty ? " +" : "");
  text(c, 18, 18, b, TEXT, 2, 32);
  snprintf(b, sizeof(b), "SEL %d BPM", a.song.bpm);
  text(c, 478, 18, b, DIM);
  if (a.buttons.held & bit(Select))
    c.box(468, 9, 154, 30, TEXT);
  c.rect(0, 48, 640, 1, LINE);
  text(c, 18, 65, "SONG / ARRANGEMENT", TEXT);
  snprintf(b, sizeof(b), "%d/64 SLOTS", a.song.count);
  text(c, 478, 65, b, DIM);
  c.rect(0, 91, 640, 1, LINE);
  text(c, 30, 107, "SLOT / SEQUENCE", DIM);
  text(c, 428, 107, "REPEAT", DIM);
  text(c, 536, 107, "BARS", DIM);
  int first = a.songRow / 4 * 4;
  unsigned playSlot =
      __atomic_load_n(&a.engine->shownSongSlot, __ATOMIC_ACQUIRE);
  unsigned repetition =
      __atomic_load_n(&a.engine->shownSongRepeat, __ATOMIC_ACQUIRE);
  for (int i = first; i <= a.song.count && i < first + 4; ++i) {
    int y = 141 + (i - first) * 43;
    if (i == a.songRow)
      c.box(18, y - 8, 604, 38, TEXT);
    if (i == a.song.count) {
      text(c, 30, y,
           a.song.count == SongSlots ? "64 SLOTS / FULL" : "+ ADD SEQUENCE",
           TEXT);
      continue;
    }
    const auto &s = a.song.slots[i];
    bool missing = !a.saved[s.sequence] || a.broken[s.sequence];
    snprintf(b, sizeof(b), "%s%02d %.24s",
             playing && playSlot == unsigned(i) ? ">" : " ", i + 1,
             missing ? "MISSING / X REPLACE" : a.projectNames[s.sequence]);
    text(c, 24, y, b, TEXT, 2, 31);
    snprintf(b, sizeof(b), "%d", s.repeats);
    text(c, 452, y, b, DIM);
    snprintf(b, sizeof(b), "%d", s.bars);
    text(c, 548, y, b, DIM);
  }
  if (playing)
    snprintf(b, sizeof(b), "PLAY %02u / REPEAT %u/%d / START STOP",
             playSlot + 1, repetition + 1, a.song.slots[playSlot].repeats);
  else
    strcpy(b, "A OPEN / X REPLACE / START PLAY FROM SLOT");
  text(c, 18, 321, b, DIM);
  View v;
  v.buttons = a.buttons;
  const char *labels[] = {"POSITION", "REPEATS", "BARS", "LOOP"};
  const char *units[] = {"MOVE SLOT", "TIMES", "PER REPEAT", "WHOLE SONG"};
  for (int i = 0; i < 4; ++i) {
    if (i == 3)
      strcpy(b, a.song.loop ? "ON" : "OFF");
    else if (a.songRow >= a.song.count)
      strcpy(b, "--");
    else
      snprintf(b, sizeof(b), "%d",
               i == 0   ? a.songRow + 1
               : i == 1 ? a.song.slots[a.songRow].repeats
                        : a.song.slots[a.songRow].bars);
    parameterField(c, v, i, labels[i], b, units[i]);
  }
}
inline void drawSequencePicker(lab::Canvas &c, const App &a) {
  if (!a.sequencePicker)
    return;
  using namespace lab;
  c.rect(28, 94, 584, 346, PANEL);
  c.box(28, 94, 584, 346, LINE);
  bool create = a.sequencePicker == 3;
  text(c, 44, 110, create ? "NEW SEQUENCE / START FROM" : "CHOOSE SEQUENCE",
       TEXT);
  int first = a.sequenceChoice / 6 * 6;
  for (int i = first; i <= a.projectCount && i < first + 6; ++i) {
    int y = 155 + (i - first) * 40;
    if (i == a.sequenceChoice)
      c.box(40, y - 8, 560, 33, TEXT);
    const char *label;
    if (create)
      label =
          i == 0 ? "+ EMPTY SEQUENCE" : a.projectNames[a.projectRows[i - 1]];
    else
      label = i == a.projectCount ? "+ NEW SEQUENCE"
                                  : a.projectNames[a.projectRows[i]];
    text(c, 52, y, label, i == a.sequenceChoice ? TEXT : DIM, 2, 44);
  }
  text(c, 44, 413,
       create ? "A CREATE COPY / B CANCEL"
              : "A SELECT / B CANCEL / R1+L-R PAGE",
       DIM);
}
