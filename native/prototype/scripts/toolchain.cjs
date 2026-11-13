const fs = require('fs');
const path = require('path');
const {execFileSync} = require('child_process');
const root = path.resolve(__dirname, '..');
const tooling = path.resolve(root, '../tooling');
const zig = path.join(tooling, 'zig-x86_64-windows-0.14.1/zig.exe');
const sdk = path.join(tooling, 'sdk/opt/miyoo');
const sysroot = path.join(sdk, 'arm-miyoo-linux-uclibcgnueabi/sysroot');
const gcc = path.join(sdk, 'lib/gcc/arm-miyoo-linux-uclibcgnueabi/7.3.0');
function run(args) { execFileSync(zig, args, {cwd: root, stdio: 'inherit'}); }
function armFlags(optimization = '-O3') {
  return ['-target','arm-linux-gnueabi','-mcpu=cortex_a9','-mfloat-abi=softfp',
    '-fno-stack-protector','-fno-pie','-nostdlib','-nostdinc','-I',path.join(sysroot,'usr/include'),
    '-isystem',path.join(tooling,'zig-x86_64-windows-0.14.1/lib/include'),
    '-std=c++17','-fno-exceptions','-fno-rtti','-Wall','-Wextra','-Werror',optimization];
}
function buildArm(source, output, {optimization = '-O3', includes = [], assembly} = {}) {
  fs.mkdirSync(path.dirname(output), {recursive:true});
  const flags = [...armFlags(optimization), ...includes.flatMap(p=>['-I',p])];
  if (assembly) run(['c++',...flags,'-S',source,'-o',assembly]);
  run(['c++',...flags,'-c',source,'-o',output+'.o']);
  run(['ld.lld','-m','armelf_linux_eabi','--sysroot='+sysroot,
    '--dynamic-linker=/lib/ld-uClibc.so.0','--hash-style=sysv','--no-undefined',
    '-L'+path.join(sysroot,'usr/lib'),'-L'+path.join(sysroot,'lib'),'-L'+gcc,
    path.join(sysroot,'usr/lib/crt1.o'),path.join(sysroot,'usr/lib/crti.o'),path.join(gcc,'crtbegin.o'),
    output+'.o','-lSDL','-lm','-lrt','-lc','-lgcc',path.join(gcc,'crtend.o'),path.join(sysroot,'usr/lib/crtn.o'),'-o',output]);
}
function hostFlags(optimization = '-O3') {
  return ['c++','-Wl,--stack,33554432','-std=c++17',optimization,'-Wall','-Wextra','-Werror'];
}
module.exports = {root,tooling,zig,run,armFlags,buildArm,hostFlags};
