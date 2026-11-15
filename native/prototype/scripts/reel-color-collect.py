"""Read completed short REEL profiles; --final validates and saves evidence."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import subprocess
import sys

root=Path(__file__).resolve().parents[3]
work=root/'work/reel-color'
mode=sys.argv[1] if len(sys.argv)>1 else 'color'
final='--final' in sys.argv
build=json.loads((work/(mode+'-build.json')).read_text())
remote=build['remote']
adb=str(root/'native/tooling/platform-tools/adb.exe')
destination=(root/'docs/verification/reel-color-v1'/mode) if final else (work/'snapshots'/remote.rsplit('/',1)[1])
destination.mkdir(parents=True,exist_ok=True)
def shell(command):
    return subprocess.check_output([adb,'shell',command],text=True).replace('\r','')
def pull(name,target):
    subprocess.run([adb,'pull',remote+'/'+name,str(target)],capture_output=True,check=True)
names=shell('ls '+remote).split()
reports={}
for row in build['profiles']:
    name=row[0]
    if name+'.json' not in names:continue
    pull(name+'.json',destination/(name+'.json'))
    report=json.loads((destination/(name+'.json')).read_text())
    reports[name]=report
    print(name, 'PASS' if report['profile_passed'] else 'FAIL',
          'mean/p99/max',report['callback_mean_us'],report['callback_p99_upper_us'],report['callback_max_us'],
          'late/over',report['late_intervals'],report['over_budget'])
if not final:sys.exit(0)
assert 'complete' in names and len(reports)==len(build['profiles']),'Not completed'
for row in build['profiles']:
    name,_,kind,mask,seconds=row;r=reports[name]
    assert shell('cat '+remote+'/'+name+'.exit').strip()=='0',name
    assert r['completed'] and r['profile_passed'] and r['elapsed_seconds']>=seconds,name
    assert r['rate']==32000 and r['buffer']==1024 and r['track_count']==4,name
    assert r['assets']==32 and r['pcm_bytes']==33554432,name
    for field in ['over_budget','late_intervals','stream_underruns','ui_slow_frames','schedule_overflows','audio_realtime_error']:
        assert r[field]==0,(name,field,r[field])
    assert r['capture_finished']>0 and all(r['lock_events']) and all(r['repeat_events']),name
    assert r['audio_priority']==10,name
    if kind==17:assert r['grain_peaks']==[8]*4 and r['fx_seen_mask']==mask,name
    if kind==20:assert r['fx_seen_mask']==31,name
for name,digest in build['sha256'].items():
    target=work/(mode+'-verified-'+name);pull(name,target)
    assert hashlib.sha256(target.read_bytes()).hexdigest()==digest,name
    if name.startswith('test-'):
        assert shell('cat '+remote+'/'+name+'.exit').strip()=='0'
        pull(name+'.log',destination/(name+'.txt'))
assert any(line.rstrip().endswith(' ./main') for line in shell('ps').splitlines()),'Garlic not restored'
assert shell('cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor').strip()=='interactive'
assert not shell('if [ -e /mnt/mmc/CFW/retroarch/command.sh ]; then echo occupied; fi').strip()
candidate=[r for name,r in reports.items() if not name.startswith('previous')]
manifest={'completed_utc':datetime.now(timezone.utc).isoformat(),'stage':'isolated REEL saturation/colour candidate',
          'production_installed':False,'release_qualified':False,'short_hardware_series_passed':True,
          'build':build,'actual_profile_seconds':sum(r['elapsed_seconds'] for r in reports.values()),
          'callbacks':sum(r['callbacks'] for r in reports.values()),
          'candidate_worst_us':max(r['callback_max_us'] for r in candidate),
          'candidate_worst_p99_us':max(r['callback_p99_upper_us'] for r in candidate),
          'max_28ms_target_met':all(r['callback_max_us']<=28000 for r in candidate),
          'p99_24ms_target_met':all(r['callback_p99_upper_us']<=24000 for r in candidate),
          'garlic_restored':True,'governor_restored':'interactive'}
if mode=='color':
    for name in ['host-unit.txt','regressions.json','audio-analysis.json']:
        (destination/name).write_bytes((work/name).read_bytes())
    manifest['source_sha256']={str(p.relative_to(root)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in [root/'native/prototype'/name for name in ['src/reel_color.h','src/reel_motion.h','src/effects.h',
          'src/effect_defs.h','src/engine.h','src/diagnostics/stress_scenario.h','tests/reel_color_test.cpp',
          'tests/reel_color_engine_test.cpp','tests/reel_motion_test.cpp']]}
(destination/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
