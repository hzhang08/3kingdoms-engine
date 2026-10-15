import {readFile} from 'node:fs/promises';
import {randomBytes} from 'node:crypto';
import {execFileSync} from 'node:child_process';
const config=JSON.parse(await readFile('wrangler.jsonc','utf8'));
if(config.name!=='caocao-game'||config.account_id!=='ad9fe8d09042662dde159d59992cf1bd')throw Error('Unexpected deployment target');
const client=JSON.parse(await readFile(process.argv[2]||'/Users/henryzhang/Documents/credentials/3kingdoms-google.json','utf8')).web;
if(!client?.client_id||!client?.client_secret||!client.redirect_uris?.includes('https://3kingdoms.cc.cd/api/auth/callback/google'))throw Error('Dedicated Google client or callback missing');
let jev=(await readFile('/Users/henryzhang/Documents/credentials/vercel-cert-jev.txt','utf8')).trim();
if((jev[0]==='"'||jev[0]==="'")&&jev.at(-1)===jev[0])jev=jev.slice(1,-1);
if(!jev||/\s/.test(jev))throw Error('Invalid JEV credential');
const wrangler=['node_modules/wrangler/bin/wrangler.js'];
const existing=JSON.parse(execFileSync(process.execPath,[...wrangler,'secret','list'],{encoding:'utf8',stdio:['ignore','pipe','pipe']}));
const secrets={GOOGLE_CLIENT_ID:client.client_id,GOOGLE_CLIENT_SECRET:client.client_secret,JEV_API_KEY:jev};
if(!existing.some(s=>s.name==='BETTER_AUTH_SECRET'))secrets.BETTER_AUTH_SECRET=randomBytes(48).toString('base64url');
try{execFileSync(process.execPath,[...wrangler,'secret','bulk'],{input:JSON.stringify(secrets),stdio:['pipe','pipe','pipe']});}catch{throw Error('Secret upload failed; no credential details are printed');}
console.log('Installed dedicated Google and JEV Worker secrets. Existing session secret preserved.');
