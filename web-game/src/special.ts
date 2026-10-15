import {Env,body,fail,json} from './types';
import {canonical,hash} from './snapshots';
import {budget} from './jev';
import release from './special-release.json';

type Match={id:string;user_id:string;hero:number;enemy:number;ally:number;ally2:number;ally3:number;initial_hash:string;created_at:number;outcome:'victory'|'defeat'|null;finished_at:number|null;final_hash:string|null;final_turn:number|null};
const invalid=()=>fail(422,'特别战场记录无效。');
const integer=(v:unknown,lo:number,hi:number)=>Number.isSafeInteger(v)&&Number(v)>=lo&&Number(v)<=hi;
const publicMatch=(m:Match)=>({id:m.id,hero:m.hero,enemy:m.enemy,ally:m.ally,ally2:m.ally2,ally3:m.ally3,outcome:m.outcome,finishedAt:m.finished_at});
async function snapshot(raw:unknown){
 if(typeof raw!=='string'||new TextEncoder().encode(raw).length>200000)invalid();
 let e:any;try{e=JSON.parse(raw as string);}catch{invalid();}
 if(!e?.payload||e.sha256!==await hash(canonical(e.payload)))invalid();
 const p=e.payload,base:any=release.template;
 for(const k of ['mode','scenario','ruleset','duel_map','campaign','conversations','dialogue',...Object.keys(base).filter(k=>k.endsWith('_manifest'))])if(canonical(p[k]??null)!==canonical(base[k]??null))invalid();
 const hero=p.duel_hero,enemy=p.duel_enemy,ally=p.duel_ally??-1,ally2=p.duel_ally2??-1,ally3=p.duel_ally3??-1,noStarter=p.schema===19,squads=noStarter||p.schema===18,supplies=!noStarter&&(squads||p.schema===17),trio=p.schema===16||(supplies&&ally2>=0);
 if(![0,6,1,11].includes(hero)||![119,127].includes(enemy))invalid();
 if(squads){
  const friendlies=[hero,ally,...(enemy===119?[ally2,ally3]:[])];
  if(p.engine!==(noStarter?'prototype-0.19-duel':'prototype-0.18-duel')||p.layout!=='hulao-duel-squads-v1'||friendlies.some(id=>![0,6,1,11].includes(id))||new Set(friendlies).size!==friendlies.length||(enemy===127&&(p.duel_ally2!==undefined||p.duel_ally3!==undefined)))invalid();
 }else if(enemy===119?(![0,6,1,11].includes(ally)||ally===hero||p.schema!==(supplies?17:trio?16:15)||p.engine!==(supplies?'prototype-0.17-duel':trio?'prototype-0.16-duel':'prototype-0.15-duel')||p.layout!==(trio?'hulao-duel-trio-v1':'hulao-duel-team-v1')):(ally!==-1||p.schema!==(supplies?17:13)||p.engine!==(supplies?'prototype-0.17-duel':'prototype-0.13-duel')||p.layout!=='hulao-duel-heroes-v1'))invalid();
 if(noStarter){if(enemy!==127||p.duel_starting_beans!==0)invalid();}
 else if(supplies?p.duel_starting_beans!==3:p.duel_starting_beans!==undefined)invalid();
 if(trio?(![0,6,1,11].includes(ally2)||ally2===hero||ally2===ally):p.duel_ally2!==undefined)invalid();
 if(!squads&&p.duel_ally3!==undefined)invalid();
 const key=`${hero}:${enemy}:${ally}`+(ally2>=0?':'+ally2:'')+(ally3>=0?':'+ally3:'')+(noStarter?':beans0':supplies?':beans3':'')+(squads?':squad':'');
 return {p,key,hash:e.sha256 as string,hero:hero as number,enemy:enemy as number,ally:ally as number,ally2:ally2 as number,ally3:ally3 as number};
}
async function owned(env:Env,userId:string,id:string){
 const row=await env.DB.prepare('SELECT * FROM special_hulao_matches WHERE id=? AND user_id=?').bind(id,userId).first<Match>();
 if(!row)fail(404,'没有找到这局特别战场。');return row!;
}
async function start(request:Request,env:Env,userId:string){
 const b=await body(request);if(typeof b.id!=='string'||!/^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/.test(b.id))invalid();
 const s=await snapshot(b.snapshot);const starts=release.starts as Record<string,string>;
 if(starts[s.key]!==s.hash)fail(422,'请从选择武将开始新的排名对局。');
 const existing=await env.DB.prepare('SELECT * FROM special_hulao_matches WHERE id=? AND user_id=?').bind(b.id,userId).first<Match>();
 if(existing){if(existing.initial_hash!==s.hash)invalid();return json(publicMatch(existing));}
 if(!await budget(env,'special-start:'+userId,120,3600))fail(429,'开启对局过于频繁，请稍后重试。');
 await env.DB.prepare('INSERT OR IGNORE INTO special_hulao_matches(id,user_id,hero,enemy,ally,ally2,ally3,initial_hash,created_at) VALUES(?,?,?,?,?,?,?,?,?)').bind(b.id,userId,s.hero,s.enemy,s.ally,s.ally2,s.ally3,s.hash,Date.now()).run();
 const row=await owned(env,userId,b.id);if(row.initial_hash!==s.hash)invalid();return json(publicMatch(row),201);
}
async function finish(request:Request,env:Env,userId:string,id:string){
 const m=await owned(env,userId,id);const s=await snapshot((await body(request)).snapshot),p=s.p,state=p.state;
 if((release.starts as Record<string,string>)[s.key]!==m.initial_hash)invalid();
 if(m.hero!==s.hero||m.enemy!==s.enemy||m.ally!==s.ally||m.ally2!==s.ally2||m.ally3!==s.ally3||!state||!integer(state.turn,1,20)||!integer(state.rng,0,0xffffffff)||!['victory','defeat'].includes(state.phase)||state.selected!==-1||state.undo!==null)invalid();
 const units=state.units,reference:any=release.template.state.units;
 if(!units||canonical(Object.keys(units).sort())!==canonical(Object.keys(reference).sort()))invalid();
 const included=[s.hero,s.enemy,...[s.ally,s.ally2,s.ally3].filter(id=>id>=0)];
 for(const id of Object.keys(reference)){
  const u=units[id],r=reference[id],b=u?.battle;
  if(!b||b.id!==r.battle.id||b.side!==r.battle.side||b.slot!==r.battle.slot||u.name!==r.name||!integer(u.max_hp,1,500)||!integer(b.hp,0,u.max_hp)||!integer(b.x,0,19)||!integer(b.y,0,19)||!integer(b.mp,0,500)||!integer(u.record?.level,1,99)||u.record?.id!==r.record.id||u.record?.class!==r.record.class||canonical(u.record?.base_stats??null)!==canonical(r.record.base_stats))invalid();
  if(included.includes(b.id)?b.lifecycle!==(b.hp>0?2:3):b.lifecycle!==3)invalid();
 }
 if(typeof state.turn_limit_reached!=='boolean'||!Array.isArray(p.npc_order)||p.npc_order.some((id:number)=>id!==s.enemy)||p.npc_order.length>1||!integer(state.enemy_cursor,0,p.npc_order.length))invalid();
 if(state.turn_limit_reached&&(state.turn!==20||state.enemy_cursor!==p.npc_order.length))invalid();
 if(canonical(p.progress)!==canonical({...release.template.progress,victory:state.phase==='victory'}))invalid();
 const lost=[s.hero,s.ally,s.ally2,s.ally3].filter(id=>id>=0).every(id=>units[id].battle.hp===0)||state.turn_limit_reached;
 const won=!lost&&units[s.enemy].battle.hp===0;
 if((state.phase==='victory')!==won||(state.phase==='defeat')!==lost||p.progress?.victory!==won)invalid();
 // First terminal result wins. One atomic row update, no separately incremented counters.
 const change=await env.DB.prepare('UPDATE special_hulao_matches SET outcome=?,finished_at=?,final_hash=?,final_turn=? WHERE id=? AND user_id=? AND outcome IS NULL').bind(state.phase,Date.now(),s.hash,state.turn,id,userId).run();
 const recorded=await owned(env,userId,id);return json({...publicMatch(recorded),counted:change.meta.changes===1});
}
async function leaderboard(env:Env,userId:string,page:number){
 const cte=`WITH totals AS (
 SELECT p.user_id,p.username,p.username_key,SUM(m.outcome='victory') AS wins,SUM(m.outcome='defeat') AS losses
 FROM special_hulao_matches m JOIN profiles p ON p.user_id=m.user_id WHERE m.outcome IS NOT NULL GROUP BY p.user_id
 ), ranked AS (
 SELECT *,RANK() OVER(ORDER BY (wins>0) DESC,CASE WHEN wins>0 THEN wins ELSE losses END DESC) AS rank FROM totals
 )`;
 const fields='username,wins,losses,rank,(user_id=?) AS isYou';
 const [{results},mine]=await Promise.all([
  env.DB.prepare(cte+` SELECT ${fields} FROM ranked ORDER BY rank,username_key,user_id LIMIT 51 OFFSET ?`).bind(userId,page*50).all(),
  env.DB.prepare(cte+` SELECT ${fields} FROM ranked WHERE user_id=?`).bind(userId,userId).first()
 ]);
 return json({page,hasMore:results.length>50,entries:results.slice(0,50),you:mine??{wins:0,losses:0,rank:null},ranking:'wins-first; zero-wins-by-losses; shared-ranks-for-ties'});
}
export async function specialRoute(request:Request,env:Env,userId:string,url:URL):Promise<Response>{
 const path=url.pathname,method=request.method;
 if(path==='/api/special/hulao/leaderboard'&&method==='GET')return leaderboard(env,userId,Math.min(200,Math.max(0,Number(url.searchParams.get('page'))||0)|0));
 if(path==='/api/special/hulao/matches'&&method==='POST')return start(request,env,userId);
 const match=path.match(/^\/api\/special\/hulao\/matches\/([0-9a-f-]{36})(\/result)?$/);
 if(match){if(!match[2]&&method==='GET')return json(publicMatch(await owned(env,userId,match[1])));if(match[2]&&method==='POST'){
  if(!await budget(env,'special-result:'+userId,120,60))fail(429,'提交过于频繁，请稍后重试。');return finish(request,env,userId,match[1]);
 }}
 return fail(404,'没有找到此接口。');
}
