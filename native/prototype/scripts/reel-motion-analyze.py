"""Measure the rendered candidate, without pretending to perform a listening test."""
from pathlib import Path
import csv
import hashlib
import json
import math
import sys
import wave
import numpy as np

root = Path(__file__).resolve().parents[3]
audio = root / 'work/reel-motion/audio'
out = root / 'docs/verification/reel-motion-v1'
out.mkdir(parents=True, exist_ok=True)
rate = 32000
measurements = []
frequencies = [.1, .3, 4, 2, 6, 30]
for index in range(9):
    data = np.fromfile(audio / f'tone-{index}.f32', dtype='<f4').reshape(-1, 3)
    signal = data[:, 0].astype(float)
    delay = data[:, 2].astype(float)
    time = np.arange(len(signal)) / rate
    use = (time > 3) & (time < 23)
    expected = .5 * np.sin(2 * np.pi * 1000 * (time - delay / rate))
    residual = signal[use] - expected[use]
    # Analytic phase measures the *audio*, not just the oscillator control.
    spectrum = np.fft.fft(signal)
    analytic_weights = np.zeros(len(signal))
    analytic_weights[0] = analytic_weights[len(signal)//2] = 1
    analytic_weights[1:len(signal)//2] = 2
    phase = np.unwrap(np.angle(np.fft.ifft(spectrum * analytic_weights)))
    stride = 16
    frequency = (phase[2*stride:] - phase[:-2*stride]) * rate / (4*np.pi*stride)
    cents = 1200*np.log2(np.maximum(1e-10, frequency/1000))
    chosen = use[stride:-stride]
    measured = cents[chosen]
    result = dict(case=index, reconstruction_error_rms=float(np.sqrt(np.mean(residual**2))),
                  output_rms_db=float(20*np.log10(np.sqrt(np.mean(signal[use]**2)) / (.5/np.sqrt(2)))),
                  pitch_min_cents=float(measured.min()), pitch_max_cents=float(measured.max()),
                  max_head_speed_deviation=float(np.max(np.abs(np.diff(delay)))),
                  stereo_identical=bool(np.array_equal(data[:,0],data[:,1])))
    # A finite fractional-delay filter has a small amplitude/phase ripple;
    # compare to the ideal moving sine, not to a fictitious exact resampler.
    assert result['reconstruction_error_rms'] < .0005
    assert abs(result['output_rms_db']) < .03
    assert result['max_head_speed_deviation'] < .0262
    if index < 6:
        hz = frequencies[index]
        crossings = np.flatnonzero((delay[:-1] < 512) & (delay[1:] >= 512))
        crossing_times = (crossings + (512-delay[crossings])/(delay[crossings+1]-delay[crossings]))/rate
        crossing_times = crossing_times[crossing_times > 3]
        actual = 1/np.mean(np.diff(crossing_times))
        # Fit the audio's pitch modulation at the independently measured rate.
        t = time[stride:-stride][chosen]
        design = np.column_stack((np.sin(2*np.pi*actual*t),np.cos(2*np.pi*actual*t),np.ones(len(t))))
        coeff = np.linalg.lstsq(design, measured, rcond=None)[0]
        depth = float(np.hypot(coeff[0],coeff[1]))
        result.update(rate_hz=float(actual), rate_error_percent=float(abs(actual/hz-1)*100), audio_depth_cents=depth)
        assert result['rate_error_percent'] < 1
        assert abs(depth/10-1) < .1
    measurements.append(result)


def read_wav(path):
    with wave.open(str(path),'rb') as f:
        assert f.getframerate() == rate
        return np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').reshape(-1,2).astype(float)/32768


gain_rows = []
for name in ['bass','drums','pad','texture','mix']:
    dry = read_wav(audio/f'{name}-dry.wav')
    neutral = read_wav(audio/f'{name}-new-neutral.wav')
    assert np.array_equal(dry,neutral), name
    for variant in ['old-neutral','old-wobble','new-neutral','new-gentle','new-deep-wow','new-flutter','new-short']:
        x = read_wav(audio/f'{name}-{variant}.wav')
        gain = float(10*np.log10(np.sum(x*x)/np.sum(dry*dry)))
        peak = float(np.max(np.abs(x)))
        gain_rows.append(dict(source=name,variant=variant,rms_gain_db=gain,peak=peak))
        assert peak < .99
        # Separate, explicitly labelled loudness-matched comparisons for listening.
        matched = x * 10**(-gain/20)
        assert np.max(np.abs(matched)) < 1
        with wave.open(str(audio/f'{name}-{variant}-matched.wav'),'wb') as f:
            f.setnchannels(2);f.setsampwidth(2);f.setframerate(rate)
            f.writeframes(np.round(matched*32767).astype('<i2').tobytes())

# A concise audition file: 8 s dry / 8 s deep WOW / 8 s fast FLUTTER.
# This is deliberately separate from unmodified measurement renders.
chunks=[]
for name in ['pad-dry.wav','pad-new-deep-wow-matched.wav','pad-new-flutter-matched.wav']:
    part=read_wav(audio/name)[rate:rate*9].copy()
    fade=np.linspace(0,1,640)
    part[:640]*=fade[:,None];part[-640:]*=fade[::-1,None]
    chunks.append(part)
with wave.open(str(audio/'pad-audition.wav'),'wb') as f:
    f.setnchannels(2);f.setsampwidth(2);f.setframerate(rate)
    f.writeframes(np.round(np.concatenate(chunks)*32767).astype('<i2').tobytes())

with (out/'audio-levels.csv').open('w',newline='') as f:
    writer=csv.DictWriter(f,fieldnames=list(gain_rows[0]));writer.writeheader();writer.writerows(gain_rows)
(out/'audio-metrics.json').write_text(json.dumps(dict(rate=rate,neutral_pcm_exact=True,
    probe_seconds=24,cases=measurements,
    source_sha256={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in [
        'native/prototype/src/reel_motion.h','native/prototype/src/reel_tables.h',
        'native/prototype/tests/reel_motion_render.cpp']},
    audition_sha256=hashlib.sha256((audio/'pad-audition.wav').read_bytes()).hexdigest()),indent=2)+'\n')
print(json.dumps(measurements,indent=2))
print('PASS: exact aligned neutral WAVs; rate/depth, waveform reconstruction, bounded pitch and matched listening files.')

try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
except ImportError:
    sys.exit(0)
freq=np.linspace(20,14000,500)
worst=np.zeros_like(freq)
best=np.zeros_like(freq)
for fraction in np.linspace(0,1,257):
    x=np.arange(-5,7)-fraction
    h=np.sinc(x)*np.i0(5*np.sqrt(np.maximum(0,1-(x/6)**2)))/np.i0(5)
    h/=h.sum()
    amplitude=np.abs(np.sum(h*np.exp(-2j*np.pi*freq[:,None]*np.arange(12)/rate),axis=1))
    db=20*np.log10(amplitude)
    worst=np.minimum(worst,db);best=np.maximum(best,db)
fig,ax=plt.subplots(figsize=(9,4),layout='constrained')
ax.fill_between(freq,worst,best,color='#577b9a',alpha=.7,label='Moving REEL: fractional-position envelope')
ax.plot(freq,np.zeros_like(freq),color='#333333',label='Neutral: exact delayed input')
ax.axvline(12000,color='#996c50',linestyle='--',linewidth=1,label='Qualified passband: 12 kHz')
ax.set(xlim=(20,14000),ylim=(-3,.3),xlabel='Frequency, Hz',ylabel='Magnitude error, dB',title='REEL motion candidate · 32 kHz · 12-tap interpolation')
ax.grid(alpha=.2);ax.legend(loc='lower left',fontsize=8)
fig.savefig(out/'response.png',dpi=150);fig.savefig(out/'response.svg');plt.close(fig)
print('14 kHz worst fractional-position error:',float(worst[-1]),'dB')
