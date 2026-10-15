// Local UI fixture only. Never the production Worker entrypoint.
import worker from '../src/worker';
import {Env} from '../src/types';
export default {async fetch(request:Request,env:Env){
 const url=new URL(request.url);
 if(url.hostname==='127.0.0.1'&&url.pathname==='/__test/login'){
  const now=Date.now(),id=url.searchParams.has('first')?'local-ui-first-'+crypto.randomUUID():'local-ui-only',token=crypto.randomUUID();
  await env.DB.prepare('INSERT OR IGNORE INTO user(id,name,email,emailVerified,createdAt,updatedAt) VALUES(?,?,?,1,?,?)').bind(id,'本机测试',id+'@example.invalid',now,now).run();
  await env.DB.prepare('INSERT INTO session(id,token,userId,expiresAt,createdAt,updatedAt) VALUES(?,?,?,?,?,?)').bind(crypto.randomUUID(),token,id,now+3600000,now,now).run();
  const key=await crypto.subtle.importKey('raw',new TextEncoder().encode(env.BETTER_AUTH_SECRET),{name:'HMAC',hash:'SHA-256'},false,['sign']);
  const sig=btoa(String.fromCharCode(...new Uint8Array(await crypto.subtle.sign('HMAC',key,new TextEncoder().encode(token)))));
  return new Response(null,{status:302,headers:{Location:url.searchParams.has('special')?'/special/hulao/':'/', 'Set-Cookie':`caocao.session_token=${encodeURIComponent(token+'.'+sig)}; Path=/; HttpOnly; SameSite=Lax; Max-Age=3600`}});
 }
 return worker.fetch(request,env);
}};
