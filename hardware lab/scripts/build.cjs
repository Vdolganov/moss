const fs=require('fs'),path=require('path'),crypto=require('crypto'),{execFileSync}=require('child_process');
const lab=path.resolve(__dirname,'..'),tools=path.resolve(lab,'../native/tooling'),zig=path.join(tools,'zig-x86_64-windows-0.14.1/zig.exe');
const sdk=path.join(tools,'sdk/opt/miyoo'),sys=path.join(sdk,'arm-miyoo-linux-uclibcgnueabi/sysroot'),gcc=path.join(sdk,'lib/gcc/arm-miyoo-linux-uclibcgnueabi/7.3.0'),build=path.join(lab,'build');
fs.mkdirSync(build,{recursive:true});
function run(args){execFileSync(zig,args,{stdio:'inherit',cwd:lab})}
const target=['-target','arm-linux-gnueabi','-mcpu=cortex_a9','-mfloat-abi=softfp','-fno-stack-protector','-fno-pie','-nostdlib'];
const flags=[...target,'-nostdinc','-I',path.join(sys,'usr/include'),'-isystem',path.join(tools,'zig-x86_64-windows-0.14.1/lib/include'),'-std=c++17','-fno-exceptions','-fno-rtti','-Wall','-Wextra','-Werror','-Os'];
run(['c++',...flags,'-c',path.join(lab,'src/main.cpp'),'-o',path.join(build,'moss-lab.o')]);
run(['ld.lld','-m','armelf_linux_eabi','--sysroot='+sys,'--dynamic-linker=/lib/ld-uClibc.so.0','--hash-style=sysv','--no-undefined','-L'+path.join(sys,'usr/lib'),'-L'+path.join(sys,'lib'),'-L'+gcc,path.join(sys,'usr/lib/crt1.o'),path.join(sys,'usr/lib/crti.o'),path.join(gcc,'crtbegin.o'),path.join(build,'moss-lab.o'),'-lSDL','-lm','-lrt','-lc','-lgcc',path.join(gcc,'crtend.o'),path.join(sys,'usr/lib/crtn.o'),'-o',path.join(build,'moss-lab')]);
console.log('ARM binary: '+path.join(build,'moss-lab'));
run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror',path.join(lab,'tests/core_test.cpp'),'-o',path.join(build,'core-test.exe')]);execFileSync(path.join(build,'core-test.exe'),[],{stdio:'inherit'});
run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror',path.join(lab,'tests/input_test.cpp'),'-o',path.join(build,'input-test.exe')]);execFileSync(path.join(build,'input-test.exe'),[],{stdio:'inherit'});
run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror',path.join(lab,'tests/timing_test.cpp'),'-o',path.join(build,'timing-test.exe')]);execFileSync(path.join(build,'timing-test.exe'),[],{stdio:'inherit'});
run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror',path.join(lab,'tests/sample_test.cpp'),'-o',path.join(build,'sample-test.exe')]);execFileSync(path.join(build,'sample-test.exe'),[],{stdio:'inherit',cwd:lab});
run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror',path.join(lab,'src/preview.cpp'),'-o',path.join(build,'preview.exe')]);for(let i=0;i<5;i++)execFileSync(path.join(build,'preview.exe'),[String(i),path.join(build,'preview-'+i+'.bmp')],{stdio:'inherit',cwd:lab});
execFileSync(path.join(build,'preview.exe'),['0',path.join(build,'preview-hold.bmp'),'hold'],{stdio:'inherit'});
fs.writeFileSync(path.join(build,'test-results.json'),JSON.stringify({arm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'moss-lab'))).digest('hex'),core_tests:'passed',input_tests:'passed',timing_tests:'passed',sample_tests:'passed',render_previews:5,device_test:'0.1.2 audio and holds confirmed by user; 0.1.3 sample device test pending'},null,2)+'\n');
