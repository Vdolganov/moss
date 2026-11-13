const {run,hostFlags,buildArm}=require('./toolchain.cjs');
const {execFileSync}=require('child_process');
const path=require('path');
run([...hostFlags(), 'tests/fx_character_test.cpp','-o','build/fx-character-test.exe']);
execFileSync(path.resolve(__dirname,'../build/fx-character-test.exe'),[],{cwd:path.resolve(__dirname,'..'),stdio:'inherit'});
buildArm('tests/fx_character_test.cpp','build/fx-character-test-arm',{includes:['src']});
