const http=require('node:http'),fs=require('node:fs'),path=require('node:path');
const root=__dirname;
const types={'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.json':'application/json; charset=utf-8'};
const allowed=new Set(['index.html','style.css','refinements.css','app.js','model.js']);
const server=http.createServer((req,res)=>{const name=new URL(req.url,'http://localhost').pathname.slice(1)||'index.html';if(!allowed.has(name)){res.writeHead(404);return res.end('Not found');}fs.readFile(path.join(root,name),(error,data)=>{if(error){res.writeHead(500);return res.end('Read error');}res.writeHead(200,{'Content-Type':types[path.extname(name)],'Cache-Control':'no-store'});res.end(data);});});
server.listen(4173,'127.0.0.1',()=>console.log('MOSS: http://127.0.0.1:4173'));
server.on('error',e=>{console.error(e.message);process.exitCode=1;});
