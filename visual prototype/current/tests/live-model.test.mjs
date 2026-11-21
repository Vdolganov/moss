import test from 'node:test';
import assert from 'node:assert/strict';
import {LiveSession, BUTTONS, MAX_EVENTS, segments, pitchRows, livePage} from '../src/live-model.js';

test('free playing does not write notes; keyboard repeat cannot retrigger',()=>{
  const s=new LiveSession();s.press('LEFT');s.press('LEFT');assert.equal(s.held.size,1);
  assert.equal(s.track.events.length,0);s.release('LEFT');assert.equal(s.held.size,0);
});
test('record from stop, exact off-grid onset and held length, record off preserves transport',()=>{
  const s=new LiveSession();s.toggleRecord();s.advance(.123);const at=s.head;
  s.press('LEFT');s.advance(.75);s.release('LEFT');s.toggleRecord();
  assert.equal(s.data.playing,true);assert.equal(s.data.recording,false);
  assert.equal(s.track.events[0].at,at);assert.notEqual(at%4,0);
  assert.ok(Math.abs(s.track.events[0].duration-.75*84/60*16)<1e-8);
});
test('held note crosses loop without an extra trigger',()=>{
  const s=new LiveSession();s.toggleRecord();s.data.position=62;s.press('Y');s.advance(.5);s.release('Y');
  assert.equal(s.track.events.length,1);const parts=segments(s.track.events[0],64);
  assert.equal(parts.length,2);assert.equal(parts[1].continuation,true);
});
test('notes already held on record are not retriggered or invented',()=>{
  const s=new LiveSession();s.press('LEFT');s.toggleRecord();s.advance(1);s.release('LEFT');
  assert.equal(s.track.events.length,0);
});
test('last pressed melodic note ends previous recorded gate',()=>{
  const s=new LiveSession();s.toggleRecord();s.press('LEFT');s.advance(.2);s.press('Y');
  const duration=s.track.events[0].duration;s.advance(.5);s.release('LEFT');
  assert.equal(s.track.events[0].duration,duration);s.release('Y');assert.equal(s.track.events.length,2);
});
test('quantization keeps raw timing and duration; undo restores exact timing',()=>{
  const s=new LiveSession();s.seed();const before=structuredClone(s.track.events);
  s.quantize(16,.5);assert.notEqual(s.track.events[0].at,before[0].at);
  s.quantize(16,1);assert.equal(s.track.events[0].at,0);
  assert.equal(s.track.events[0].rawAt,before[0].rawAt);
  assert.equal(s.track.events[0].duration,before[0].duration);
  s.undo();s.undo();assert.deepEqual(s.track.events,before);
});
test('recording is one undoable take and cannot switch target track',()=>{
  const s=new LiveSession();s.seed();const before=structuredClone(s.track.events);
  s.toggleRecord();s.press('A');s.advance(.2);s.release('A');
  assert.equal(s.changeTrack(1),false);assert.equal(s.data.track,0);
  s.toggleRecord();s.undo();assert.deepEqual(s.track.events,before);
});
test('focus loss stops recording and releases every held key',()=>{
  const s=new LiveSession();s.toggleRecord();s.press('A');s.advance(.2);s.loseFocus();
  assert.equal(s.data.playing,false);assert.equal(s.held.size,0);
  assert.equal(s.track.events[0].pending,undefined);
});
test('pentatonic unused pads cannot create undefined notes',()=>{
  const s=new LiveSession();s.data.scale='PENTATONIC';s.toggleRecord();
  BUTTONS.forEach(b=>s.press(b));s.releaseAll();assert.equal(s.track.events.length,6);
});
test('fixed capacity stops recording but preserves free play and data',()=>{
  const s=new LiveSession();s.toggleRecord();
  for(let i=0;i<MAX_EVENTS+1;i++){s.press('LEFT');s.advance(.01);s.release('LEFT');}
  assert.equal(s.track.events.length,MAX_EVENTS);assert.equal(s.data.recording,false);assert.equal(s.data.playing,true);
});
test('polyrhythm position follows selected track cycle',()=>{
  const s=new LiveSession();s.data.position=62;assert.equal(s.head,62);s.changeTrack(1);assert.equal(s.head,2);
});
test('octave change preserves held pitch and complete recorded gate',()=>{
  const s=new LiveSession();s.toggleRecord();s.press('LEFT');s.advance(.25);s.changeOctave(1);s.advance(.25);
  assert.equal(s.held.get('LEFT').note,48);assert.equal(s.notes[0],60);s.release('LEFT');
  assert.equal(s.track.events[0].note,48);assert.ok(Math.abs(s.track.events[0].duration-11.2)<1e-9);
});
test('score rows retain actual pitch after scale, root and octave changes',()=>{
  const s=new LiveSession();s.seed();s.data.root=2;s.data.scale='MAJOR';s.changeOctave(1);
  const rows=pitchRows(s.track.events,s.notes,48);
  assert.ok(rows.notes.includes(48));assert.equal(rows.notes.length,8);
  assert.equal(rows.high+rows.low+s.track.events.filter(e=>rows.notes.includes(e.note)).length,s.track.events.length);
});
test('16/64 view changes page count, not pattern duration; partial page and wrap',()=>{
  assert.deepEqual(livePage(304,260,64),{index:1,start:256,ticks:48,count:12,total:2});
  assert.deepEqual(livePage(304,260,16),{index:4,start:256,ticks:48,count:12,total:5});
  assert.equal(livePage(304,304,64).index,0);assert.equal(livePage(60,62,64).count,15);
});
test('metronome follows shared beat, survives save, and never changes transport',()=>{
  const s=new LiveSession();s.data.position=62;s.changeTrack(1);s.toggleMetronome();
  assert.equal(s.beat,3);assert.equal(s.head,2);assert.equal(s.data.playing,false);
  assert.equal(new LiveSession(s.save()).data.metronome,true);
  s.toggleRecord();assert.equal(s.data.metronome,true);s.toggleMetronome();assert.equal(s.data.recording,true);
});
