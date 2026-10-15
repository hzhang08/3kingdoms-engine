export interface Env {
 DB: D1Database; ASSETS: Fetcher; APP_ORIGIN: string;
 BETTER_AUTH_SECRET: string; GOOGLE_CLIENT_ID: string; GOOGLE_CLIENT_SECRET: string;
 JEV_API_KEY?: string; JEV_DAILY_USER_LIMIT: string; JEV_DAILY_GLOBAL_LIMIT: string;
}
export class HttpError extends Error { constructor(public status: number, message: string) { super(message); } }
export const fail = (status: number, message: string): never => { throw new HttpError(status,message); };
export const json = (value: unknown, status = 200) => Response.json(value,{status,headers:{'Cache-Control':'no-store','X-Content-Type-Options':'nosniff'}});
export async function body(request: Request, limit = 220000): Promise<any> {
 if(!request.headers.get('content-type')?.startsWith('application/json')) fail(415,'需要 JSON 请求。');
 const length = Number(request.headers.get('content-length') || 0); if(length>limit)fail(413,'请求过大。');
 const reader = request.body?.getReader(); if(!reader)fail(400,'请求为空。');
 const chunks: Uint8Array[]=[];let total=0;
 while(true){const {done,value}=await reader!.read();if(done)break;total+=value.length;if(total>limit){await reader!.cancel();fail(413,'请求过大。');}chunks.push(value);}
 const raw=new Uint8Array(total);let pos=0;for(const b of chunks){raw.set(b,pos);pos+=b.length;}
 try{return JSON.parse(new TextDecoder('utf-8',{fatal:true,ignoreBOM:false}).decode(raw));}catch{fail(400,'JSON 无效。');}
}
