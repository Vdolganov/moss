// App member functions: contextual actions and their availability.
int rawOptionCount() const {
  if (page == SongPage)
    return 6;
  if (page == Atlas && atlasTab)
    return 2;
  if (importAssign)
    return 3;
  if (assignMode)
    return 2;
  if (manageMenu)
    return 3;
  if (page == Atlas)
    return 7;
  if (page == Grid)
    return 8;
  if (fxPage())
    return 4;
  if (page == Tape)
    return tapeTab == 0 ? 6 : tapeTab == 1 ? 8 : 6;
  return page == Sampler ? 7 : page == Pattern ? 9 : 5;
}
bool projectUses(const Project &p, int id) const {
  for (int t = 0; t < p.trackCount; ++t) {
    if (p.tracks[t].granular.asset == id) return true;
    for (const auto &k : p.tracks[t].keys)
      if (k.asset == id)
        return true;
  }
  return false;
}
bool assetProtected(int id) const {
  if (id < 0 || id >= Assets || projectUses(project, id) ||
      (haveUndo && projectUses(undoProject, id)))
    return true;
  for (uint32_t mask : savedAssetMasks)
    if (mask & (uint32_t(1) << id))
      return true;
  return false;
}
uint32_t assetMask(const Project &p) const {
  static_assert(Assets == 32, "Saved sample references use one 32-bit mask");
  uint32_t mask = 0;
  for (int t = 0; t < p.trackCount; ++t) {
    int grain = p.tracks[t].granular.asset;
    if (grain >= 0 && grain < Assets) mask |= uint32_t(1) << grain;
    for (const auto &k : p.tracks[t].keys)
      if (k.asset >= 0 && k.asset < Assets)
        mask |= uint32_t(1) << k.asset;
  }
  return mask;
}
bool currentLocksPresent() const {
  if (bank != 1 && bank != 2)
    return false;
  const auto &t = project.tracks[track];
  for (int i = 0; i < 4; ++i) {
    int value =
        bank == 2 ? t.steps[step].fxLocks[t.fx.type][clamp(fxBank,0,fxBanks(t.fx.type)-1)*4+i]
        : granularTrack() ? t.steps[step].grainLocks[synthBank * 4 + i]
        : t.instrument
            ? t.steps[step].synthLocks[t.instrument - 1][synthBank * 4 + i]
            : t.hits[key][step].locks[i];
    if (value != Inherit)
      return true;
  }
  return false;
}
bool optionVisible(int id) const {
  if (page == SongPage || (page == Atlas && atlasTab))
    return songOptionVisible(id);
  if (importAssign)
    return id == 0;
  if (assignMode)
    return false;
  if (manageMenu)
    return id != 1 || (manageMenu == 1 ? projectCount < Projects
                                       : project.trackCount < Tracks);
  if (page == Atlas) {
    if (id < 2)
      return false; // creation has a permanent entry on ATLAS
    if (id == 3 && projectCount == Projects)
      return false;
    if (id <= 4)
      return projectCursor < projectCount &&
             !broken[projectRows[projectCursor]];
    if (id == 5)
      return deletedProjectAvailable && projectCount < Projects;
    return true;
  }
  if (id == 0)
    return page != Tape;
  if (id == 1)
    return page != Tape && haveUndo && !captureBusy();
  if (id == 2)
    return page == Tape && tapeTab == 1 && sliceCount;
  if (page == Grid)
    return (id == 4 && track < project.trackCount) || id == 5;
  if (fxPage())
    return false; // toggle and order have direct controls
  if (page == Tape) {
    if (tapeTab == 0)
      return id == 5 && captureState() == 2 && !captureWritePending;
    if (tapeTab == 1)
      return sliceCount && ((id == 6 && sliceCount < 16 &&
                             slices[sliceIndex].end < durationMs()) ||
                            ((id == 4 || id == 7) && sliceCount > 1));
    return asset >= 0 && (id == 4 || (id == 5 && !assetProtected(asset)));
  }
  if (page == Sampler)
    return id == 3 || id == 4 ||
           (id == 5 && (granularTrack() ? project.tracks[track].granular.asset >= 0 : (project.tracks[track].instrument ||
                        project.tracks[track].keys[key].asset >= 0)));
  if (page == Pattern)
    return (id == 3 && bank == 0 &&
            (project.tracks[track].instrument ||
             project.tracks[track].keys[key].asset >= 0)) ||
           (id == 4 && currentLocksPresent()) ||
           (id == 6 && (selectedEvent(project.tracks[track],step,key)!=EmptyEvent || currentLocksPresent())) ||
           (id == 7 && selectedEvent(project.tracks[track],step,key)!=EmptyEvent) ||
           (id == 8 && (bank==1 || bank==2));
  return false;
}
int optionCount() const {
  int count = 0;
  for (int id = 0; id < rawOptionCount(); ++id)
    count += optionVisible(id);
  return count;
}
int optionId(int position) const {
  for (int id = 0; id < rawOptionCount(); ++id)
    if (optionVisible(id) && !position--)
      return id;
  return -1;
}
int optionIndex(int id) const {
  for (int i = 0; i < optionCount(); ++i)
    if (optionId(i) == id)
      return i;
  return -1;
}
const char *optionName(int i) const {
  i = optionId(i);
  if (i < 0)
    return "";
  if (page == SongPage || (page == Atlas && atlasTab))
    return songOptionName(i);
  if (importAssign) {
    const char *n[] = {"AUDITION FULL SOURCE", "NAME + ASSIGN SLICE",
                       "CANCEL IMPORT"};
    return n[i];
  }
  if (assignMode)
    return i == 0 ? "IMPORT FROM DEVICE" : "CANCEL ASSIGNMENT";
  if (manageMenu) {
    const char *n[] = {"RENAME", "DUPLICATE", "DELETE"};
    return n[i];
  }
  if (page == Atlas) {
    const char *n[] = {"NEW SEQUENCE",    "DEMO / SLOW SIGNAL",
                       "RENAME SEQUENCE", "DUPLICATE SEQUENCE",
                       "DELETE SEQUENCE", "RESTORE DELETED SEQUENCE",
                       "EXIT MOSS"};
    return n[i];
  }
  if (page == Grid && i >= 3) {
    const char *n[] = {"ADD TRACK", "TRACK OPTIONS", "SEQUENCE OPTIONS",
                       "BACK TO SEQUENCES", "EXIT MOSS"};
    return n[i - 3];
  }
  if (i == 0)
    return "SAVE SEQUENCE";
  if (i == 1)
    return "UNDO LAST EDIT";
  if (i == 2)
    return page == Tape && tapeTab == 1 ? "AUDITION FULL SOURCE"
                                        : "RESTART TRANSPORT";
  if (fxPage())
    return "ENABLE / BYPASS SELECTED";
  if (page == Tape) {
    const char *n[3][6] = {
        {"IMPORT WAV", "FROM LIBRARY", "DISCARD CAPTURE"},
        {"SAVE SELECTED SLICE", "SAVE ALL SLICES", "CUT AT MIDPOINT",
         "ADD NEXT RANGE", "REMOVE SELECTED SLICE", "AUDITION FULL SOURCE"},
        {"EDIT COPY", "RENAME SAMPLE", "DELETE UNUSED SAMPLE"}};
    return n[tapeTab][i - 3];
  }
  if (page == Sampler) {
    if (i == 3)
      return "CHOOSE INSTRUMENT";
    if (project.tracks[track].instrument && !granularTrack()) {
      const char *n[] = {"CHOOSE PRESET", "CHANGE MODE", "EXIT MOSS"};
      return n[i - 4];
    }
    const char *n[] = {"ASSIGN / REPLACE SAMPLE", "REMOVE ASSIGNMENT",
                       "ONE SHOT PLAYBACK"};
    return n[i - 4];
  }
  if (page == Pattern) {
    const char *n[] = {"LENGTH TO CYCLE END", "CLEAR CURRENT LOCK BANK",
                       "CYCLE 8 / 16 / 32 / 64", "CLEAR SELECTED STEP",
                       "", "RESET CURRENT LOCK BANK"};
    if(i==7) return selectedEvent(project.tracks[track],step,key)==NoteEvent?"REMOVE NOTE / KEEP LOCKS":"ADD NOTE / KEEP LOCKS";
    return n[i - 3];
  }
  return i == 3 ? "BACK TO SEQUENCES" : "EXIT MOSS";
}
void chooseOption() {
  int i = optionId(option);
  options = false;
  if (i < 0)
    return;
  if (page == SongPage || (page == Atlas && atlasTab)) {
    editing = false;
    chooseSongOption(i);
    return;
  }
  if (importAssign) {
    if (i == 0 && sliceCount)
      previewToggle(PreviewSource);
    if (i == 1)
      nameSlice();
    if (i == 2)
      action(lab::B);
    return;
  }
  if (assignMode) {
    if (i == 0) {
      asset = -1;
      tapeAction(lab::A);
    } else
      action(lab::B);
    return;
  }
  editing = false;
  if (manageMenu) {
    int target = manageMenu;
    manageMenu = 0;
    if (!structureAllowed())
      return;
    if (i == 0)
      beginName(target,
                target == 1 ? project.name : project.tracks[track].name);
    if (i == 1) {
      if (target == 1)
        duplicateProject();
      else
        duplicateTrack();
    }
    if (i == 2 && structureAllowed()) {
      confirmDelete = true;
      confirmTarget = target;
    }
    return;
  }
  if (page == Atlas) {
    if (i == 0)
      beginSequenceCreate();
    if (i == 1)
      createProject("SLOW SIGNAL", true);
    if (i >= 2 && i <= 4 && selectedProject()) {
      if (i == 2)
        beginName(1, project.name);
      if (i == 3)
        duplicateProject();
      if (i == 4) {
        confirmDelete = true;
        confirmTarget = 1;
      }
    }
    if (i == 5)
      restoreProject();
    if (i == 6)
      requestExit();
    return;
  }
  if (i == 0) {
    save();
    return;
  }
  if (i == 1) {
    if (captureBusy()) {
      say("FINISH CAPTURE FIRST");
      return;
    }
    if (haveUndo) {
      Project temp = project;
      project = undoProject;
      undoProject = temp;
      dirty = true;
      reload = true;
      normalizeSelection();
      say("UNDO / TRANSPORT STOPPED");
    } else
      say("NOTHING TO UNDO");
    return;
  }
  if (i == 2) {
    if (page == Tape && tapeTab == 1) {
      if (sliceCount)
        previewToggle(PreviewSource);
      else
        say("IMPORT A SOURCE FIRST");
      return;
    }
    send({Op::Rewind});
    return;
  }
  if (page == Grid) {
    if (i == 3 && structureAllowed()) {
      if (project.trackCount == Tracks)
        say("4 TRACKS MAXIMUM");
      else {
        voicePicker = 3;
        voiceChoice = OrganismInstrument;
      }
    }
    if (i == 4) {
      if (track < project.trackCount) {
        manageMenu = 2;
        options = true;
        option = 0;
      } else
        say("SELECT A TRACK FIRST");
    }
    if (i == 5) {
      manageMenu = 1;
      options = true;
      option = 0;
    }
    if (i == 6)
      backToAtlas();
    if (i == 7)
      requestExit();
    return;
  }
  if (fxPage()) {
    auto &f = page == Mix ? project.fx : project.tracks[track].fx;
    change({Op::Fx, page == Mix ? Tracks : track, 0, 1, !f.enabled});
  } else if (page == Tape) {
    if (captureBusy()) {
      if (tapeTab == 0 && i == 5) {
        discardPending = send({Op::CaptureDiscard});
        capturePending = false;
        captureFailed = false;
        say("CAPTURE DISCARDED");
      } else
        say("FINISH CAPTURE FIRST");
      return;
    }
    if (tapeTab == 0) {
      if (i == 3)
        tapeAction(lab::X);
      if (i == 4) {
        nameTarget = 0;
        tapeTab = 2;
        assignMode = false;
      }
      return;
    }
    if (tapeTab == 1 && sliceCount) {
      auto cut = slices[sliceIndex];
      if (i == 3)
        nameSlice();
      if (i == 4)
        nameSlice(true);
      if (i == 5 || i == 6) {
        if (sliceCount == 16) {
          say("16 SLICES MAXIMUM");
          return;
        }
        int at = i == 5 ? (cut.start + cut.end) / 2 : cut.end;
        if ((i == 5 && cut.end - cut.start < 2) ||
            (i == 6 && at >= durationMs())) {
          say("NO SOURCE LEFT FOR ANOTHER SLICE");
          return;
        }
        for (int j = sliceCount; j > sliceIndex + 1; j--)
          slices[j] = slices[j - 1];
        slices[sliceIndex + 1] = {at, i == 5 ? cut.end : durationMs(), 0, 0};
        limitSlice(slices[sliceIndex + 1], 0);
        if (i == 5) {
          slices[sliceIndex].end = at;
          slices[sliceIndex].fadeIn = slices[sliceIndex].fadeOut = 0;
        }
        sliceCount++;
        sliceIndex++;
      }
      if (i == 7) {
        if (sliceCount == 1) {
          say("KEEP ONE SLICE / EDIT ITS BOUNDS");
          return;
        }
        for (int j = sliceIndex; j < sliceCount - 1; j++)
          slices[j] = slices[j + 1];
        sliceCount--;
        sliceIndex = clamp(sliceIndex, 0, sliceCount - 1);
      }
    } else if (tapeTab == 2 && library && asset >= 0 &&
               library->pcm[asset].frames) {
      if (i == 3) {
        char p[128];
        library->assetPath(p, sizeof(p), asset);
        openSource(p);
      }
      if (i == 4) {
        beginName(0, library->names[asset]);
      }
      if (i == 5) {
        confirmTarget = 0;
        if (assetUsed(asset))
          say("IN USE BY SEQUENCE OR UNDO / FILE KEPT");
        else
          confirmDelete = true;
      }
    }
  } else if (page == Sampler) {
    auto &t = project.tracks[track];
    if (i == 3) {
      voicePicker = 1;
      voiceChoice = t.instrument;
      return;
    }
    if (granularTrack()) {
      if (i == 4) pickAsset();
      if (i == 5) assignSample(-1);
      return;
    }
    if (t.instrument) {
      if (i == 4) {
        voicePicker = 2;
        voiceChoice = t.synthPreset[t.instrument - 1] < 0
                          ? 0
                          : t.synthPreset[t.instrument - 1];
      }
      if (i == 5)
        change({Op::SynthMode, track, t.instrument - 1, 0,
                1 - t.synthMode[t.instrument - 1]});
      if (i == 6)
        requestExit();
      return;
    }
    if (i == 4)
      pickAsset();
    if (i == 5)
      change({Op::Key, track, key, 0, -1});
    if (i == 6)
      say("ONE SHOT / SAMPLE PLAYS TO ITS END");
  } else if (page == Pattern) {
    auto &t = project.tracks[track];
    if (i == 3 && t.instrument) {
      change({Op::Step, track, step, 0, 1});
      change({Op::Step, track, step, 3, t.cycle - step});
    }
    if (i == 3 && !t.instrument) {
      change({Op::Hit, track, step, hitField(key, HitOn), 1});
      change({Op::Hit, track, step, hitField(key, HitLength), t.cycle - step});
    }
    if (i == 4 || i==8)
      for (int j = 0; j < 4; j++)
        change(
            bank == 2
                ? Command{Op::FxLock, track, step, t.fx.type * EffectParameters + clamp(fxBank,0,fxBanks(t.fx.type)-1)*4 + j, i==8?ResetLock:Inherit}
            : granularTrack()
                ? Command{Op::GranularLock, track, step, synthBank * 4 + j, i==8?ResetLock:Inherit}
            : t.instrument
                ? Command{Op::SynthLock, track, step,
                          (t.instrument - 1) * 8 + synthBank * 4 + j, i==8?ResetLock:Inherit}
                : Command{Op::Hit, track, step, hitField(key, HitLock0 + j),
                          i==8?ResetLock:Inherit});
    if (i == 5) {
      int cells = t.cells();
      int count = cells < 8    ? 8
                  : cells < 16 ? 16
                  : cells < 32 ? 32
                  : cells < 64 ? 64
                               : 8;
      change({Op::Track, track, 0, 0, count * t.stride()});
      step = clamp(step, 0, t.cycle - 1);
    }
    if (i == 6)
      change({Op::ClearEvent,track,step,key,0});
    if(i==7) change({Op::SetEventType,track,step,key,selectedEvent(t,step,key)==NoteEvent?LockEvent:NoteEvent});
  } else if (i == 4)
    requestExit();
  else
    backToAtlas();
  editing = false;
}
