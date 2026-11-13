"""Numerical checks of offline examples; not listening or ARM timing tests."""
import json
from pathlib import Path
import wave
import numpy as np
root=Path(__file__).resolve().parents[1]
n=np.arange(17)-8
h=.5*np.sinc(.5*n)*np.hamming(17)
h/=h.sum()
fixed=np.array([0,-.005239181,0,.023211101,0,-.076105845,0,.307698779,.500870292,.307698779,0,-.076105845,0,.023211101,0,-.005239181,0])
results={'fir_max_difference':float(np.max(np.abs(h-fixed))),'audio':{}}
assert results['fir_max_difference']<1e-8
for name in ('mono','organism','fm'):
    with wave.open(str(root/'build'/f'{name}.wav'),'rb') as w:
        assert w.getnchannels()==2 and w.getframerate()==44100 and w.getsampwidth()==2
        x=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(float)/32768
    assert np.all(np.isfinite(x)) and np.max(np.abs(x))<.999
    assert np.sqrt(np.mean(x*x))>.001 and abs(np.mean(x))<.001
    results['audio'][name]={'peak_dbfs':round(float(20*np.log10(np.max(np.abs(x)))),2),'rms_dbfs':round(float(20*np.log10(np.sqrt(np.mean(x*x)))) ,2),'mean_dc':round(float(np.mean(x)),7),'clipped_samples':int(np.sum(np.abs(x)>=.999))}
(root/'build/audio-metrics.json').write_text(json.dumps(results,indent=2))
print(json.dumps(results,indent=2))
