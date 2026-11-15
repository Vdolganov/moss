const fs=require('fs'),path=require('path'),{execFileSync}=require('child_process');
const {root,run,hostFlags,buildArm}=require('./toolchain.cjs');
fs.mkdirSync(path.join(root,'build/reel-motion'),{recursive:true});
run([...hostFlags(),'tests/reel_motion_test.cpp','-o','build/reel-motion/test.exe']);
execFileSync(path.join(root,'build/reel-motion/test.exe'),[],{stdio:'inherit',cwd:root});
buildArm('tests/reel_motion_test.cpp','build/reel-motion/test-arm',{includes:['src']});
buildArm('tests/reel_motion_garlic.cpp','build/reel-motion/moss',{includes:['src']});
buildArm('src/main_garlic.cpp','build/reel-motion/moss-baseline',{includes:['src']});
console.log('Built isolated REEL candidate, unmodified baseline and host/ARM checks.');
if(process.argv.includes('--render')) {
  const output=path.resolve(root,'../../work/reel-motion/audio');
  fs.mkdirSync(output,{recursive:true});
  run([...hostFlags(),'tests/reel_motion_render.cpp','-o','build/reel-motion/render.exe']);
  execFileSync(path.join(root,'build/reel-motion/render.exe'),[output],{stdio:'inherit',cwd:root});
}
