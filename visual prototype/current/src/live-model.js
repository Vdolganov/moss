// Silent design study. Free-time events are separate from the native MS18 format.
export const BUTTONS = ['LEFT', 'DOWN', 'RIGHT', 'UP', 'Y', 'B', 'A', 'X'];
export const SCALES = {
  MINOR: [0, 2, 3, 5, 7, 8, 10, 12],
  MAJOR: [0, 2, 4, 5, 7, 9, 11, 12],
  DORIAN: [0, 2, 3, 5, 7, 9, 10, 12],
  PENTATONIC: [0, 3, 5, 7, 10, 12],
};
export const NAMES = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B'];
export const noteName = n => `${NAMES[(n % 12 + 12) % 12]}${Math.floor(n / 12) - 1}`;
export const mod = (n, d) => ((n % d) + d) % d;
export const MAX_EVENTS = 512;
const copy = value => structuredClone(value);

export function createLiveState() {
  return {
    schema: 'moss-live-study-1', name: 'AFTER HOURS', root: 0, scale: 'MINOR', octave: 3,
    bpm: 84, track: 0, playing: false, recording: false, metronome: false, position: 0, take: 0,
    tracks: ['FM', 'MONO', 'ORGANISM', 'GRANULAR'].map((name, i) => ({
      name, instrument: name, cycle: i === 1 ? 60 : 64, events: [],
    })),
  };
}

