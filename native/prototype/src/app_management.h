// Member functions included inside App. Structural edits are applied by service
// with audio paused; the callback never reads the UI's mutable project.
void beginName(int target, const char *value) {
  nameTarget = target;
  naming = true;
  status[0] = 0;
  snprintf(nameEdit, sizeof(nameEdit), "%.24s", value);
  nameCursor = int(strlen(nameEdit));
  nameKey = 0;
  nameSelected = nameCursor > 0;
}
bool structureAllowed() {
  if (captureBusy()) {
    say("FINISH CAPTURE FIRST");
    return false;
  }
  return true;
}
void checkpoint() {
  undoProject = project;
  haveUndo = true;
  dirty = true;
  reload = true;
  editing = false;
}
void normalizeSelection() {
  track = clamp(track, 0, project.trackCount ? project.trackCount - 1 : 0);
  step = clamp(step, 0, project.tracks[track].cycle - 1);
  captureSource = 0;
  if (!project.trackCount && page >= Pattern && page != SongPage)
    page = Grid;
  voicePicker = 0;
  assignMode = importAssign = false;
  fxDetail = false;
  synthBank = 0;
}
int freeProjectSlot() {
  scan();
  for (int i = 0; i < Projects; i++)
    if (!saved[i] && !sequenceInSong(i))
      return i;
  return -1;
}
bool selectedProject() {
  if (page != Atlas)
    return true;
  if (projectCursor >= projectCount) {
    say("SELECT A SAVED SEQUENCE");
    return false;
  }
  int id = projectRows[projectCursor];
  char file[128];
  path(file, sizeof(file), id);
  Project p;
  if (!loadProject(file, p)) {
    say("DAMAGED SEQUENCE / FILE KEPT");
    return false;
  }
  project = p;
  slot = id;
  dirty = false;
  haveUndo = false;
  reload = true;
  normalizeSelection();
  return true;
}
void createProject(const char *title, bool demo = false) {
  if (!structureAllowed())
    return;
  if (page != Atlas && dirty && !save())
    return;
  int id = freeProjectSlot();
  if (id < 0) {
    say("32 SEQUENCES / DELETE ONE FIRST");
    return;
  }
  Project next = demo ? makeSynthDemo() : Project{};
  if (!demo && copySource >= 0) {
    char sourceFile[128];
    path(sourceFile, sizeof(sourceFile), copySource);
    if (!loadProject(sourceFile, next)) {
      say("COPY SOURCE MISSING OR DAMAGED");
      return;
    }
  }
  if (!demo && copySource < 0) {
    next.trackCount = 0;
    next.bpm = 120;
  }
  snprintf(next.name, sizeof(next.name), "%s", title);
  char file[128];
  path(file, sizeof(file), id);
  if (!saveProject(file, next)) {
    say("CREATE FAILED / CHECK SD");
    return;
  }
  naming = false;
  copySource = -1;
  int songInsert = pendingSongInsert;
  project = next;
  slot = id;
  dirty = false;
  haveUndo = false;
  page = Grid;
  track = step = key = bank = 0;
  reload = true;
  normalizeSelection();
  scan();
  if (songInsert) {
    insertSequence(id, songInsert == 2);
    inSongSequence = true;
    saveArrangement();
  }
  say(demo ? "SLOW SIGNAL READY / START PLAY"
           : "SEQUENCE READY / A OPEN OR ADD TRACK");
}
void addTrack(int instrument) {
  if (!structureAllowed())
    return;
  if (project.trackCount == Tracks) {
    say("4 TRACKS MAXIMUM");
    return;
  }
  checkpoint();
  track = project.trackCount++;
  project.tracks[track] = Track{};
  auto &t = project.tracks[track];
  t.instrument = instrument;
  snprintf(t.name, sizeof(t.name), "%s", instrumentName(instrument));
  step = key = bank = 0;
  page = Pattern;
  normalizeSelection();
  say("TRACK ADDED / A ENABLE STEP");
}
void duplicateTrack() {
  if (!structureAllowed() || !project.trackCount)
    return;
  if (project.trackCount == Tracks) {
    say("4 TRACKS MAXIMUM");
    return;
  }
  checkpoint();
  for (int i = project.trackCount; i > track + 1; i--)
    project.tracks[i] = project.tracks[i - 1];
  project.tracks[track + 1] = project.tracks[track];
  track++;
  project.trackCount++;
  char old[25];
  snprintf(old, sizeof(old), "%.19s", project.tracks[track].name);
  snprintf(project.tracks[track].name, 25, "%s COPY", old);
  normalizeSelection();
  say("TRACK DUPLICATED / UNDO AVAILABLE");
}
void removeTrack() {
  if (!structureAllowed() || !project.trackCount)
    return;
  checkpoint();
  for (int i = track; i < project.trackCount - 1; i++)
    project.tracks[i] = project.tracks[i + 1];
  project.tracks[--project.trackCount] = Track{};
  page = Grid;
  normalizeSelection();
  say("TRACK DELETED / OPTIONS UNDO");
}
void duplicateProject() {
  if (!structureAllowed() || !selectedProject())
    return;
  if (dirty && !save())
    return;
  Project copy = project;
  char old[25];
  snprintf(old, sizeof(old), "%.19s", copy.name);
  snprintf(copy.name, 25, "%s COPY", old);
  int id = freeProjectSlot();
  if (id < 0) {
    say("32 SEQUENCES / DELETE ONE FIRST");
    return;
  }
  char file[128];
  path(file, sizeof(file), id);
  if (!saveProject(file, copy)) {
    say("COPY FAILED / CHECK SD");
    return;
  }
  project = copy;
  slot = id;
  dirty = false;
  haveUndo = false;
  reload = true;
  page = Grid;
  normalizeSelection();
  scan();
  say("SEQUENCE COPY OPENED");
}
void removeProject() {
  if (!structureAllowed())
    return;
  if (sequenceInSong(slot)) {
    say("USED BY SONG / REMOVE ITS SLOTS FIRST");
    return;
  }
  // Keep one recoverable deleted project. Its sample references remain
  // protected.
  if (!saveProject("projects/deleted.moss", project)) {
    say("DELETE BACKUP FAILED / FILE KEPT");
    return;
  }
  char file[128];
  path(file, sizeof(file), slot);
  if (remove(file)) {
    say("DELETE FAILED / CHECK SD");
    return;
  }
  project = Project{};
  project.trackCount = 0;
  slot = -1;
  dirty = false;
  haveUndo = false;
  reload = true;
  page = Atlas;
  projectCursor = 0;
  normalizeSelection();
  scan();
  say("SEQUENCE DELETED / OPTIONS RESTORE");
}
void restoreProject() {
  if (!structureAllowed())
    return;
  Project restored;
  if (!loadProject("projects/deleted.moss", restored)) {
    say("NO DELETED SEQUENCE TO RESTORE");
    return;
  }
  int id = freeProjectSlot();
  if (id < 0) {
    say("32 SEQUENCES / DELETE ONE FIRST");
    return;
  }
  char file[128];
  path(file, sizeof(file), id);
  if (!saveProject(file, restored)) {
    say("RESTORE FAILED / CHECK SD");
    return;
  }
  remove("projects/deleted.moss");
  scan();
  say("SEQUENCE RESTORED IN ATLAS");
}

