// Member functions included inside App. Structural edits are applied by service
// with audio paused; the callback never reads the UI's mutable project.
void beginName(int target, const char *value) {
  nameTarget = target;
  naming = true;
  nameCursor = 0;
  memset(nameEdit, ' ', 24);
  nameEdit[24] = 0;
  size_t n = strlen(value);
  if (n > 24)
    n = 24;
  memcpy(nameEdit, value, n);
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
  if (!project.trackCount && page >= Pattern)
    page = Grid;
  voicePicker = 0;
  assignMode = importAssign = false;
  fxDetail = false;
  synthBank = 0;
}
int freeProjectSlot() {
  scan();
  for (int i = 0; i < Projects; i++)
    if (!saved[i])
      return i;
  return -1;
}
bool selectedProject() {
  if (page != Atlas)
    return true;
  if (projectCursor >= projectCount) {
    say("SELECT A SAVED PROJECT");
    return false;
  }
  int id = projectRows[projectCursor];
  char file[128];
  path(file, sizeof(file), id);
  Project p;
  if (!loadProject(file, p)) {
    say("DAMAGED PROJECT / FILE KEPT");
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
    say("32 PROJECTS / DELETE ONE FIRST");
    return;
  }
  Project next = demo ? makeSynthDemo() : Project{};
  if (!demo) {
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
  project = next;
  slot = id;
  dirty = false;
  haveUndo = false;
  page = Grid;
  track = step = key = bank = 0;
  reload = true;
  normalizeSelection();
  scan();
  say(demo ? "SLOW SIGNAL READY / START PLAY" : "EMPTY PROJECT / A ADD TRACK");
}
void addTrack(int instrument) {
  if (!structureAllowed())
    return;
  if (project.trackCount == Tracks) {
    say("8 TRACKS MAXIMUM");
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
    say("8 TRACKS MAXIMUM");
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
    say("32 PROJECTS / DELETE ONE FIRST");
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
  say("PROJECT COPY OPENED");
}
void removeProject() {
  if (!structureAllowed())
    return;
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
  say("PROJECT DELETED / OPTIONS RESTORE");
}
void restoreProject() {
  if (!structureAllowed())
    return;
  Project restored;
  if (!loadProject("projects/deleted.moss", restored)) {
    say("NO DELETED PROJECT TO RESTORE");
    return;
  }
  int id = freeProjectSlot();
  if (id < 0) {
    say("32 PROJECTS / DELETE ONE FIRST");
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
  say("PROJECT RESTORED IN ATLAS");
}
