# REEL baseline evidence / исходные измерения

15 November 2026. Research only; no new effect, production modification or device benchmark.

**RU:** [исследование и план](../../REEL-RESEARCH-PLAN.md). **EN:** [research and plan](../../REEL-RESEARCH-PLAN.en.md).

`probe.cpp` includes production `effects.h` directly. At 32 kHz it renders 171 coherent tones for three WEAR settings, three MIX settings and two input amplitudes: 3078 measurements. Each tone runs for four seconds; the first second is discarded and the next three are projected onto sine/cosine at the input frequency. Total RMS is measured separately. WOBBLE and NOISE are zero. WEAR is not time invariant: its amplitude modulation remains active, so these are finite-window measurements, not a linear transfer-function claim. Figures plot input peak −20 dBFS; CSV also includes peak approximately −6.02 dBFS.

`probe.exe phase` counts phase wraps in the actual processor over 64 seconds, separately from tone tests. `rates.csv` contains the observed periods, not just division of source constants.

Files:

- `tones.csv`: all tone results, fundamental gain and total RMS gain.
- `rates.csv`: oscillator measurement.
- `analyze.py`: figure/manifest generator (NumPy, Matplotlib).
- `response.png` / `response.svg`: inspected plots. Connected points illustrate sampled values, not densely measured continuous curves.
- `manifest.json`: baseline source hash, probe/data hashes, method, limitations, external GrainBrick release hashes and evidence boundaries.

Reproduce from the repository root, using the existing Zig toolchain:

```powershell
node -e "const t=require('./native/prototype/scripts/toolchain.cjs');t.run([...t.hostFlags(),'../../docs/verification/reel-baseline/probe.cpp','-o','../../work/reel-research/probe.exe'])"
& work/reel-research/probe.exe | Set-Content -Encoding utf8 docs/verification/reel-baseline/tones.csv
& work/reel-research/probe.exe phase | Set-Content -Encoding utf8 docs/verification/reel-baseline/rates.csv
python docs/verification/reel-baseline/analyze.py
```

Create the ignored `work/reel-research` directory first if absent. The research session used the bundled Windows Python and a local Matplotlib installation; other Python environments need NumPy and Matplotlib installed. Source commit/hash in the manifest identify this baseline, so update provenance deliberately if measuring a different version; the generator intentionally retains the recorded commit rather than guessing a new one.

**RU:** воспроизведение выполняется на компьютере; запускается именно текущий производственный REEL, без аналога формул на Python. Эти измерения не проверяют наушники, аналоговый выход, звук GrainBrick/RC-20 или скорость RG35XX. Данные WEAR>0 включают медленную амплитудную модуляцию. Сторонние бинарники и декомпиляции не включены в Git; в отчёте опубликованы только выводы и контрольные суммы.