void insertNameCharacter(char value) {
  if (nameSelected) {
    nameEdit[0] = 0;
    nameCursor = 0;
    nameSelected = false;
  }
  int length = int(strlen(nameEdit));
  nameCursor = clamp(nameCursor, 0, length);
  if (length >= NameLimit) {
    say("24 CHARACTERS MAX / X DELETE");
    return;
  }
  memmove(nameEdit + nameCursor + 1, nameEdit + nameCursor,
          length - nameCursor + 1);
  nameEdit[nameCursor++] = value;
}
void deleteNameCharacter() {
  if (nameSelected) {
    nameEdit[0] = 0;
    nameCursor = 0;
    nameSelected = false;
    return;
  }
  int length = int(strlen(nameEdit));
  nameCursor = clamp(nameCursor, 0, length);
  if (!nameCursor)
    return;
  memmove(nameEdit + nameCursor - 1, nameEdit + nameCursor,
          length - nameCursor + 1);
  --nameCursor;
}
void commitName() {
  int n = int(strlen(nameEdit));
  while (n > 0 && nameEdit[n - 1] == ' ')
    n--;
  nameEdit[n] = 0;
  if (!n) {
    say("NAME CANNOT BE EMPTY");
    return;
  }
  if (nameTarget == 6) {
    createArrangement(nameEdit);
    return;
  }
  if (nameTarget == 7) {
    Song before = song;
    bool wasDirty = songDirty;
    snprintf(song.name, 25, "%s", nameEdit);
    songDirty = true;
    if (!saveArrangement()) {
      song = before;
      songDirty = wasDirty;
      return;
    }
    naming = false;
    say("SONG RENAMED");
    return;
  }
  if(nameTarget==8){if(renameSelectedKit(nameEdit))naming=false;return;}
  if (nameTarget == 4 || nameTarget == 5) {
    snprintf(saveLabel, sizeof(saveLabel), "%s", nameEdit);
    naming = false;
    job = nameTarget == 5 ? SaveAll : SaveSlice;
    return;
  }
  if (nameTarget) {
    if (nameTarget == 3) {
      createProject(nameEdit);
      return;
    }
    if (nameTarget == 1) {
      char before[25];
      memcpy(before, project.name, 25);
      bool wasDirty = dirty;
      snprintf(project.name, 25, "%s", nameEdit);
      dirty = true;
      if (!save()) {
        memcpy(project.name, before, 25);
        dirty = wasDirty;
        return;
      }
      scan();
    } else {
      checkpoint();
      snprintf(project.tracks[track].name, 25, "%s", nameEdit);
    }
    naming = false;
    say("NAME SAVED");
    return;
  }
  char old[32];
  memcpy(old, library->names[asset], 32);
  snprintf(library->names[asset], 32, "%s", nameEdit);
  if (!library->saveName(asset)) {
    memcpy(library->names[asset], old, 32);
    say("NAME SAVE FAILED");
    return;
  }
  naming = false;
  say("SAMPLE RENAMED");
}
