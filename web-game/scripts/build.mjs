import {build} from 'esbuild';
import {readFile,writeFile,mkdir,cp,readdir,stat,rm} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import path from 'node:path';
const root=path.resolve('..'),dist=path.resolve('dist');
await mkdir(dist,{recursive:true});
await cp('public',dist,{recursive:true});
const sha=b=>createHash('sha256').update(b).digest('hex');
const files=await Promise.all(['js','wasm','data'].map(x=>readFile(path.join(root,'build-web/prototype.'+x))));
const id=sha(Buffer.concat(files)).slice(0,16),base='/engine/'+id;
await mkdir(dist+base,{recursive:true});
await writeFile(dist+base+'/prototype.js',files[0]);await writeFile(dist+base+'/prototype.wasm',files[1]);
const chunks=[];for(let at=0,n=0;at<files[2].length;at+=20*1024*1024,n++){
 const part=files[2].subarray(at,at+20*1024*1024),url=base+'/data-'+n+'.bin';await writeFile(dist+url,part);chunks.push({url,bytes:part.length,sha256:sha(part)});
}
const engine={version:'0.19.1-web-special-dong-no-beans',script:base+'/prototype.js',wasm:base+'/prototype.wasm',bytes:files[2].length,chunks};
await writeFile(dist+'/engine.json',JSON.stringify(engine));
const p=JSON.parse(await readFile(root+'/docs/evidence/equipment-ui/cross-platform-snapshot.json','utf8')).payload;
await writeFile('src/release.json',JSON.stringify({identity:Object.fromEntries(Object.entries(p).filter(([k])=>k.endsWith('_manifest')))}));
await build({entryPoints:['client/app.ts'],bundle:true,format:'iife',target:'es2022',minify:true,outfile:dist+'/app.js'});
await writeFile(dist+'/NOTO-SANS-FONT-LICENSE.txt',(await readFile(root+'/assets/presentation-font/NOTICE.txt','utf8'))+'\n'+(await readFile(root+'/assets/presentation-font/OFL.txt','utf8')));
await cp('client/index.html',dist+'/index.html');await cp('client/style.css',dist+'/style.css');
await mkdir(dist+'/special/hulao',{recursive:true});
await build({entryPoints:['client/special.ts'],bundle:true,format:'iife',target:'es2022',minify:true,outfile:dist+'/special/hulao/special.js'});
await cp('client/special.html',dist+'/special/hulao/index.html');await cp('client/special.css',dist+'/special/hulao/special.css');
await cp(root+'/assets/special-hulao/preview.jpg',dist+'/special/hulao/preview.jpg');
// Remove only obsolete generated engine bundles, never user data or source.
for(const entry of await readdir(dist+'/engine'))if(entry!==id)await rm(dist+'/engine/'+entry,{recursive:true});
await writeFile(dist+'/_headers',`/*
  X-Content-Type-Options: nosniff
  Referrer-Policy: same-origin
  X-Frame-Options: DENY
  Permissions-Policy: camera=(), microphone=(), geolocation=()
  Content-Security-Policy: default-src 'self'; script-src 'self' 'wasm-unsafe-eval'; style-src 'self'; img-src 'self' data:; connect-src 'self'; media-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none'; form-action 'self'
/engine/*
  Cache-Control: public, max-age=31536000, immutable
/engine.json
  Cache-Control: no-cache
/index.html
  Cache-Control: no-cache
/app.js
  Cache-Control: no-cache
/style.css
  Cache-Control: no-cache
/special/hulao/*
  Cache-Control: no-cache
`);
for(const name of ['FONT-LICENSE.txt','SDL-LICENSE.txt','JSON-LICENSE.txt'])try{await cp(root+'/dist/prototype-web/'+name,dist+'/'+name);}catch{}
await writeFile(dist+'/robots.txt','User-agent: *\nDisallow: /api/\n');
const manifest=[];async function inspect(dir){for(const name of await readdir(dir)){const file=path.join(dir,name),s=await stat(file);if(s.isDirectory())await inspect(file);else{if(s.size>25*1024*1024)throw Error('Cloudflare asset too large: '+file);manifest.push({path:path.relative(dist,file),bytes:s.size});}}}await inspect(dist);
await writeFile(root+'/docs/evidence/web-game/build.json',JSON.stringify({engine:id,assets:manifest},null,2));
console.log(`Built ${manifest.length} public assets; game data split into ${chunks.length} pieces.`);
