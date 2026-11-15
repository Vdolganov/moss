"""Deterministic spectral checks and level-matched listening examples."""
from pathlib import Path
import json
import subprocess
import wave
import re
import numpy as np

root=Path(__file__).resolve().parents[3]
work=root/'work/reel-color/audio'
work.mkdir(parents=True,exist_ok=True)
# Reproduce the compact halfband fit, including an exact DC constraint.
omega=np.linspace(0,2*np.pi*12000/64000,1000)
matrix=2*np.cos(omega[:,None]*np.arange(11,0,-2))
normal=np.block([[matrix.T@matrix,np.ones((6,1))],[np.ones((1,6)),np.zeros((1,1))]])
half=np.linalg.solve(normal,np.r_[matrix.T@np.full(len(omega),.5),.25])[:6]
coeff=np.r_[half,half[::-1]]
header=(root/'native/prototype/src/reel_color.h').read_text()
literal=re.search(r'Halfband\[12\]=\{([^}]+)\}',header).group(1)
compiled=np.array([float(x.strip().removesuffix('f')) for x in literal.split(',')])
assert np.max(abs(compiled-coeff))<1e-9
fir=np.zeros(23);fir[11]=.5;fir[::2]=coeff
hz=np.linspace(20,12000,601);w=2*np.pi*hz/64000
halfband_response=np.abs(np.exp(-1j*w[:,None]*np.arange(23))@fir)**2
correction=np.exp(-1j*w[:,None]*np.arange(4))@np.array([-.172,.672,.672,-.172])
linear_response=halfband_response*np.abs(correction)*np.abs(np.cos(w/2))
ripple=float(np.max(np.abs(20*np.log10(linear_response))))
assert ripple<.25,ripple
fixture=root/'work/reel-motion/audio/mix-dry.wav'
with wave.open(str(fixture),'rb') as f:
    assert f.getframerate()==32000 and f.getnchannels()==2 and f.getsampwidth()==2
    original=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,2)/32768.
original.astype('<f4').tofile(work/'music-input.f32')
exe=root/'native/prototype/build/reel-motion/color-test.exe'
result=subprocess.run([str(exe),str(work)],capture_output=True,text=True,check=True)
(work.parent/'host-unit.txt').write_text(result.stdout)
print(result.stdout,end='')
measurements=[]
for hz in [997,3991,7177,11003]:
    samples=np.fromfile(work/f'alias-{hz}.f32','<f4').reshape(-1,3)
    spectra=np.abs(np.fft.rfft(samples,axis=0))**2
    legitimate=np.zeros(len(spectra),bool)
    for harmonic in range(hz,16000,2*hz):
        legitimate[max(0,harmonic-1):harmonic+2]=True
    legitimate[0]=True
    values=[]
    for channel in [1,2]:
        # dBc relative to that implementation's actual fundamental, not its input.
        values.append(float(10*np.log10(spectra[~legitimate,channel].sum()/spectra[hz,channel])))
    reduction=values[1]-values[0]
    measurements.append({'input_hz':hz,'oversampled_alias_dbc':values[0],
                         'base_rate_alias_dbc':values[1],'reduction_db':reduction})
    print(f'{hz} Hz: alias {values[0]:.2f} dBc; 1x {values[1]:.2f}; reduction {reduction:.2f} dB')
    # A polynomial whose harmonics all fit under Nyquist is already at float
    # roundoff. A relative "improvement" there is not a useful quality measure.
    assert (values[0]<-100 and values[1]<-100) or reduction>8,(hz,reduction)

names=['dry','warm','dense','bright']
settings=[{'drive':0,'tone_db':0,'bump_db':0},{'drive':35,'tone_db':-1,'bump_db':2},
          {'drive':70,'tone_db':-2,'bump_db':4},{'drive':100,'tone_db':3,'bump_db':1}]
raw=[np.fromfile(work/f'music-{k}.f32','<f4').reshape(-1,2) for k in range(4)]
assert np.array_equal(raw[0],original.astype(np.float32))
rms=[float(np.sqrt(np.mean(x*x))) for x in raw]
matched=[x*(rms[0]/level) for x,level in zip(raw,rms)]
common_gain=min(1.,.95/max(float(abs(x).max()) for x in matched))

def wav(path,samples):
    assert abs(samples).max()<1
    with wave.open(str(path),'wb') as f:
        f.setnchannels(2);f.setsampwidth(2);f.setframerate(32000)
        f.writeframes(np.rint(samples*32767).astype('<i2').tobytes())

segments=[]
for name,config,signal,matched_signal,level in zip(names,settings,raw,matched,rms):
    config.update(name=name,rms=level,peak=float(abs(signal).max()),rms_gain_db=float(20*np.log10(level/rms[0])))
    wav(work/f'{name}-matched.wav',matched_signal*common_gain)
    segment=matched_signal[:32000*6].copy()*common_gain
    fade=np.linspace(0,1,640)[:,None];segment[:640]*=fade;segment[-640:]*=fade[::-1]
    segments.append(segment)
wav(work/'reel-color-audition.wav',np.concatenate(segments))
metrics={'rate':32000,'small_signal_test_amplitude':.00001,'alias_input_peak':.3,
         'anti_aliasing':'2x, 23-tap halfband, C2 quintic ADAA, 4-tap response correction',
         'linear_response_ripple_db_20_to_12000_hz':ripple,
         'alias':measurements,'music':settings,'listening_rms_matched':True,
         'audition_segments_seconds':6,'common_listening_gain':common_gain,
         'neutral_exact':True,'note':'Objective analysis, not a claim of subjective listening.'}
(work.parent/'audio-analysis.json').write_text(json.dumps(metrics,indent=2)+'\n')
