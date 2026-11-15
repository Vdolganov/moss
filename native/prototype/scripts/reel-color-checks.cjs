const fs=require('fs'),path=require('path'),{execFileSync}=require('child_process');
const {root,run,hostFlags}=require('./toolchain.cjs');
const work=path.resolve(root,'../../work/reel-color');fs.mkdirSync(work,{recursive:true});
const cases=[['reel_color_test',false,true],['reel_motion_test',false,true],
  ['fx_character_test',false,false],['swing_test',true,false],['trigless_repeat_test',true,false],
  ['reel_color_engine_test',true,false]];
const results=[];
for(const [test,lab,sanitize] of cases) {
  const binary=path.join(root,'build/reel-motion',test+'-check.exe');
  run([...hostFlags(sanitize?'-O1':'-O3'),'-I','src',...(lab?['-DMOSS_REEL_MOTION_LAB']:[]),
    ...(sanitize?['-fsanitize=undefined','-fno-sanitize-recover=all']:[]),
    'tests/'+test+'.cpp','-o',binary]);
  const output=execFileSync(binary,[],{cwd:root,encoding:'utf8'});
  results.push({test,lab,sanitize,passed:true,output});console.log('PASS',test,lab?'lab':'',sanitize?'UBSan':'');
}
fs.writeFileSync(path.join(work,'regressions.json'),JSON.stringify(results,null,2)+'\n');
