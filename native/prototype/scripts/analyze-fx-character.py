"""Optional offline spectral check: Python + NumPy; no device dependencies.

Run fx_character_audio first, then pass its output directory. This deliberately
bounded sine test is not an alias-free claim or a listening-quality score.
"""
from pathlib import Path
import json,sys,wave
import numpy as np

directory=Path(sys.argv[1])
result={'alias_fixture':'32 kHz, 16384 samples, periodic 1978.515625 Hz sine, input amplitude 3', 'curves':[], 'renders':[]}
for curve in range(3):
    x=np.fromfile(directory/f'alias-curve-{curve}.f32',dtype='<f4').reshape(2,16384)
    energy=np.abs(np.fft.rfft(x.astype(np.float64)))**2
    unwanted=np.ones(8193,dtype=bool)
    unwanted[[0,1013,3039,5065,7091]]=False # DC and true odd harmonics below Nyquist
    alias_db=10*np.log10(energy[:,unwanted].sum(axis=1)/energy[:,1013])
    improvement=float(alias_db[0]-alias_db[1])
    assert improvement>3, (curve,alias_db)
    result['curves'].append({'curve':curve,'naive_alias_db_vs_fundamental':float(alias_db[0]),'adaa_alias_db_vs_fundamental':float(alias_db[1]),'reduction_db':improvement})
for path in sorted(directory.glob('0*.wav')):
    with wave.open(str(path),'rb') as f:
        x=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').astype(np.float64)/32768
    result['renders'].append({'file':path.name,'peak':float(np.max(np.abs(x))),'rms':float(np.sqrt(np.mean(x*x))),'dc':float(np.mean(x))})
    assert np.max(np.abs(x))<1 and np.max(np.abs(x))>.01
(directory/'audio-analysis.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
