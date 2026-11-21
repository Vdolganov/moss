const fs=require('fs'),path=require('path'),{execFileSync}=require('child_process');
const {root,run,hostFlags}=require('./toolchain.cjs');
const out=path.resolve(root,'../../media/organism');fs.mkdirSync(out,{recursive:true});
const exe=path.join(root,'build/organism_render.exe');
run([...hostFlags(),'tests/organism_render.cpp','-o',exe]);
execFileSync(exe,[out],{cwd:root,stdio:'inherit'});
