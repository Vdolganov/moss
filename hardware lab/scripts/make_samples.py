"""Deterministic original test sounds, saved as real PCM16 WAV files."""
import math, pathlib, random, struct, wave
root=pathlib.Path(__file__).resolve().parents[1]/'samples'
root.mkdir(exist_ok=True)
def write(name,rate,channels,seconds,fn):
    count=round(rate*seconds)
    with wave.open(str(root/name),'wb') as f:
        f.setnchannels(channels);f.setsampwidth(2);f.setframerate(rate)
        data=bytearray()
        for i in range(count):
            t=i/rate;edge=min(1,i/(rate*.01),(count-1-i)/(rate*.01))
            for ch in range(channels):
                v=round(fn(t,ch)*edge)
                data.extend(struct.pack('<h',max(-32768,min(32767,v))))
        f.writeframes(data)
    print(name,(root/name).stat().st_size,'bytes')
write('chime.wav',44100,1,.75,lambda t,ch:4200*math.exp(-t*5)*(math.sin(2*math.pi*440*t)+.3*math.sin(2*math.pi*1100*t)))
write('air.wav',48000,2,.8,lambda t,ch:2400*math.sin(2*math.pi*(220 if ch==0 else 275)*t)+1100*math.sin(2*math.pi*330*t))
rng=random.Random(357);state=[0.0]
def dust(t,ch):
    state[0]=.8*state[0]+.2*rng.uniform(-1,1)
    return 8000*state[0]*(.55+.45*math.sin(2*math.pi*3*t)**2)
write('dust.wav',22050,1,1,dust)
