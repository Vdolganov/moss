"""Collect and validate the exact isolated REEL qualification build.

Use --final only after the launcher has restored Garlic. Without it, print a
compact live snapshot and collect completed short reports into the work folder.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import subprocess
import sys

root=Path(__file__).resolve().parents[3]
work=root/'work/reel-motion'
build=json.loads((work/'build.json').read_text())
remote=build['remote']
adb=str(root/'native/tooling/platform-tools/adb.exe')
final='--final' in sys.argv
shortened=None
if '--shortened-soak' in sys.argv:
    shortened=json.loads((work/'shortened-soak.json').read_text())
    assert shortened['remote']==remote
destination=root/'docs/verification/reel-motion-v1' if final else work/'hardware-snapshot'
destination.mkdir(parents=True,exist_ok=True)


def shell(command):
    return subprocess.check_output([adb,'shell',command],text=True).replace('\r','')


def pull(name,target):
    subprocess.run([adb,'pull',remote+'/'+name,str(target)],check=True,capture_output=True)


names=shell('ls '+remote).split()
print('Stage:',shell('cat '+remote+'/current').strip())
reports={}
for name in sorted(names):
    if name.endswith('.json'):
        pull(name,destination/name)
        report=json.loads((destination/name).read_text())
        reports[name.removesuffix('.json')]=report
        print(name, 'PASS' if report['profile_passed'] else 'INCOMPLETE/FAIL',
              'mean/p99/max:',report['callback_mean_us'],report['callback_p99_upper_us'],report['callback_max_us'])
if not final:
    paths=shell('ls '+remote+'/perf/run/reports').split()
    latest=max((x for x in paths if x.endswith('.json')),default=None)
    if latest:
        data=json.loads(shell('cat '+remote+'/perf/run/reports/'+latest))
        print('Live:',json.dumps({key:data[key] for key in ['elapsed_seconds','callbacks','callback_max_us','over_budget','late_intervals','ui_slow_frames','stream_underruns']}))
    sys.exit(0)

assert 'complete' in names, 'Series has not finished'
if build.get('mode','full')=='quick':
    expected={'baseline-10','candidate-10','mixed'}
else:
    expected={f'{variant}-{mask}' for variant in ['baseline','candidate'] for mask in [9,10,12,24,18]}
    expected.update(['sampler','mixed','song','soak-reel-pond'])
assert set(reports)==expected, 'Missing or unexpected hardware reports'
assert shell('cat '+remote+'/unit.exit').strip()=='0'
for name,report in reports.items():
    assert shell('cat '+remote+'/'+name+'.exit').strip()=='0',name
    shortened_this_run=shortened is not None and name=='soak-reel-pond'
    effective_target=shortened['minimum_observed_seconds'] if shortened_this_run else report['profile_target_seconds']
    assert report['completed'] and (report['profile_passed'] or shortened_this_run),name
    assert report['elapsed_seconds']>=effective_target,name
    assert report['rate']==32000 and report['buffer']==1024,name
    assert report['track_count']==4 and report['assets']==32 and report['pcm_bytes']==33554432,name
    for field in ['over_budget','late_intervals','stream_underruns','ui_slow_frames','schedule_overflows','audio_realtime_error']:
        assert report[field]==0,(name,field,report[field])
    assert report['capture_finished']>0,name
    assert all(x>0 for x in report['lock_events']),name
    assert all(x>0 for x in report['repeat_events']),name
    assert report['audio_priority']==10,name
    if report['profile_kind']==17:
        assert report['grain_peaks']==[8]*4,name
        assert report['fx_seen_mask']==report['profile_effect_mask'],name
    if name=='song':assert report['fx_seen_mask']==31,name

if shortened:assert not reports['soak-reel-pond']['profile_passed'] # Retain the raw unmet 1200-second target.
processes=shell('ps')
assert any(line.rstrip().endswith(' ./main') for line in processes.splitlines()),'Garlic not restored'
assert not any(line.rstrip().endswith((' ./moss',' ./moss-baseline',' ./test-arm')) for line in processes.splitlines())
governor=shell('cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor').strip()
assert governor=='interactive',governor
assert shell('if [ -e /mnt/mmc/CFW/retroarch/command.sh ]; then echo occupied; fi').strip()==''

for name,digest in build['sha256'].items():
    target=work/('verified-device-'+name)
    pull(name,target)
    assert hashlib.sha256(target.read_bytes()).hexdigest()==digest,name
pull('unit.log',destination/'arm-unit.txt')
for source,name in [('host-unit.txt','host-unit.txt'),('ubsan.txt','host-ubsan.txt'),('regressions.json','host-regressions.json')]:
    (destination/name).write_bytes((work/source).read_bytes())
regressions=json.loads((work/'regressions.json').read_text())
assert all(item['passed'] for item in regressions)
pairs=[]
for mask in ([10] if build.get('mode','full')=='quick' else [9,10,12,24,18]):
    old,new=reports[f'baseline-{mask}'],reports[f'candidate-{mask}']
    pairs.append(dict(mask=mask,baseline_mean_us=old['callback_mean_us'],candidate_mean_us=new['callback_mean_us'],
                      delta_mean_us=new['callback_mean_us']-old['callback_mean_us'],
                      baseline_p99_upper_us=old['callback_p99_upper_us'],candidate_p99_upper_us=new['callback_p99_upper_us'],
                      baseline_max_us=old['callback_max_us'],candidate_max_us=new['callback_max_us']))
candidate=[value for key,value in reports.items() if not key.startswith('baseline-')]
manifest=dict(completed_utc=datetime.now(timezone.utc).isoformat(),stage='isolated clean-path + wow/flutter candidate',
    device='original RG35XX / GarlicOS 1.4.9',production_installed=False,hardware_series_passed=True,
    release_qualified=False,reason='Preview/capture routing, final UI/persistence, listening and cold-boot qualification remain separate integration work.',
    binary=build,run_count=len(reports),total_profile_seconds=sum(x['elapsed_seconds'] for x in reports.values()),
    candidate_profile_seconds=sum(x['elapsed_seconds'] for x in candidate),
    callbacks=sum(x['callbacks'] for x in reports.values()),candidate_callbacks=sum(x['callbacks'] for x in candidate),
    candidate_worst_callback_us=max(x['callback_max_us'] for x in candidate),
    candidate_worst_p99_upper_us=max(x['callback_p99_upper_us'] for x in candidate),
    active_reel_mean_cost_target_met=(all(x['delta_mean_us']<=1500 for x in pairs if x['mask']!=18)
                                    if build.get('mode','full')=='full' else None),
    overall_p99_24ms_target_met=all(x['callback_p99_upper_us']<=24000 for x in candidate),
    overall_max_28ms_target_met=all(x['callback_max_us']<=28000 for x in candidate),
    audio_faults=0,queue_overflows=0,slow_ui_frames=0,governor_restored=governor,garlic_restored=True,
    comparison=pairs,soak=reports.get('soak-reel-pond'),soak_shortened_by_user=shortened,reports=list(reports),
    source_sha256={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in [
        'native/prototype/src/reel_motion.h','native/prototype/src/reel_tables.h',
        'native/prototype/src/effects.h','native/prototype/src/effect_defs.h',
        'native/prototype/src/diagnostics/stress_scenario.h',
        'native/prototype/tests/reel_motion_garlic.cpp','native/prototype/tests/reel_motion_test.cpp']})
(destination/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
