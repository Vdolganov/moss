// Arrangement and sequence-library workflows; UI thread only.
Song song, undoSong;
SongBank *songPlayback = nullptr;
int atlasTab = 0, songId = -1, songCursor = 0, songCount = 0, songRow = 0;
int songRows[Songs] = {};
bool songSaved[Songs] = {}, songBroken[Songs] = {};
char songNames[Songs][25] = {};
int songSizes[Songs] = {}, songTempos[Songs] = {};
bool songDirty = false, haveSongUndo = false, inSongSequence = false;
int sequencePicker = 0, sequenceChoice = 0, copySource = -1,
    pendingSongInsert = 0;
bool songPlaying() const {
  return page == SongPage &&
         __atomic_load_n(&engine->shownPlaying, __ATOMIC_ACQUIRE);
}
void scanSongs() {
  songCount = 0;
  for (int i = 0; i < Songs; ++i) {
    char file[128];
    songPath(file, sizeof(file), i);
    songSaved[i] = exists(file);
    Song value;
    songBroken[i] = songSaved[i] && !loadSong(file, value);
    if (!songSaved[i])
      continue;
    songRows[songCount++] = i;
    snprintf(songNames[i], 25, "%s",
             songBroken[i] ? "DAMAGED SONG" : value.name);
    songSizes[i] = songBroken[i] ? 0 : value.count;
    songTempos[i] = songBroken[i] ? 0 : value.bpm;
  }
  songCursor = clamp(songCursor, 0, songCount);
}
bool saveArrangement() {
  if (songId < 0)
    return true;
  char file[128];
  songPath(file, sizeof(file), songId);
  if (!saveSong(file, song)) {
    say("SONG SAVE FAILED / CHECK SD");
    return false;
  }
  songDirty = false;
  scanSongs();
  return true;
}
void songCheckpoint() {
  if (!editing) {
    undoSong = song;
    haveSongUndo = true;
    editing = true;
  }
  songDirty = true;
}
bool songEditable() {
  if (songPlaying()) {
    say("START STOP / THEN EDIT SONG");
    return false;
  }
  return structureAllowed();
}
void openArrangement(int id) {
  if (!structureAllowed() || (dirty && !save()) ||
      (songDirty && !saveArrangement()))
    return;
  char file[128];
  songPath(file, sizeof(file), id);
  Song next;
  if (!loadSong(file, next)) {
    say("SONG DAMAGED / FILE KEPT");
    return;
  }
  sendStop();
  song = next;
  songId = id;
  songRow = 0;
  songDirty = haveSongUndo = false;
  inSongSequence = false;
  page = SongPage;
  scan();
  say("A OPEN / X REPLACE / START PLAY");
}
void createArrangement(const char *title) {
  scanSongs();
  int id = 0;
  while (id < Songs && songSaved[id])
    ++id;
  if (id == Songs) {
    say("32 SONGS / DELETE ONE FIRST");
    return;
  }
  Song next;
  snprintf(next.name, 25, "%s", title);
  char file[128];
  songPath(file, sizeof(file), id);
  if (!saveSong(file, next)) {
    say("CREATE FAILED / CHECK SD");
    return;
  }
  song = next;
  songId = id;
  songRow = 0;
  songDirty = haveSongUndo = false;
  naming = false;
  page = SongPage;
  inSongSequence = false;
  scanSongs();
  scan();
  say("EMPTY SONG / A ADD SEQUENCE");
}
bool sequenceInSong(int id) {
  auto uses = [&](const Song &s) {
    for (int i = 0; i < s.count; ++i)
      if (s.slots[i].sequence == id)
        return true;
    return false;
  };
  if (songId >= 0 && (uses(song) || (haveSongUndo && uses(undoSong))))
    return true;
  for (int i = 0; i < Songs; ++i) {
    char file[128];
    songPath(file, sizeof(file), i);
    if (!exists(file))
      continue;
    Song value;
    if (!loadSong(file, value) || uses(value))
      return true;
  }
  return false;
}
void beginSequenceCreate() {
  if (!structureAllowed())
    return;
  scan();
  sequencePicker = 3;
  sequenceChoice = 0;
  copySource = -1;
}
void insertSequence(int id, bool replace) {
  Project value;
  char file[128];
  path(file, sizeof(file), id);
  if (!loadProject(file, value)) {
    say("SEQUENCE MISSING OR DAMAGED");
    return;
  }
  if (!replace && song.count == SongSlots) {
    say("64 SLOTS / REMOVE ONE FIRST");
    return;
  }
  editing = false;
  songCheckpoint();
  if (!replace)
    songRow = song.count++;
  song.slots[songRow] = {id, 1, defaultBars(value)};
  editing = false;
  sequencePicker = 0;
  pendingSongInsert = 0;
  scan();
  say("SLOT ADDED / HOLD B REPEATS / HOLD A BARS");
}
void chooseSequence() {
  if (sequencePicker == 3) {
    copySource = sequenceChoice ? projectRows[sequenceChoice - 1] : -1;
    if (copySource >= 0 && broken[copySource]) {
      say("DAMAGED SEQUENCE / FILE KEPT");
      return;
    }
    char title[25];
    snprintf(title, 25, copySource < 0 ? "%s" : "%.19s COPY",
             copySource < 0 ? "UNTITLED" : projectNames[copySource]);
    sequencePicker = 0;
    beginName(3, title);
    return;
  }
  if (sequenceChoice == projectCount) {
    pendingSongInsert = sequencePicker;
    beginSequenceCreate();
    return;
  }
  insertSequence(projectRows[sequenceChoice], sequencePicker == 2);
}
void prepareSong() {
  // Called by the platform with audio paused. The previous bank is not read.
  engine->init(project, engine->samples, engine->rate, engine->sampleCount);
  if (!song.count) {
    say("ADD A SEQUENCE FIRST");
    return;
  }
  if (!songPlayback) {
    void *memory = malloc(sizeof(SongBank));
    if (memory)
      songPlayback = new (memory) SongBank;
  }
  if (!songPlayback) {
    say("NOT ENOUGH MEMORY FOR SONG");
    return;
  }
  auto &b = *songPlayback;
  b.song = song;
  b.count = 0;
  for (int &m : b.map)
    m = -1;
  for (int i = 0; i < song.count; ++i) {
    int id = song.slots[i].sequence;
    if (b.map[id] >= 0)
      continue;
    char file[128];
    path(file, sizeof(file), id);
    auto &p = b.sequences[b.count];
    if (!loadProject(file, p)) {
      say("SEQUENCE MISSING OR DAMAGED / X REPLACE");
      return;
    }
    b.map[id] = b.count++;
  }
  if(library) {
    AssetMask needed;
    for(int i=0;i<b.count;++i) {
      AssetMask set=projectAssets(b.sequences[i]);
      for(int id=0;id<CatalogAssets;++id)if(set.has(id))needed.add(id);
    }
    if(!library->prepare(needed)){say(library->error);return;}
    engine->init(project,library->views,engine->rate,CatalogAssets);
  }
  engine->startSong(b, clamp(songRow, 0, song.count - 1));
  say("SONG PLAYING / START STOP");
}
void editSong(int i, int d, int amount) {
  if (!songEditable())
    return;
  if (i != 3 && songRow >= song.count)
    return;
  int delta=parameterDelta(amount);
  if((i==0&&clamp(songRow+((buttons.held&(lab::bit(lab::Up)|lab::bit(lab::Down)))?-d:d),0,song.count-1)==songRow)||
     (i==1&&clamp(song.slots[songRow].repeats+d*delta,1,64)==song.slots[songRow].repeats)||
     (i==2&&clamp(song.slots[songRow].bars+d*parameterDelta(amount,4),1,64)==song.slots[songRow].bars)||
     (i==3&&song.loop==(d>0)))return;
  songCheckpoint();
  if (i == 0) {
    int direction =
        buttons.held & (lab::bit(lab::Up) | lab::bit(lab::Down)) ? -d : d;
    int target = clamp(songRow + direction, 0, song.count - 1);
    SongSlot held = song.slots[songRow];
    song.slots[songRow] = song.slots[target];
    song.slots[target] = held;
    songRow = target;
  } else if (i == 1)
    song.slots[songRow].repeats =
        clamp(song.slots[songRow].repeats + d * parameterDelta(amount), 1, 64);
  else if (i == 2)
    song.slots[songRow].bars =
        clamp(song.slots[songRow].bars + d * parameterDelta(amount, 4), 1, 64);
  else
    song.loop = d > 0;
}
void songAction(int b) {
  if (b == lab::Start) {
    if (songPlaying())
      sendStop();
    else if (structureAllowed() && (!songDirty || saveArrangement()))
      job = StartSong;
    return;
  }
  if (b == lab::B || b == lab::Select) {
    backToAtlas();
    return;
  }
  if (!songEditable())
    return;
  if (b == lab::A && songRow < song.count) {
    if (!saveArrangement() || (dirty && !save()))
      return;
    inSongSequence = true;
    requestedSlot = song.slots[songRow].sequence;
    return;
  }
  if ((b == lab::A && songRow == song.count) ||
      (b == lab::X && songRow < song.count)) {
    if (b == lab::A && song.count == SongSlots) {
      say("64 SLOTS / FULL");
      return;
    }
    scan();
    sequencePicker = b == lab::X ? 2 : 1;
    sequenceChoice = 0;
  }
}
bool songOptionVisible(int id) const {
  if (page == Atlas){
    if(id==3)return exists("projects/deleted-song.moss")&&songCount<Songs;
    return songCursor<songCount&&!songBroken[songRows[songCursor]]&&(id!=2||songCount<Songs);
  }
  if (songPlaying())
    return id == 0;
  if (id == 1)
    return haveSongUndo;
  if (id >= 3)
    return songRow < song.count && (id != 3 || song.count < SongSlots);
  return true;
}
const char *songOptionName(int id) const {
  if (page == Atlas) {
    const char *labels[] = {"RENAME SONG", "DELETE SONG", "DUPLICATE SONG", "RESTORE DELETED SONG"};
    return labels[id];
  }
  const char *labels[] = {"SAVE SONG",   "UNDO SONG EDIT",
                          "RENAME SONG", "DUPLICATE SLOT",
                          "REMOVE SLOT", "COPY SEQUENCE AS NEW"};
  return labels[id];
}
void chooseSongOption(int id) {
  if(page==Atlas&&(id==2||id==3)){
    Song next;char path[128];if(id==2)songPath(path,sizeof(path),songRows[songCursor]);else strcpy(path,"projects/deleted-song.moss");
    if(!loadSong(path,next)){say("SONG UNAVAILABLE / FILE KEPT");return;}
    if(id==2){char name[25];snprintf(name,25,"%.19s COPY",next.name);strcpy(next.name,name);}
    int free=-1;for(int i=0;i<Songs;++i)if(!songSaved[i]){free=i;break;}if(free<0)return;songPath(path,sizeof(path),free);
    if(!saveSong(path,next)){say("SONG NOT SAVED / CHECK CARD");return;}
    scanSongs();for(int i=0;i<songCount;++i)if(songRows[i]==free)songCursor=i;say(id==2?"SONG DUPLICATED":"SONG RESTORED");return;
  }
  if (page == Atlas) {
    openArrangement(songRows[songCursor]);
    if (page != SongPage)
      return;
    if (id == 0)
      beginName(7, song.name);
    else {
      confirmDelete = true;
      confirmTarget = 3;
    }
    return;
  }
  if (id == 0) {
    saveArrangement();
    return;
  }
  if (!songEditable())
    return;
  if (id == 1) {
    Song tmp = song;
    song = undoSong;
    undoSong = tmp;
    songDirty = true;
    songRow = clamp(songRow, 0, song.count);
    return;
  }
  if (id == 2) {
    beginName(7, song.name);
    return;
  }
  if (id == 5) {
    copySource = song.slots[songRow].sequence;
    pendingSongInsert = 2;
    char title[25];
    snprintf(title, 25, "%.19s COPY", projectNames[copySource]);
    beginName(3, title);
    return;
  }
  songCheckpoint();
  if (id == 3) {
    for (int i = song.count; i > songRow + 1; --i)
      song.slots[i] = song.slots[i - 1];
    song.slots[songRow + 1] = song.slots[songRow];
    ++song.count;
    ++songRow;
  } else {
    for (int i = songRow; i < song.count - 1; ++i)
      song.slots[i] = song.slots[i + 1];
    --song.count;
    songRow = clamp(songRow, 0, song.count);
  }
  editing = false;
}
void removeArrangement() {
  char file[128];
  songPath(file, sizeof(file), songId);
  if (!saveSong("projects/deleted-song.moss",song) || remove(file)) {
    say("DELETE FAILED / FILE KEPT");
    return;
  }
  songId = -1;
  songDirty = haveSongUndo = false;
  inSongSequence = false;
  page = Atlas;
  atlasTab = 1;
  scanSongs();
  say("SONG DELETED / SEQUENCES KEPT");
}
