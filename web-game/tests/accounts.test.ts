import {before,after,test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile,readdir,mkdtemp,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {createHmac} from 'node:crypto';
import {getPlatformProxy} from 'wrangler';
import worker from '../src/worker';
import {Env} from '../src/types';
import {canonical,hash,validateSnapshot,username} from '../src/snapshots';
let proxy:Awaited<ReturnType<typeof getPlatformProxy>>,env:Env,temp:string;
const origin='https://3kingdoms.cc.cd',secret='local-integration-only-'.repeat(3);
const tokens:Record<string,string>={a:'token-a-private-test',b:'token-b-private-test',unverified:'token-unverified',expired:'token-expired'};
const cookie=(who:string)=>'__Secure-caocao.session_token='+encodeURIComponent(tokens[who]+'.'+createHmac('sha256',secret).update(tokens[who]).digest('base64'));
async function req(path:string,method='GET',value?:unknown,who='a',requestOrigin=origin){
 const headers:Record<string,string>={Origin:requestOrigin};if(who)headers.Cookie=cookie(who);if(value!==undefined)headers['Content-Type']='application/json';
 const response=await worker.fetch(new Request(origin+'/api'+path,{method,headers,body:value===undefined?undefined:JSON.stringify(value)}),env);return {status:response.status,data:await response.json() as any};
}
before(async()=>{
 temp=await mkdtemp(tmpdir()+'/caocao-web-test-');proxy=await getPlatformProxy({configPath:'wrangler.jsonc',persist:{path:temp}});
 env={...proxy.env as object,APP_ORIGIN:origin,BETTER_AUTH_SECRET:secret,GOOGLE_CLIENT_ID:'local-test.apps.googleusercontent.com',GOOGLE_CLIENT_SECRET:'local-test-no-oauth',JEV_DAILY_USER_LIMIT:'10',JEV_DAILY_GLOBAL_LIMIT:'20'} as unknown as Env;
 for(const file of (await readdir('migrations')).filter(f=>f.endsWith('.sql')).sort()){
  const sql=await readFile('migrations/'+file,'utf8');for(const statement of sql.split(';').map(s=>s.trim()).filter(Boolean))await env.DB.prepare(statement).run();
 }
 const now=Date.now();for(const id of Object.keys(tokens)){
  await env.DB.prepare('INSERT INTO user(id,name,email,emailVerified,createdAt,updatedAt) VALUES(?,?,?,?,?,?)').bind(id,id,id+'@test.invalid',id==='unverified'?0:1,now,now).run();
  await env.DB.prepare('INSERT INTO session(id,expiresAt,token,createdAt,updatedAt,userId) VALUES(?,?,?,?,?,?)').bind(id,id==='expired'?now-1000:now+3600000,tokens[id],now,now,id).run();
 }
});
after(async()=>{await proxy?.dispose();if(temp)await rm(temp,{recursive:true,force:true});});
test('private routes reject anonymous, expired, unverified and cross-origin writes',async()=>{
 for(const who of ['', 'unverified','expired'])assert.equal((await req('/saves','GET',undefined,who)).status,401);
 assert.equal((await req('/profile','POST',{username:'曹孟德'},'a','https://evil.invalid')).status,403);
 assert.equal((await req('/me')).data.user.id,'a');assert.equal((await req('/runs','POST',{})).status,428);
 assert.equal((await req('/special/hulao/leaderboard')).status,428);
 const forged=await worker.fetch(new Request(origin+'/api/saves',{headers:{'x-user-id':'a',Cookie:'__Secure-caocao.session_token=forged'}}),env);assert.equal(forged.status,401);
});
test('first login username, normalized uniqueness and unsafe names',async()=>{
 assert.equal((await req('/profile','POST',{username:'曹孟德'})).status,201);
 assert.equal((await req('/profile','POST',{username:'曹孟德'},'b')).status,409);
 assert.equal((await req('/profile','POST',{username:'孙仲谋'},'b')).status,201);
 assert.throws(()=>username('<script>'));assert.equal(username('ＡＢ').key,username('ab').key);
 assert.equal((await req('/me')).data.user.username,'曹孟德');
 assert.equal((await req('/profile','PUT',{username:'孙仲谋'})).status,409);
 assert.equal((await req('/me')).data.user.username,'曹孟德');
});
test('C++ canonical checksum, ownership, CAS, corrupted saves, progression and ranking',async()=>{
 const fixture=JSON.parse(await readFile('../docs/evidence/equipment-ui/cross-platform-snapshot.json','utf8'));
 await validateSnapshot(JSON.stringify(fixture));
 const run=(await req('/runs','POST',{})).data;const other=(await req('/runs','POST',{},'b')).data;
 const base=fixture.payload;
 // These are isolated server-contract fixtures, not claims of battle playthrough.
 const make=async(scenario:number,won=false)=>{
  const p=structuredClone(base);p.scenario=scenario;p.campaign=scenario?{schema:1,origin:0,battles:Array.from({length:scenario},(_,n)=>({scenario:n,progress:{victory:true,active:null,choice:null}}))}:{};
  p.state.turn=2;p.state.phase=won?'victory':'player';p.progress.victory=won;p.progress.active=null;p.progress.choice=null;p.dialogue={pending:[],page:0,triggered:[]};
  return JSON.stringify({payload:p,sha256:await hash(canonical(p))});
 };
 const initial=await make(0);
 assert.equal((await req('/saves/1','PUT',{runId:other.id,revision:0,snapshot:initial})).status,404);
 const first=await req('/saves/1','PUT',{runId:run.id,revision:0,snapshot:initial});assert.equal(first.status,200,JSON.stringify(first));
 assert.equal((await req('/saves/1','GET',undefined,'b')).status,404);
 assert.equal((await req('/saves/1','PUT',{runId:run.id,revision:0,snapshot:initial})).status,409);
 assert.equal((await req('/saves/1','PUT',{runId:run.id,revision:1,snapshot:initial.replace('"sha256":"','"sha256":"bad')})).status,422);
 assert.equal((await req('/saves/1','GET')).data.snapshot,initial);
 assert.equal((await req('/saves/2','PUT',{runId:run.id,revision:0,snapshot:await make(3,true)})).status,422);
 for(let n=0;n<4;n++)assert.equal((await req('/saves/0','PUT',{runId:run.id,revision:n,snapshot:await make(n,true)})).status,200);
 const ranking=await req('/leaderboard?kind=completed');assert.equal(ranking.data.entries[0].username,'曹孟德');const stamp=ranking.data.entries[0].completed_at;
 assert.equal((await req('/leaderboard?kind=progress')).data.entries.length,1);
 assert.equal((await req('/saves/0','PUT',{runId:run.id,revision:4,snapshot:initial})).status,200);
 assert.equal((await req('/leaderboard?kind=completed')).data.entries[0].completed_at,stamp);
 assert.equal((await req('/saves/1','GET')).data.snapshot,initial);
});
test('JEV denies unsigned requests and returns only a bounded choice, never a credential',async()=>{
 assert.equal((await req('/jev','POST',{},'')).status,401);
 const run=(await req('/runs','POST',{})).data;
 const r={key:'a'.repeat(64),state:{protocol:'caocao-revised-v5',actor:100,scenario:0},choices:{a0:{x:1,y:1,target:-1}}};
 const first=await req('/jev','POST',{runId:run.id,request:r});assert.equal(first.status,200);assert.equal(first.data.choice,'a0');assert.equal(first.data.source,'fallback');
 assert.equal((await req('/jev','POST',{runId:run.id,request:r})).data.source,'cache');
 assert.deepEqual(Object.keys(first.data).sort(),['choice','key','source']);
 const abandoned={...r,key:'b'.repeat(64)};
 await env.DB.prepare("INSERT INTO jev_decisions(user_id,request_key,run_id,choice,source,created_at) VALUES(?,?,?,'a0','pending',?)").bind('a',await hash(canonical(abandoned)),run.id,Date.now()-31000).run();
 assert.equal((await req('/jev','POST',{runId:run.id,request:abandoned})).data.source,'cache');
});
test('concurrent saves have one winner and a failed run update rolls back the slot',async()=>{
 const fixture=JSON.parse(await readFile('../docs/evidence/equipment-ui/cross-platform-snapshot.json','utf8'));
 fixture.payload.scenario=0;fixture.payload.campaign={};fixture.sha256=await hash(canonical(fixture.payload));
 const run=(await req('/runs','POST',{})).data;
 const value={runId:run.id,revision:0,snapshot:JSON.stringify(fixture)};
 const replies=await Promise.all([req('/saves/8','PUT',value),req('/saves/8','PUT',value)]);
 assert.deepEqual(replies.map(r=>r.status).sort(),[200,409]);
 assert.equal((await env.DB.prepare('SELECT revision FROM runs WHERE id=?').bind(run.id).first<any>())!.revision,1);
 await env.DB.prepare("CREATE TRIGGER test_run_failure BEFORE UPDATE ON runs BEGIN SELECT RAISE(ABORT,'test failure'); END").run();
 try{
  assert.equal((await req('/saves/9','PUT',value)).status,500);
  assert.equal((await req('/saves/9')).status,404);
 }finally{await env.DB.prepare('DROP TRIGGER test_run_failure').run();}
 assert.equal((await req('/saves/9','PUT',value)).status,200);
});

const special='/special/hulao';
async function specialFixture(key='0:127:-1'){
 const all=JSON.parse(await readFile('../tests/fixtures/special-hulao-starts.json','utf8'));return all[key];
}
async function specialStart(who='a',key='0:127:-1',id=crypto.randomUUID()){
 return req(special+'/matches','POST',{id,snapshot:JSON.stringify(await specialFixture(key))},who);
}
async function specialEnd(key='0:127:-1',outcome='victory'){
 const e=await specialFixture(key),p=e.payload,s=p.state;s.turn=4;s.phase=outcome;s.message='isolated outcome fixture';s.selected=-1;s.undo=null;p.progress.victory=outcome==='victory';
 for(const id of outcome==='victory'?[p.duel_enemy]:[p.duel_hero,...[p.duel_ally,p.duel_ally2,p.duel_ally3].filter(id=>id>=0)]){s.units[id].battle.hp=0;s.units[id].battle.lifecycle=3;}
 e.sha256=await hash(canonical(p));return JSON.stringify(e);
}
test('special battle requires verified login and profile; exact starts, ownership and source checks',async()=>{
 for(const who of ['', 'unverified','expired'])for(const path of [special+'/leaderboard',special+'/matches'])assert.equal((await req(path,path.endsWith('matches')?'POST':'GET',path.endsWith('matches')?{}:undefined,who)).status,401);
 const switched=await worker.fetch(new Request(origin+'/api'+special+'/leaderboard',{headers:{Cookie:cookie('a'),'X-Special-Account':'b'}}),env);assert.equal(switched.status,401);
 const id=crypto.randomUUID(),first=await specialStart('a','0:127:-1',id);assert.equal(first.status,201,JSON.stringify(first));
 assert.equal((await specialStart('a','0:127:-1',id)).status,200);
 assert.equal((await specialStart('a','0:119:6',id)).status,422);
 assert.equal((await req(special+'/matches/'+id,'GET',undefined,'b')).status,404);
 assert.equal((await req(special+'/matches/'+id+'/result','POST',{snapshot:await specialEnd()},'b')).status,404);
 assert.equal((await req(special+'/matches','POST',{id:crypto.randomUUID(),snapshot:await specialEnd()})).status,422);
 assert.equal((await req(special+'/matches/'+id+'/result','POST',{snapshot:await specialEnd()},'a','https://evil.invalid')).status,403);
 const broken=await specialFixture();broken.payload.state.units['0'].battle.hp=500;broken.sha256=await hash(canonical(broken.payload));
 assert.equal((await req(special+'/matches','POST',{id:crypto.randomUUID(),snapshot:JSON.stringify(broken)})).status,422);
});
test('all 72 current and historical C++ starts register; terminal validation, single survivor win, team defeat and timeout',async()=>{
 const fixtures=JSON.parse(await readFile('../tests/fixtures/special-hulao-starts.json','utf8'));
 for(const [key,value] of Object.entries(fixtures)){
  // Each allowlist fixture is an independent result-validation case. Reset only
  // this test user's local result quota; production rate limits remain unchanged.
  await env.DB.prepare("DELETE FROM budgets WHERE bucket='special-result:a'").run();
  const id=crypto.randomUUID();assert.equal((await req(special+'/matches','POST',{id,snapshot:JSON.stringify(value)})).status,201,key);
  const initial=value as any;assert.equal((await req(special+'/matches/'+id+'/result','POST',{snapshot:JSON.stringify(initial)})).status,422);
  const end=JSON.parse(await specialEnd(key));
  if(end.payload.duel_ally>=0){const dead=end.payload.state.units[end.payload.duel_hero].battle;dead.hp=0;dead.lifecycle=3;end.sha256=await hash(canonical(end.payload));}
  const win=await req(special+'/matches/'+id+'/result','POST',{snapshot:JSON.stringify(end)});assert.equal(win.status,200,JSON.stringify(win));assert.equal(win.data.outcome,'victory');
 }
 const start=(await specialStart('a','0:119:11')).data;
 const bad=JSON.parse(await specialEnd('0:119:11','defeat'));bad.payload.state.units['11'].battle.hp=50;bad.payload.state.units['11'].battle.lifecycle=2;bad.sha256=await hash(canonical(bad.payload));
 assert.equal((await req(special+'/matches/'+start.id+'/result','POST',{snapshot:JSON.stringify(bad)})).status,422);
 bad.payload.state.turn_limit_reached=true;bad.payload.state.turn=19;bad.sha256=await hash(canonical(bad.payload));
 assert.equal((await req(special+'/matches/'+start.id+'/result','POST',{snapshot:JSON.stringify(bad)})).status,422);
 bad.payload.state.turn=20;bad.sha256=await hash(canonical(bad.payload));assert.equal((await req(special+'/matches/'+start.id+'/result','POST',{snapshot:JSON.stringify(bad)})).status,200);
 const team=(await specialStart('a','1:119:0')).data;assert.equal((await req(special+'/matches/'+team.id+'/result','POST',{snapshot:await specialEnd('1:119:0','defeat')})).status,200);
});
test('concurrent duplicate results count once and later saves cannot change a recorded result',async()=>{
 const match=(await specialStart()).data,snapshot=await specialEnd();
 const before=await env.DB.prepare("SELECT COUNT(*) AS n FROM special_hulao_matches WHERE user_id='a' AND outcome='victory'").first<any>();
 const results=await Promise.all(Array.from({length:6},()=>req(special+'/matches/'+match.id+'/result','POST',{snapshot})));
 assert.ok(results.every(r=>r.status===200));assert.equal(results.filter(r=>r.data.counted).length,1);
 const after=await env.DB.prepare("SELECT COUNT(*) AS n FROM special_hulao_matches WHERE user_id='a' AND outcome='victory'").first<any>();assert.equal(after.n,before.n+1);
 const changed=await req(special+'/matches/'+match.id+'/result','POST',{snapshot:await specialEnd('0:127:-1','defeat')});assert.equal(changed.data.counted,false);assert.equal(changed.data.outcome,'victory');
});
test('three friendly matches require the last survivor to fall; third-ally wins and immutable lineup',async()=>{
 const key='0:119:1:11',fixture=await specialFixture(key);
 const start=(await specialStart('a',key)).data;assert.equal(start.ally,1);assert.equal(start.ally2,11);
 // Two casualties are not defeat while the second random ally remains alive.
 const end=JSON.parse(await specialEnd(key,'defeat'));
 end.payload.state.units['11'].battle.hp=30;end.payload.state.units['11'].battle.lifecycle=2;end.sha256=await hash(canonical(end.payload));
 assert.equal((await req(special+'/matches/'+start.id+'/result','POST',{snapshot:JSON.stringify(end)})).status,422);
 end.payload.state.phase='victory';end.payload.progress.victory=true;end.payload.state.units['119'].battle.hp=0;end.payload.state.units['119'].battle.lifecycle=3;end.sha256=await hash(canonical(end.payload));
 const victory=await req(special+'/matches/'+start.id+'/result','POST',{snapshot:JSON.stringify(end)});assert.equal(victory.status,200);assert.equal(victory.data.outcome,'victory');assert.equal(victory.data.counted,true);
 assert.equal((await req(special+'/matches/'+start.id+'/result','POST',{snapshot:JSON.stringify(end)})).data.counted,false);
 const lost=(await specialStart('a',key)).data;assert.equal((await req(special+'/matches/'+lost.id+'/result','POST',{snapshot:await specialEnd(key,'defeat')})).status,200);
 for(const bad of [-1,0,1,119]){const broken=structuredClone(fixture);broken.payload.duel_ally2=bad;broken.sha256=await hash(canonical(broken.payload));assert.equal((await req(special+'/matches','POST',{id:crypto.randomUUID(),snapshot:JSON.stringify(broken)})).status,422);}
 // Current and older clients may both finish their existing registered lineups.
 const old=(await specialStart('a','0:119:1')).data;assert.equal(old.ally2,-1);
 assert.equal((await req(special+'/matches/'+old.id+'/result','POST',{snapshot:await specialEnd(key)})).status,422);
 assert.equal((await req(special+'/matches/'+old.id+'/result','POST',{snapshot:await specialEnd('0:119:1')})).status,200);
 assert.equal((await req(special+'/matches/'+lost.id+'/result','POST',{snapshot:await specialEnd('0:119:1')})).status,422);
});
test('three starter beans register for either enemy and cannot change an existing match supply policy',async()=>{
 for(const key of ['0:127:-1','0:119:1:11']){
  const current=await specialFixture(key+':beans3');assert.equal(current.payload.duel_starting_beans,3);assert.equal(current.payload.benefits.inventory['87'],3);
  const match=(await specialStart('a',key+':beans3')).data;
  assert.equal((await req(special+'/matches/'+match.id+'/result','POST',{snapshot:await specialEnd(key)})).status,422);
  assert.equal((await req(special+'/matches/'+match.id+'/result','POST',{snapshot:await specialEnd(key+':beans3')})).status,200);
  const old=(await specialStart('a',key)).data;
  assert.equal((await req(special+'/matches/'+old.id+'/result','POST',{snapshot:await specialEnd(key+':beans3')})).status,422);
  const wrong=structuredClone(current);wrong.payload.benefits.inventory['87']=4;wrong.sha256=await hash(canonical(wrong.payload));
  assert.equal((await req(special+'/matches','POST',{id:crypto.randomUUID(),snapshot:JSON.stringify(wrong)})).status,422);
 }
});
test('special ranking uses wins only, zero-win losses descending, shared ties, private fields and pagination',async()=>{
 // Isolated ranking fixtures; never run against the production database.
 const names=[['rank-a',3,0],['rank-b',3,9],['rank-c',1,100],['rank-d',0,6],['rank-e',0,2],['rank-f',0,6]] as const;
 const now=Date.now();
 for(const [id,wins,losses] of names){
  await env.DB.prepare('INSERT INTO user(id,name,email,emailVerified,createdAt,updatedAt) VALUES(?,?,?,1,?,?)').bind(id,id,id+'@example.invalid',now,now).run();
  await env.DB.prepare('INSERT INTO profiles(user_id,username,username_key,created_at) VALUES(?,?,?,?)').bind(id,id,id,now).run();
  for(let n=0;n<wins+losses;n++)await env.DB.prepare('INSERT INTO special_hulao_matches(id,user_id,hero,enemy,ally,initial_hash,created_at,outcome,finished_at,final_hash,final_turn) VALUES(?,?,0,127,-1,?,?,?, ?,?,4)').bind(crypto.randomUUID(),id,'start',now,n<wins?'victory':'defeat',now,'end').run();
 }
 const ranking=(await req(special+'/leaderboard')).data,rows=ranking.entries.filter((e:any)=>e.username.startsWith('rank-'));
 assert.deepEqual(rows.map((r:any)=>r.username),['rank-a','rank-b','rank-c','rank-d','rank-f','rank-e']);
 assert.equal(rows[0].rank,rows[1].rank);assert.equal(rows[3].rank,rows[4].rank);
 assert.ok(rows[2].rank<rows[3].rank);assert.equal(rows[1].wins,3);assert.equal(rows[1].losses,9);
 for(const row of ranking.entries)assert.deepEqual(Object.keys(row).sort(),['isYou','losses','rank','username','wins']);
 assert.ok(ranking.you.wins>0);assert.deepEqual((await req(special+'/leaderboard','GET',undefined,'b')).data.you,{wins:0,losses:0,rank:null});
 for(let n=0;n<55;n++){
  const id='page-'+String(n).padStart(2,'0');await env.DB.prepare('INSERT INTO user(id,name,email,emailVerified,createdAt,updatedAt) VALUES(?,?,?,1,?,?)').bind(id,id,id+'@example.invalid',now,now).run();
  await env.DB.prepare('INSERT INTO profiles(user_id,username,username_key,created_at) VALUES(?,?,?,?)').bind(id,id,id,now).run();
  await env.DB.prepare("INSERT INTO special_hulao_matches(id,user_id,hero,enemy,ally,initial_hash,created_at,outcome,finished_at,final_hash,final_turn) VALUES(?,?,0,127,-1,'start',?,'defeat',?,'end',1)").bind(crypto.randomUUID(),id,now,now).run();
 }
 const page0=(await req(special+'/leaderboard?page=0')).data,page1=(await req(special+'/leaderboard?page=1')).data;
 assert.equal(page0.entries.length,50);assert.equal(page0.hasMore,true);assert.equal(page1.hasMore,false);assert.ok(page1.entries.length>0);
 assert.equal(new Set([...page0.entries,...page1.entries].map((r:any)=>r.username)).size,page0.entries.length+page1.entries.length);
});
test('two/four-friendly squads bind all teammates; only the last survivor falling ends battle',async()=>{
 for(const key of ['0:127:6:beans3:squad','0:119:6:1:11:beans3:squad']){
  const fixture=await specialFixture(key),p=fixture.payload;
  const start=(await specialStart('a',key)).data;assert.equal(start.ally,p.duel_ally);assert.equal(start.ally3,p.duel_ally3??-1);
  const survivor=p.duel_ally3??p.duel_ally;
  const end=JSON.parse(await specialEnd(key,'defeat'));end.payload.state.units[survivor].battle.hp=30;end.payload.state.units[survivor].battle.lifecycle=2;end.sha256=await hash(canonical(end.payload));
  assert.equal((await req(special+'/matches/'+start.id+'/result','POST',{snapshot:JSON.stringify(end)})).status,422);
  end.payload.state.phase='victory';end.payload.progress.victory=true;end.payload.state.units[p.duel_enemy].battle.hp=0;end.payload.state.units[p.duel_enemy].battle.lifecycle=3;end.sha256=await hash(canonical(end.payload));
  assert.equal((await req(special+'/matches/'+start.id+'/result','POST',{snapshot:JSON.stringify(end)})).status,200);
  const lost=(await specialStart('a',key)).data;assert.equal((await req(special+'/matches/'+lost.id+'/result','POST',{snapshot:await specialEnd(key,'defeat')})).status,200);
  const old=(await specialStart('a','0:127:-1')).data;assert.equal((await req(special+'/matches/'+old.id+'/result','POST',{snapshot:JSON.stringify(end)})).status,422);
  for(const bad of [-1,p.duel_hero,p.duel_enemy]){const broken=structuredClone(fixture);broken.payload[p.duel_enemy===119?'duel_ally3':'duel_ally']=bad;broken.sha256=await hash(canonical(broken.payload));assert.equal((await req(special+'/matches','POST',{id:crypto.randomUUID(),snapshot:JSON.stringify(broken)})).status,422);}
 }
});
test('new Dong Zhuo matches start with zero beans; existing three-bean matches keep their policy',async()=>{
 const key='0:127:6:beans0:squad',oldKey='0:127:6:beans3:squad';
 const fixture=await specialFixture(key);assert.equal(fixture.payload.schema,19);assert.equal(fixture.payload.duel_starting_beans,0);assert.equal(fixture.payload.benefits.inventory['87'],0);
 const current=(await specialStart('a',key)).data,old=(await specialStart('a',oldKey)).data;
 assert.equal((await req(special+'/matches/'+current.id+'/result','POST',{snapshot:await specialEnd(oldKey)})).status,422);
 assert.equal((await req(special+'/matches/'+old.id+'/result','POST',{snapshot:await specialEnd(key)})).status,422);
 assert.equal((await req(special+'/matches/'+current.id+'/result','POST',{snapshot:await specialEnd(key)})).status,200);
 assert.equal((await req(special+'/matches/'+old.id+'/result','POST',{snapshot:await specialEnd(oldKey)})).status,200);
 for(const change of [(p:any)=>{p.duel_starting_beans=3;},(p:any)=>{p.benefits.inventory['87']=3;},(p:any)=>{p.duel_enemy=119;}]){const bad=structuredClone(fixture);change(bad.payload);bad.sha256=await hash(canonical(bad.payload));assert.equal((await req(special+'/matches','POST',{id:crypto.randomUUID(),snapshot:JSON.stringify(bad)})).status,422);}
});
