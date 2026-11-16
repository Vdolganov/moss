"""Analyze ambient-render output; requires NumPy. T20 is an estimate, not a perceptual score."""
from pathlib import Path
import argparse,json,wave
import numpy as np

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('directory',type=Path)
parser.add_argument('--output',type=Path)
args=parser.parse_args();report={};sr=32000
for k in range(6):
    x=np.fromfile(args.directory/f'pond-ir-{k}.f32',dtype='<f4').reshape(-1,2).astype(float)
    assert np.isfinite(x).all()
    energy=np.sum(x*x,axis=1);edc=np.cumsum(energy[::-1])[::-1]
    assert edc[0]>0
    db=10*np.log10(np.maximum(edc/edc[0],1e-30));selection=(db<-5)&(db>-25)
    fit=np.polyfit(np.arange(len(x))[selection]/sr,db[selection],1)
    frames=x[int(.5*sr):sr];deviation=frames.std(axis=0)
    density=float(np.mean(np.abs(frames)>deviation)/.31731)
    correlation=float(np.dot(x[:,0],x[:,1])/np.sqrt(np.dot(x[:,0],x[:,0])*np.dot(x[:,1],x[:,1])))
    report[f'pond-{k}']={
        't20_extrapolated_seconds':round(float(-60/fit[0]),3),
        'energy':round(float(energy.sum()),6),'late_normalized_echo_density':round(density,3),
        'lr_correlation':round(correlation,4),
        'first_nonzero_ms':round(float(np.where(energy>1e-15)[0][0])*1000/sr,2),
        'peak':round(float(np.max(np.abs(x))),5)}
for k in range(4):
    x=np.fromfile(args.directory/f'cloud-{k}.f32',dtype='<f4').reshape(-1,2)
    assert np.isfinite(x).all()
    steady=x[sr*4:sr*10];peak=float(np.abs(x).max())
    assert peak<.99
    report[f'cloud-{k}']={'peak':peak,'steady_rms_dbfs':round(float(20*np.log10(np.sqrt(np.mean(steady**2)))),2)}
    with wave.open(str(args.directory/f'cloud-{k}.wav'),'wb') as f:
        f.setnchannels(2);f.setsampwidth(2);f.setframerate(sr);f.writeframes((x*32767).astype('<i2').tobytes())
base=report['pond-0']
assert abs(report['pond-4']['first_nonzero_ms']-base['first_nonzero_ms']-100)<.1
assert report['pond-1']['t20_extrapolated_seconds']>base['t20_extrapolated_seconds']*2
assert base['late_normalized_echo_density']>report['pond-2']['late_normalized_echo_density']*1.5
assert report['pond-5']['energy']<base['energy']
target=args.output or args.directory/'analysis.json'
target.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2))
