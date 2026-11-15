// Engine members. Audio owns the voices; the renderer reads bounded atomic snapshots.
GranularTables grainTables;
GranularVoice granular[Tracks];
int grainAsset[Tracks] = {-1, -1, -1, -1};
int grainValues[Tracks][GranularParameterCount] = {};
int grainFreeze[Tracks] = {};
int grainRemaining[Tracks] = {};
unsigned grainPreview[Tracks] = {};
struct GrainView {
  uint32_t head = 0, count = 0, running = 0;
  uint32_t first = 0, last = 65536, scatter = 0, density = 0;
  struct Item {
    uint32_t position = 0, age = 0, origin = 0, span = 0;
    uint32_t first = 0, last = 65536, lane = 0, gain = 0, loops = 0;
  } grains[8];
};
struct GrainMailbox {
  static constexpr int Header = 7, Stride = 9, Words = Header + 8 * Stride;
  uint32_t sequence = 0, words[Words] = {};
  void publish(const GranularVoice &v) {
    uint32_t serial = __atomic_load_n(&sequence, __ATOMIC_RELAXED);
    __atomic_store_n(&sequence, serial + 1, __ATOMIC_SEQ_CST);
    uint32_t data[Words] = {};
    if (v.sample) {
      data[0] = uint32_t((uint64_t(v.head) >> 16) / v.sample->frames);
      data[1] = v.count;
      data[2] = v.running;
      data[3] = uint32_t((uint64_t(v.scanFirst) >> 16) / v.sample->frames);
      data[4] = uint32_t((uint64_t(v.scanLast) >> 16) / v.sample->frames);
      data[5] = uint32_t(v.parameters.scatter * 65536);
      data[6] = uint32_t(v.effectiveDensity * 1000);
      const float scale = 65536.f / v.sample->frames;
      for (unsigned i = 0; i < v.count && i < 8; ++i) {
        const auto &g = v.grains[i];
        uint32_t *item = data + Header + i * Stride;
        item[0] = uint32_t((g.index + g.fraction) * scale);
        item[1] = uint32_t(uint64_t(g.age) * 65536 / g.length);
        item[2] = uint32_t(g.origin * 65536);
        item[3] = uint32_t(fminf(1, g.sourceSpan) * 65536);
        item[4] = uint32_t(g.first * scale); item[5] = uint32_t(g.last * scale);
        item[6] = g.lane;
        item[7] = uint32_t(v.grainWindow(g) * 65536);
        item[8] = uint32_t(g.sourceSpan * v.sample->frames / (g.last-g.first));
      }
    }
    for (int i = 0; i < Words; ++i) __atomic_store_n(&words[i], data[i], __ATOMIC_RELAXED);
    __atomic_store_n(&sequence, serial + 2, __ATOMIC_RELEASE);
  }
  GrainView read() const {
    GrainView view;
    for (int attempt = 0; attempt < 3; ++attempt) {
      uint32_t before = __atomic_load_n(&sequence, __ATOMIC_ACQUIRE);
      if (before & 1) continue;
      uint32_t data[Words];
      for (int i = 0; i < Words; ++i) data[i] = __atomic_load_n(&words[i], __ATOMIC_RELAXED);
      __atomic_thread_fence(__ATOMIC_ACQUIRE);
      if (__atomic_load_n(&sequence, __ATOMIC_RELAXED) != before) continue;
      view.head = data[0]; view.count = data[1]; view.running = data[2];
      view.first = data[3]; view.last = data[4]; view.scatter = data[5]; view.density=data[6];
      for (int i = 0; i < 8; ++i) {
        const uint32_t *g=data+Header+i*Stride;
        view.grains[i] = {g[0],g[1],g[2],g[3],g[4],g[5],g[6],g[7],g[8]};
      }
      break;
    }
    return view;
  }
} grainView[Tracks];
void bindGranular(int t) {
  granular[t].stop();
  grainAsset[t] = -1;
  grainPreview[t] = grainRemaining[t] = 0;
  const auto &tr = project.tracks[t];
  int asset = tr.granular.asset;
  if (tr.instrument != GranularInstrument || asset < 0 || asset >= sampleCount || !samples[asset].frames) {
    granular[t] = GranularVoice{};
    return;
  }
  if (!granular[t].init(samples[asset], grainTables, rate, GranularVoice::NativeGrains,
                        project.seed ^ unsigned(t + 1) * 73537u)) return;
  grainAsset[t] = asset;
  grainFreeze[t] = -1; // force initial parameter preparation
  refreshGranular(t, nullptr);
}
void refreshGranular(int t, const Step *step) {
  if (grainAsset[t] < 0) return;
  const auto &patch = project.tracks[t].granular;
  int v[GranularParameterCount];
  for (int i = 0; i < GranularParameterCount; ++i)
    v[i] = step && lockValue(step->grainLocks[i]) ? step->grainLocks[i] : patch.values[i];
  // Locks may cross independently: preserve a non-empty scan range at playback.
  v[5] = clamp(v[5], 0, 999); v[6] = clamp(v[6], v[5] + 1, 1000);
  v[4] = clamp(v[4], v[5], v[6]);
  if (!memcmp(v, grainValues[t], sizeof(v)) && grainFreeze[t] == patch.freeze) return;
  memcpy(grainValues[t], v, sizeof(v)); grainFreeze[t] = patch.freeze;
  GranularParameters p;
  p.speed = v[0] * .01f; p.pitch = float(v[1]); p.sizeMs = float(v[2]); p.density = float(v[3]);
  p.position = v[4] * .001f; p.start = v[5] * .001f; p.end = v[6] * .001f; p.scatter = v[7] * .01f;
  p.spread = v[8] * .01f; p.jump = v[9] * .01f; p.cutoff = float(v[10]); p.level = v[11] * .01f;
  p.grainIn = v[12] * .01f; p.grainOut = v[13] * .01f; p.attackMs = float(v[14]); p.releaseMs = float(v[15]);
  p.freeze = patch.freeze != 0;
  granular[t].setParameters(p);
}
void triggerGranular(int t, const Step &step) {
  if (grainAsset[t] < 0) return;
  refreshGranular(t, &step);
  if (step.grainLocks[4] != Inherit) granular[t].seek(grainValues[t][4] * .001f);
  granular[t].noteOn(float(step.note - GranularRootNote), step.velocity / 127.f);
  grainRemaining[t] = step.length;
  grainPreview[t] = 0;
  seenInstrumentMask |= 1u << GranularInstrument;
  ++triggerCount[t]; lastTrigger[t] = uint32_t(frame);
}
void renderGranular(int t, int &l, int &r) {
  if (grainPreview[t] && !--grainPreview[t]) granular[t].noteOff();
  float left, right;
  granular[t].render(left, right);
  l += int(left * 32768.f); r += int(right * 32768.f);
}
