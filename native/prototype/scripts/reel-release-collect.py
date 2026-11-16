from pathlib import Path
import json,subprocess,sys
root=Path(__file__).resolve().parents[3]
work=root/'work/reel-release';revision=sys.argv[1]
manifest=json.loads((work/(revision+'-build.json')).read_text())
adb=str(root/'native/tooling/platform-tools/adb.exe');remote=manifest['remote']
def pull(name,target):subprocess.run([adb,'pull',remote+'/'+name,str(target)],capture_output=True,check=True)
def shell(command):return subprocess.check_output([adb,'shell',command],text=True).replace('\r','')
assert shell('cat '+remote+'/complete').strip()=='done','The series is incomplete'
# Optional destination lets later DSP revisions reuse the same strict collector.
out=root/sys.argv[2] if len(sys.argv)>2 else root/'docs/verification/reel-v1'/revision
out.mkdir(exist_ok=True,parents=True)
results=[]
for name,kind,mask,seconds in manifest['profiles']:
    target=out/(name+'.json');pull(name+'.json',target);pull(name+'.log',out/(name+'-output.txt'))
    report=json.loads(target.read_text());results.append(report)
    assert report['completed'] and report['profile_passed'],(name,report)
    for key in ['over_budget','late_intervals','stream_underruns','schedule_overflows','audio_realtime_error']:
        assert report[key]==0,(name,key,report[key])
    assert report['track_count']==4 and report['pcm_bytes']==33554432
    print(name,'mean',report['callback_mean_us'],'p99',report['callback_p99_upper_us'],'max',report['callback_max_us'])
if manifest.get('checks'):
    for name in ['reel-test','motion-test','color-test','engine-test']:
        assert shell('cat '+remote+'/'+name+'.exit').strip()=='0',name
        pull(name+'.log',out/(name+'-output.txt'))
manifest.update(qualified=True,callbacks=sum(r['callbacks'] for r in results),
                max_us=max(r['callback_max_us'] for r in results),seconds=sum(int(p[3]) for p in manifest['profiles']))
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Qualified',manifest['callbacks'],'callbacks /',manifest['seconds'],'seconds')
