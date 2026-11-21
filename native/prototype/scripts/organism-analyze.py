"""Dry digital output measurements; not a perceptual or headphone assessment."""
from pathlib import Path
import json,wave,numpy as np
root=Path(__file__).resolve().parents[3]
out=root/'docs/verification/organism-v010';out.mkdir(parents=True,exist_ok=True)
rows=[]
for index,name in enumerate(['01-root','02-breath','03-tangle','04-bare']):
    with wave.open(str(root/'media/organism'/f'{name}.wav'),'rb') as f:
        assert f.getframerate()==32000 and f.getnchannels()==2 and f.getsampwidth()==2
        sound=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,2)/32768.
    x=sound[4*32000:8*32000];mono=x.mean(axis=1)
    spectrum=abs(np.fft.rfft(mono*np.hanning(len(mono))))**2
    hz=np.fft.rfftfreq(len(mono),1/32000)
    low=spectrum[(hz>=20)&(hz<100)].sum()/spectrum.sum()
    row={'preset':name,'dry_peak_dbfs':float(20*np.log10(abs(sound).max())),
         'sustain_rms_dbfs':float(20*np.log10(np.sqrt((x*x).mean()))),
         'mean_dc':float(mono.mean()),'stereo_difference_rms':float(np.sqrt(((x[:,0]-x[:,1])**2).mean())),
         'low_band_20_100_fraction':float(low),'hard_clipped_samples':int((abs(sound)>=.98).sum())}
    assert row['hard_clipped_samples']==0 and abs(row['mean_dc'])<.01 and row['sustain_rms_dbfs']>-40
    previous=root/'work/organism'/f'old-{index}.pcm'
    if previous.exists():
        old=np.frombuffer(previous.read_bytes(),'<i2').reshape(-1,2)[4*32000:8*32000].mean(axis=1)/32768
        def band_energy(y):
            spectrum=abs(np.fft.rfft(y*np.hanning(len(y))))**2
            return spectrum[(hz>=20)&(hz<100)].sum()
        row['low_band_db_vs_old_preset']=float(10*np.log10(band_energy(mono)/band_energy(old)))
    rows.append(row)
(out/'audio-metrics.json').write_text(json.dumps({'rate':32000,'note':'A1 / 55 Hz','window_seconds':[4,8],
    'comparison':'Original and new named presets; settings differ, not an isolated filter A/B or loudness-normalized audition.',
    'rows':rows},indent=2)+'\n')
for row in rows:print(row)
