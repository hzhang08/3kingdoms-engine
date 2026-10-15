import { betterAuth } from 'better-auth';
import { APIError } from 'better-auth/api';
import { Env, fail } from './types';
export function authFor(env: Env) {
 if(!env.BETTER_AUTH_SECRET || env.BETTER_AUTH_SECRET.length<32 || !env.GOOGLE_CLIENT_ID || !env.GOOGLE_CLIENT_SECRET)fail(503,'Google 登录尚未配置完成。');
 return betterAuth({
  database:env.DB, secret:env.BETTER_AUTH_SECRET, baseURL:env.APP_ORIGIN,
  trustedOrigins:[env.APP_ORIGIN], emailAndPassword:{enabled:false},
  socialProviders:{google:{clientId:env.GOOGLE_CLIENT_ID,clientSecret:env.GOOGLE_CLIENT_SECRET,prompt:'select_account'}},
  databaseHooks:{user:{create:{before:async user=>{if(!user.emailVerified)throw new APIError('FORBIDDEN',{message:'需要已验证的 Google 账号。'});return {data:user};}}}},
  session:{expiresIn:7*86400,updateAge:86400},
  advanced:{cookiePrefix:'caocao',ipAddress:{ipAddressHeaders:['cf-connecting-ip']}},
  rateLimit:{enabled:true,storage:'database'},
  onAPIError:{errorURL:env.APP_ORIGIN+'/?login=failed'}
 });
}
export async function currentUser(request:Request,env:Env){
 if(!request.headers.get('cookie'))return null;
 const s=await authFor(env).api.getSession({headers:request.headers});
 return s?.user.emailVerified?s.user:null;
}
export async function requireUser(request:Request,env:Env,profile=true){
 const user=await currentUser(request,env);if(!user)fail(401,'请先使用 Google 登录。');
 const p=await env.DB.prepare('SELECT username FROM profiles WHERE user_id=?').bind(user!.id).first<{username:string}>();
 if(profile&&!p)fail(428,'请先设置用户名。');
 return {id:user!.id,username:p?.username??null};
}
