import {siteText} from './site-language';
export type SpecialUser={id:string;username:string|null};
export type Label=(zh:string,en:string)=>string;
const el=<T extends HTMLElement=HTMLElement>(id:string)=>document.getElementById(id)! as T;
export class SpecialAccount{
 user:SpecialUser|null=null;
 private expectedId='';
 constructor(private label:Label,private blocked:()=>void){}
 async api(path:string,method='GET',value?:unknown):Promise<any>{
  const response=await fetch('/api'+path,{method,credentials:'same-origin',cache:'no-store',headers:{...(value===undefined?{}:{'Content-Type':'application/json'}),...(this.expectedId?{'X-Special-Account':this.expectedId}:{})},body:value===undefined?undefined:JSON.stringify(value),signal:AbortSignal.timeout(20000)});
  const data=await response.json().catch(()=>({error:'Service unavailable. Please try again.'}));
  if(!response.ok){const message=siteText(data.error||'Service unavailable.',document.documentElement.lang==='en'?'en':'zh');if(response.status===401||response.status===428){this.user=null;this.blocked();this.render();el('auth-error').textContent=message;}throw Object.assign(Error(message),{status:response.status});}return data;
 }
 async check(){this.user=(await this.api('/me')).user;if(this.user?.username)this.expectedId=this.user.id;this.render();return this.user?.username?this.user:null;}
 render(){
  el('account-name').textContent=this.user?.username??'';
  el('sign-out').hidden=!this.user;
  const allowed=!!this.user?.username;el('auth-gate').hidden=allowed;
  el('login-title').textContent=this.label('登录后挑战虎牢关','Sign in to challenge Hulao Pass');
  el('login-copy').textContent=this.label('使用 Google 账号登录。排行榜只显示你的用户名、胜场和败场，不显示邮箱。','Sign in with Google. The leaderboard shows your username, wins and losses, never your email.');
  el('google-login').hidden=!!this.user;el('google-login').textContent=this.label('使用 Google 账号登录','Sign in with Google');
  el('username-form').hidden=!this.user||!!this.user.username;
  el('username-label').textContent=this.label('初次相逢，请设置用户名','Choose your username');
  el<HTMLInputElement>('username').placeholder=this.label('2–20 个字，可用汉字、字母、数字、_ 或 -','2–20 letters, numbers, Chinese characters, _ or -');
  el('username-submit').textContent=this.label('确认名字，进入游戏','Save name & enter');
  el('auth-retry').textContent=this.label('重试连接','Retry connection');
  el('sign-out').textContent=this.label('退出','Sign out');
  for(const id of ['music','saves','restart','ranks','fullscreen'])el<HTMLButtonElement>(id).disabled=!allowed;
 }
 bind(){
  el('google-login').onclick=()=>{void this.login().catch(e=>{el('auth-error').textContent=e.message;});};
  el<HTMLFormElement>('username-form').onsubmit=e=>{e.preventDefault();void this.api('/profile','POST',{username:el<HTMLInputElement>('username').value}).then(()=>location.reload()).catch(e=>{el('auth-error').textContent=e.message;});};
  el('auth-retry').onclick=()=>location.reload();
 }
 async login(){
  el('auth-error').textContent=this.label('正在前往 Google……','Opening Google...');
  const data=await this.api('/auth/sign-in/social','POST',{provider:'google',callbackURL:location.origin+'/special/hulao/',errorCallbackURL:location.origin+'/special/hulao/?login=failed'});
  const target=new URL(data.url);if(target.hostname!=='accounts.google.com'||target.protocol!=='https:')throw Error('Invalid Google sign-in URL');location.assign(target.href);
 }
}
