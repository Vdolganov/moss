export class Storage {
 async open(){this.db=await new Promise((resolve,reject)=>{const r=indexedDB.open('moss-interface-012',1);r.onupgradeneeded=()=>{r.result.createObjectStore('state');r.result.createObjectStore('sources');};r.onsuccess=()=>resolve(r.result);r.onerror=()=>reject(r.error);});return this;}
 async read(store,key){return new Promise((resolve,reject)=>{const r=this.db.transaction(store).objectStore(store).get(key);r.onsuccess=()=>resolve(r.result);r.onerror=()=>reject(r.error);});}
 async write(store,key,value){return new Promise((resolve,reject)=>{const t=this.db.transaction(store,'readwrite');t.objectStore(store).put(value,key);t.oncomplete=resolve;t.onerror=()=>reject(t.error);t.onabort=()=>reject(t.error);});}
 load(){return this.read('state','library');}save(data){return this.write('state','library',data);}source(id){return this.read('sources',id);}saveSource(id,blob){return this.write('sources',id,blob);}
}
// Parse RIFF for visual inspection. No AudioContext, playback, DSP or transcoding.
export async function readWave(file){
 const buffer=await file.arrayBuffer(),v=new DataView(buffer);const str=(p,n)=>String.fromCharCode(...new Uint8Array(buffer,p,n));
 if(buffer.byteLength<44||str(0,4)!=='RIFF'||str(8,4)!=='WAVE')throw Error('WAV REQUIRED / CHOOSE A RIFF WAVE FILE');
 let fmt=null,data=null;for(let p=12;p+8<=buffer.byteLength;){const tag=str(p,4),size=v.getUint32(p+4,true);if(p+8+size>buffer.byteLength)throw Error('INCOMPLETE WAV FILE');if(tag==='fmt '&&size>=16)fmt={format:v.getUint16(p+8,true),channels:v.getUint16(p+10,true),rate:v.getUint32(p+12,true),align:v.getUint16(p+20,true),bits:v.getUint16(p+22,true)};if(tag==='data')data={offset:p+8,size};p+=8+size+(size%2);}
 if(!fmt||!data||![1,3].includes(fmt.format)||![1,2].includes(fmt.channels)||![8,16,24,32].includes(fmt.bits)||fmt.rate<8000||fmt.rate>192000||(fmt.format===3&&fmt.bits!==32)||fmt.align!==fmt.channels*fmt.bits/8)throw Error('UNSUPPORTED WAV / PCM MONO OR STEREO');
 const frames=Math.floor(data.size/fmt.align);if(frames<4)throw Error('WAV TOO SHORT');
 const sample=(frame,ch=0)=>{const p=data.offset+Math.max(0,Math.min(frames-1,frame))*fmt.align+ch*fmt.bits/8;let value=0;if(fmt.format===3)value=v.getFloat32(p,true);else if(fmt.bits===8)value=(v.getUint8(p)-128)/128;else if(fmt.bits===16)value=v.getInt16(p,true)/32768;else if(fmt.bits===24){let n=v.getUint8(p)|(v.getUint8(p+1)<<8)|(v.getUint8(p+2)<<16);if(n&0x800000)n-=0x1000000;value=n/8388608;}else value=v.getInt32(p,true)/2147483648;return Number.isFinite(value)?Math.max(-1,Math.min(1,value)):0;};
 const peaks=(first=0,last=frames,n=1024,ch=0)=>Array.from({length:n},(_,i)=>{const a=Math.floor(first+i*(last-first)/n),b=Math.max(a+1,Math.floor(first+(i+1)*(last-first)/n));let lo=1,hi=-1;const skip=Math.max(1,Math.floor((b-a)/256));for(let f=a;f<b;f+=skip){const value=sample(f,ch);lo=Math.min(lo,value);hi=Math.max(hi,value);}return[lo,hi];});
 return {...fmt,frames,bytes:frames*fmt.channels*2,wave:peaks(),waveR:fmt.channels===2?peaks(0,frames,1024,1):null,sample,peaks};
}
