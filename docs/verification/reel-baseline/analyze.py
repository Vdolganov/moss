"""Plot measured production REEL tones; requires NumPy and Matplotlib."""
from pathlib import Path
import csv
import json
import hashlib
import numpy as np
import matplotlib
matplotlib.use("Agg")
matplotlib.rcParams['svg.hashsalt'] = 'moss-reel-baseline-2026-11-15'
import matplotlib.pyplot as plt

base = Path(__file__).resolve().parent
root = base.parents[2]
rows = list(csv.DictReader((base / "tones.csv").open(encoding="utf-8-sig")))
def select(wear, mix, amplitude=0.1):
    return [r for r in rows if int(r['wear']) == wear and int(r['mix']) == mix
            and float(r['amplitude']) == amplitude]
def series(wear, mix):
    r = select(wear, mix)
    return np.array([float(x['frequency_hz']) for x in r]), np.array([float(x['fundamental_gain_db']) for x in r])

fig, axes = plt.subplots(1, 2, figsize=(12, 4.5), layout="constrained")
for mix, color in [(0, '#777777'), (100, '#357da3'), (50, '#b35e43')]:
    x,y=series(0,mix);mask=x<=1100
    axes[0].semilogx(x[mask],y[mask], '.-',color=color, linewidth=1,label=f'MIX {mix}%')
axes[0].set(title='Unintended notches, WEAR = 0', xlabel='Frequency (Hz)', ylabel='Fundamental gain (dB)', ylim=(-32,3))
axes[0].annotate('83.33 Hz: −28.15 dB',xy=(83.333,-28.15),xytext=(26,-21),arrowprops={'arrowstyle':'->','color':'#444444'},fontsize=9)
for wear,color in [(0,'#357da3'),(30,'#b49a4e'),(100,'#b35e43')]:
    x,y=series(wear,100);reference=float(next(r['fundamental_gain_db'] for r in select(wear,100) if float(r['frequency_hz'])==1000))
    axes[1].semilogx(x,y-reference,color=color,linewidth=1.5,label=f'WEAR {wear}%')
axes[1].set(title='Coloration, MIX = 100%',xlabel='Frequency (Hz)',ylabel='Relative to 1 kHz (dB)',ylim=(-17,4))
for ax in axes:
    ax.grid(True,which='both',alpha=.18);ax.legend(loc='lower left',fontsize=9)
fig.suptitle('Existing MOSS REEL · 32 kHz · input −20 dBFS peak · WOBBLE / NOISE = 0',fontsize=12)
fig.savefig(base/'response.png',dpi=150)
fig.savefig(base/'response.svg',metadata={'Date':'2026-11-15'})
svg=base/'response.svg'
svg.write_text('\n'.join(line.rstrip() for line in svg.read_text(encoding='utf8').splitlines())+'\n',encoding='utf8',newline='\n')
selected=[r for r in rows if float(r['amplitude'])==.1 and float(r['frequency_hz']) in (250,1000,8000,12000) and (r['wear']=='0' or r['mix']=='100')]
phase=list(csv.DictReader((base/'rates.csv').open(encoding='utf-8-sig')))
manifest={
 'date':'2026-11-15','kind':'host research measurement, not a device benchmark',
 'source_commit':'59c7cd217fd43acc8c63de9b39ab39134913f38b',
 'sample_rate':32000,'input_amplitudes_peak':[.1,.5], 'tone_rows':len(rows),
 'warmup_seconds':1,'measurement_seconds':3,
 'method':'3-second coherent sinusoidal projection; frequencies are integer multiples of 1/3 Hz; measure fundamental amplitude and total RMS separately',
 'limitations':['WEAR > 0 also applies slow amplitude modulation: these are finite-window tone measurements, not an LTI transfer function.', 'No headphones, analog output or listening comparison measured.', 'No new REEL implementation or ARM test performed.'],
 'oscillators':phase,'selected_tones':selected,
 'sources':{
  'effects.h':hashlib.sha256((root/'native/prototype/src/effects.h').read_bytes()).hexdigest(),
  'probe.cpp':hashlib.sha256((base/'probe.cpp').read_bytes()).hexdigest(),
  'tones.csv':hashlib.sha256((base/'tones.csv').read_bytes()).hexdigest(),
  'rates.csv':hashlib.sha256((base/'rates.csv').read_bytes()).hexdigest()
 },
 'grainbrick':{'release':'v1.0.1-beta.2','url':'https://github.com/ETRNLIAM/GrainBrick/releases/tag/v1.0.1-beta.2',
  'archive_sha256':'e2d985f7a99b8f123b0d39369b47bc8ed7b72be4a2296cc5f7ed2c8682518d7a',
  'tape_library_sha256':'174dfbf9d0cc8bcbb05c4a234c0c95e50ee719c03598a7f492eaa1edcceca9e7',
  'main_module_sha256':'bcef28522efb789105ac5126612e97692374d1a5eff65b58a9a1876650b8f2a8',
  'method':'static analysis of native tape library; main-module strings; official tutorial frames at 4:16 and 4:19',
  'tutorial':'https://www.youtube.com/watch?v=Iy-IeH4_EvQ&t=252s',
  'limits':'No original source, no execution or benchmark on RG35XX, no full UI decompilation. Third-party binaries/decompilations remain in ignored work/.'}
}
(base/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
print(json.dumps({'rows':len(rows),'oscillators':phase},indent=2))
