"""Short, isolated production-binary tests. Never changes the installed MOSS."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
root=Path(__file__).resolve().parents[3]
adb=str(root/'native/tooling/platform-tools/adb.exe')
work=root/'work/reel-release';work.mkdir(exist_ok=True,parents=True)
revision=sys.argv[1];assert re.fullmatch(r'v[0-9]+',revision)
checks='--checks' in sys.argv[2:]
fm_checks='--fm-check' in sys.argv[2:]
profiles=[tuple(row.split(':')) for row in sys.argv[2:] if row not in ('--checks','--fm-check')]
assert profiles and all(len(p)==4 and all(x.isdigit() for x in p[1:]) for p in profiles)
assert all(0<=int(p[1])<=31 and 0<=int(p[2])<32 and 10<=int(p[3])<=1800 for p in profiles),'Unsupported kind/mask/duration; device profiles require at least 10 seconds'
remote='/mnt/mmc/MOSS-Reel-Release-'+revision
def shell(s):return subprocess.check_output([adb,'shell',s],text=True).replace('\r','')
def push(src,dst):subprocess.run([adb,'push',str(src),dst],capture_output=True,check=True)
processes=shell('ps');assert not re.search(r'\./moss\b',processes),'MOSS is open'
menu=[line.split()[1] for line in processes.splitlines() if line.rstrip().endswith(' ./main')]
assert len(menu)==1,'Expected Garlic menu'
assert not shell('if [ -e /mnt/mmc/CFW/retroarch/command.sh ]; then echo occupied; fi').strip()
assert remote.rsplit('/',1)[1] not in shell('ls /mnt/mmc'),'Use a fresh directory'
binary=root/'native/prototype/build/reel-release/moss'
shell('mkdir '+remote);push(binary,remote+'/moss');shell('chmod 755 '+remote+'/moss')
push(root/'native/prototype/packaging/cpu-performance.sh',remote+'/cpu-performance.sh')
script='#!/bin/sh\nPATH=/bin:/usr/bin:/sbin:/usr/sbin\nexport PATH SDL_NOMOUSE=1\ncd '+remote+' || exit 2\n. ./cpu-performance.sh || exit 3\n'
if fm_checks:
    push(root/'native/prototype/build/fm_update_test-arm',remote+'/fm-test');shell('chmod 755 '+remote+'/fm-test')
    script+='\n./fm-test > fm-test.log 2>&1\nstatus=$?; echo "$status" > fm-test.exit\n[ "$status" -eq 0 ] || exit "$status"\n'
if checks:
    for name in ['reel-test','motion-test','color-test','engine-test']:
        push(binary.parent/name,remote+'/'+name);shell('chmod 755 '+remote+'/'+name)
        script+=f'./{name} > {name}.log 2>&1\nstatus=$?; echo "$status" > {name}.exit\n[ "$status" -eq 0 ] || exit "$status"\n'
for name,kind,mask,seconds in profiles:
    assert re.fullmatch(r'[a-z0-9-]+',name)
    script+=f'''echo {name} > current
./moss --profile {kind} {mask} {seconds} > {name}.log 2>&1
status=$?; echo "$status" > {name}.exit
report=$(ls -t perf/run/reports/*.json | head -n 1)
cp "$report" {name}.json || exit 4
[ "$status" -eq 0 ] || exit "$status"
'''
script+='sync\necho done > complete\n'
launcher=work/(revision+'-launcher.sh');launcher.write_text(script,newline='\n')
(work/(revision+'-build.json')).write_text(json.dumps({'remote':remote,'sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'profiles':profiles,'checks':checks},indent=2))
push(launcher,'/mnt/mmc/CFW/retroarch/command.sh');shell('kill '+menu[0])
print('Started',remote,'/',sum(int(p[3]) for p in profiles),'seconds')