export class LiveSession {
  constructor(data = createLiveState()) {
    this.data = data; this.held = new Map();
    this.data.metronome = Boolean(this.data.metronome);
    this.serial = Math.max(0, ...data.tracks.flatMap(t => t.events.map(e => e.id)));
    this.history = []; this.openTake = false; this.notice = '';
    this.pitchFocus = this.notes[Math.floor(this.notes.length / 2)];
  }
  get track() { return this.data.tracks[this.data.track]; }
  get notes() { return SCALES[this.data.scale].map(n => n + this.data.root + 12 * (this.data.octave + 1)); }
  get head() { return mod(this.data.position, this.track.cycle); }
  get beat() { return Math.floor(this.data.position / 16) % 4; }
  toggleMetronome() { this.data.metronome = !this.data.metronome; }
  advance(seconds) {
    if (this.data.playing) this.data.position += Math.max(0, seconds) * this.data.bpm / 60 * 16;
  }
  snapshot() {
    this.history.push(copy(this.data.tracks));
    if (this.history.length > 12) this.history.shift();
  }
  beginTake() {
    if (!this.data.playing) this.data.position = 0;
    this.snapshot(); this.openTake = true; this.data.take++;
    this.data.recording = true; this.data.playing = true;
    // Already held notes keep sounding; only new key-downs enter this take.
  }
  endTake() {
    for (const h of this.held.values()) this.finishEvent(h);
    this.data.recording = false; this.openTake = false;
  }
  toggleRecord() {
    if (this.data.recording) this.endTake();
    else this.beginTake();
  }
  togglePlay() {
    if (this.data.playing) { this.endTake(); this.releaseAll(); this.data.playing = false; }
    else { this.data.position = 0; this.data.playing = true; }
  }
  finishEvent(h) {
    if (!h.event) return;
    h.event.duration = Math.min(this.track.cycle, Math.max(.125, this.data.position - h.started));
    h.event.rawDuration = h.event.duration; delete h.event.pending;
    h.event = null;
  }
  press(button) {
    if (this.held.has(button)) return;
    const index = BUTTONS.indexOf(button), note = this.notes[index];
    if (note === undefined) return;
    this.pitchFocus = note;
    // One melodic voice per track: last pressed note wins. Key highlights remain physical.
    const previous = [...this.held.values()].at(-1);
    if (previous) this.finishEvent(previous);
    const h = { note, index, started: this.data.position, event: null };
    if (this.data.recording) {
      if (this.track.events.length >= MAX_EVENTS) {
        this.notice = 'TAKE FULL / RECORD STOPPED'; this.endTake();
      } else {
        h.event = { id: ++this.serial, note, degree: index, at: this.head, rawAt: this.head,
          duration: .125, rawDuration: .125, take: this.data.take, pending: true };
        this.track.events.push(h.event);
      }
    }
    this.held.set(button, h);
  }
  release(button) {
    const h = this.held.get(button);
    if (!h) return;
    this.finishEvent(h); this.held.delete(button);
    // Deliberately no hidden retrigger of an older held note.
  }
  releaseAll() { for (const b of [...this.held.keys()]) this.release(b); }
  loseFocus() {
    this.endTake(); this.releaseAll(); this.data.playing = false;
  }
  changeTrack(direction) {
    if (this.data.recording) { this.notice = 'STOP REC TO CHANGE TRACK'; return false; }
    this.releaseAll(); this.data.track = mod(this.data.track + direction, this.data.tracks.length);
    return true;
  }
  changeOctave(direction) {
    this.data.octave = Math.max(1, Math.min(6, this.data.octave + direction));
    if (!this.held.size) this.pitchFocus = this.notes[Math.floor(this.notes.length / 2)];
  }
  undo() {
    if (this.data.recording) { this.notice = 'STOP REC TO UNDO'; return false; }
    if (!this.history.length) { this.notice = 'NOTHING TO UNDO'; return false; }
    this.releaseAll(); this.data.tracks = this.history.pop(); this.notice = 'UNDO'; return true;
  }
  quantize(division, strength = 1) {
    if (this.data.recording || !this.track.events.length) return false;
    this.snapshot(); const quantum = 64 / division;
    for (const e of this.track.events) {
      const snapped = Math.round(e.rawAt / quantum) * quantum;
      e.at = mod(e.rawAt + (snapped - e.rawAt) * strength, this.track.cycle);
    }
    this.notice = `QUANTIZE 1/${division} / ${Math.round(strength * 100)}%`;
    return true;
  }
  seed() {
    if (this.data.recording) return;
    this.snapshot();
    const notes = this.notes;
    this.track.events = [0, 2, 4, 3, 6, 7].map((degree, i) => {
      const at = [0.65, 9.7, 19.1, 29.5, 42.8, 56.3][i] * this.track.cycle / 64;
      return { id: ++this.serial, note: notes[degree % notes.length], degree: degree % notes.length,
        at, rawAt: at, duration: [7, 5, 8, 10, 6, 10][i], rawDuration: [7, 5, 8, 10, 6, 10][i], take: 0 };
    });
  }
  events() {
    return this.track.events.map(e => e.pending ? { ...e, duration: Math.min(this.track.cycle,
      Math.max(.125, this.data.position - [...this.held.values()].find(h => h.event === e).started)) } : e);
  }
  save() {
    const d = copy(this.data); d.playing = false; d.recording = false;
    d.tracks.forEach(t => t.events.forEach(e => delete e.pending));
    return d;
  }
}

// Split a note across the cycle boundary without treating its tail as a new trigger.
export function segments(event, cycle) {
  const start = mod(event.at, cycle), length = Math.min(cycle, event.duration);
  return start + length <= cycle ? [{ start, length, continuation: false }] : [
    { start, length: cycle - start, continuation: false },
    { start: 0, length: start + length - cycle, continuation: true },
  ];
}

// Eight readable pitch rows; hidden events are counted instead of mislabeled.
export function pitchRows(events, mappedNotes, focus) {
  const all = [...new Set([...mappedNotes, ...events.map(e => e.note)])].sort((a,b)=>a-b);
  const center = all.reduce((best,n,i)=>Math.abs(n-focus)<Math.abs(all[best]-focus)?i:best,0);
  const first = Math.max(0,Math.min(all.length-8,center-3));
  const notes = all.slice(first,first+8).reverse();
  return { notes, high:events.filter(e=>e.note>notes[0]).length,
    low:events.filter(e=>e.note<notes.at(-1)).length };
}

export function livePage(cycle, head, size) {
  const span=size*4, index=Math.floor(mod(head,cycle)/span), start=index*span;
  const ticks=Math.min(span,cycle-start);
  return { index, start, ticks, count:Math.ceil(ticks/4), total:Math.ceil(cycle/span) };
}
