import { authFor, currentUser, requireUser } from './auth';
import { Env, HttpError, body, fail, json } from './types';
import { BATTLES, username, validateSnapshot } from './snapshots';
import { budget, decide } from './jev';
import release from './release.json';
import {specialRoute} from './special';

function uuid(v:unknown):string{if(typeof v!=='string'||!/^[a-f0-9-]{36}$/.test(v))fail(400,'游戏编号无效。');return v as string;}
function slot(v:string){const n=Number(v);if(!/^\d{1,2}$/.test(v)||n<0||n>10)fail(400,'存档位无效。');return n;}
const metadata='slot,run_id,scenario,turn,phase,cleared,revision,saved_at,state_hash';
async function owner(env:Env,userId:string,runId:string){
 const r=await env.DB.prepare('SELECT * FROM runs WHERE id=? AND user_id=?').bind(runId,userId).first<any>();if(!r)fail(404,'没有找到这局游戏。');return r;
}
async function save(request:Request,env:Env,userId:string,n:number){
 const b=await body(request);const run=await owner(env,userId,uuid(b.runId));
 if(!Number.isSafeInteger(b.revision)||b.revision<0)fail(400,'请刷新存档列表后重试。');
 const s=await validateSnapshot(b.snapshot),p=s.payload,m=s.summary;
 for(const [key,value] of Object.entries(release.identity))if(p[key]!==value)fail(422,'存档资源版本不兼容，请保留备份。');
 if(m.scenario>run.cleared||m.cleared>run.cleared+1)fail(422,'请先完成前一战并同步进度。');
 const old=await env.DB.prepare('SELECT revision FROM saves WHERE user_id=? AND slot=?').bind(userId,n).first<{revision:number}>();
 if((old?.revision??0)!==b.revision)fail(409,'这个存档已在其他窗口更新。原存档已保留，请刷新后选择存档。');
 const now=Date.now();
 // Single conditional upsert provides optimistic locking even for the first save.
 const write=env.DB.prepare(`INSERT INTO saves(user_id,slot,run_id,snapshot,state_hash,scenario,turn,phase,cleared,revision,saved_at)
 SELECT ?,?,?,?,?,?,?,?,?,1,? WHERE ?=0 OR EXISTS(SELECT 1 FROM saves WHERE user_id=? AND slot=? AND revision=?)
 ON CONFLICT(user_id,slot) DO UPDATE SET run_id=excluded.run_id,snapshot=excluded.snapshot,state_hash=excluded.state_hash,scenario=excluded.scenario,turn=excluded.turn,phase=excluded.phase,cleared=excluded.cleared,revision=saves.revision+1,saved_at=excluded.saved_at WHERE saves.revision=? RETURNING revision`).bind(userId,n,run.id,s.raw,s.hash,m.scenario,m.turn,m.phase,m.cleared,now,b.revision,userId,n,b.revision,b.revision);
 // High-water ranking does not regress when an older save is resumed.
 const advance=env.DB.prepare(`UPDATE runs SET updated_at=?,checkpoint=?,revision=revision+1,scenario=?,turn=?,phase=?,
 cleared=MAX(cleared,?),battle_progress=CASE WHEN ?>cleared OR (?=cleared AND ?>battle_progress) THEN ? ELSE battle_progress END,
 completed_at=CASE WHEN ? THEN COALESCE(completed_at,?) ELSE completed_at END WHERE id=? AND user_id=? AND changes()=1`).bind(now,s.raw,m.scenario,m.turn,m.phase,m.cleared,m.cleared,m.cleared,m.battleProgress,m.battleProgress,m.completed?1:0,now,run.id,userId);
 // D1 batch is one transaction. A failed ranking write rolls back the save;
 // changes() prevents a losing CAS request from advancing the run.
 const [written]=await env.DB.batch<{revision:number}>([write,advance]);
 const wrote=written.results[0];
 if(!wrote)fail(409,'存档发生冲突，未覆盖其他窗口的进度。');
 return json({slot:n,revision:wrote.revision,saved_at:now,...m,state_hash:s.hash});
}
async function leaderboard(env:Env,kind:string,page:number){
 const completed=kind==='completed',offset=page*50;
 const sql=completed?`WITH best AS (SELECT user_id,MIN(completed_at) AS completed_at FROM runs WHERE completed_at IS NOT NULL GROUP BY user_id)
 SELECT p.username,b.completed_at,4 AS cleared FROM best b JOIN profiles p ON p.user_id=b.user_id ORDER BY b.completed_at,p.username_key LIMIT 51 OFFSET ?`:
 `WITH best AS (SELECT r.*,ROW_NUMBER() OVER(PARTITION BY user_id ORDER BY cleared DESC,battle_progress DESC,updated_at,id) AS rn FROM runs r
 WHERE NOT EXISTS(SELECT 1 FROM runs c WHERE c.user_id=r.user_id AND c.completed_at IS NOT NULL))
 SELECT p.username,b.cleared,b.battle_progress,b.scenario,b.turn,b.phase,b.updated_at FROM best b JOIN profiles p ON p.user_id=b.user_id WHERE rn=1 ORDER BY b.cleared DESC,b.battle_progress DESC,b.updated_at,p.username_key LIMIT 51 OFFSET ?`;
 const {results}=await env.DB.prepare(sql).bind(offset).all();
 return json({kind,page,hasMore:results.length>50,entries:results.slice(0,50).map((r,i)=>({...r,rank:offset+i+1})),battles:BATTLES});
}
async function route(request:Request,env:Env){
 const url=new URL(request.url),path=url.pathname,method=request.method;
 if(!path.startsWith('/api/'))return env.ASSETS.fetch(request);
 if(!['GET','HEAD'].includes(method)&&request.headers.get('origin')!==env.APP_ORIGIN)fail(403,'请求来源无效。');
 if(path==='/api/health')return json({ok:true,version:'1.0.0',authConfigured:!!(env.BETTER_AUTH_SECRET&&env.GOOGLE_CLIENT_ID&&env.GOOGLE_CLIENT_SECRET),jevConfigured:!!env.JEV_API_KEY});
 if(path.startsWith('/api/auth/'))return authFor(env).handler(request);
 if(path==='/api/me'&&method==='GET'){
  const user=await currentUser(request,env);if(!user)return json({user:null});
  const p=await env.DB.prepare('SELECT username FROM profiles WHERE user_id=?').bind(user.id).first<{username:string}>();
  return json({user:{id:user.id,username:p?.username??null},battles:BATTLES,jevConfigured:!!env.JEV_API_KEY});
 }
 const user=await requireUser(request,env,path!=='/api/profile');
 if(path==='/api/profile'&&(method==='POST'||method==='PUT')){
  const {name,key}=username((await body(request,1024)).username);
  if(method==='PUT'){
   if(!await budget(env,'rename:'+user.id,10,86400))fail(429,'今天修改名字过于频繁，请明天再试。');
   try{const updated=await env.DB.prepare('UPDATE profiles SET username=?,username_key=? WHERE user_id=? RETURNING username').bind(name,key,user.id).first();if(!updated)fail(428,'请先设置用户名。');return json(updated);}
   catch(e){if(e instanceof HttpError)throw e;fail(409,'这个用户名已被使用，请换一个。');}
  }
  try{await env.DB.prepare('INSERT INTO profiles(user_id,username,username_key,created_at) VALUES(?,?,?,?)').bind(user.id,name,key,Date.now()).run();}
  catch{const p=await env.DB.prepare('SELECT username FROM profiles WHERE user_id=?').bind(user.id).first();if(p)return json(p);fail(409,'这个用户名已被使用，请换一个。');}
  return json({username:name},201);
 }
 if(path.startsWith('/api/special/hulao/')){if(request.headers.has('X-Special-Account')&&request.headers.get('X-Special-Account')!==user.id)fail(401,'账号已切换，请刷新后重试。');return specialRoute(request,env,user.id,url);}
 if(path==='/api/runs'&&method==='POST'){
  if(!await budget(env,'new-run:'+user.id,20,86400))fail(429,'今天已开启多局游戏，请继续已有存档。');
  const id=crypto.randomUUID(),seed=crypto.getRandomValues(new Uint32Array(1))[0]||1,now=Date.now();
  await env.DB.prepare('INSERT INTO runs(id,user_id,seed,created_at,updated_at) VALUES(?,?,?,?,?)').bind(id,user.id,seed,now,now).run();
  return json({id,seed,created_at:now},201);
 }
 if(path==='/api/saves'&&method==='GET'){
  const {results}=await env.DB.prepare('SELECT '+metadata+' FROM saves WHERE user_id=? ORDER BY slot').bind(user.id).all();return json({saves:results});
 }
 const match=path.match(/^\/api\/saves\/(\d+)$/);
 if(match){const n=slot(match[1]);
  if(method==='GET'){const row=await env.DB.prepare('SELECT '+metadata+',snapshot FROM saves WHERE user_id=? AND slot=?').bind(user.id,n).first();if(!row)fail(404,'这个存档位还没有记录。');return json(row);}
  if(method==='PUT'){if(!await budget(env,'save:'+user.id,60,60))fail(429,'保存过于频繁，请稍后重试。');return save(request,env,user.id,n);}
 }
 if(path==='/api/leaderboard'&&method==='GET')return leaderboard(env,url.searchParams.get('kind')==='completed'?'completed':'progress',Math.min(200,Math.max(0,Number(url.searchParams.get('page'))||0)|0));
 if(path==='/api/jev'&&method==='POST'){
  const b=await body(request,100000);await owner(env,user.id,uuid(b.runId));return json(await decide(env,user.id,b.runId,b.request));
 }
 return fail(404,'没有找到此接口。');
}
export default {async fetch(request:Request,env:Env){
 try{return await route(request,env);}catch(e){if(e instanceof HttpError)return json({error:e.message},e.status);console.error('game_request_failed');return json({error:'服务暂时不可用，请稍后重试。'},500);}
}};
