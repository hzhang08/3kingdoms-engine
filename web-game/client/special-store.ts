import type {SpecialSaved} from './special-results';
export class SpecialStore{
 private db:Promise<IDBDatabase>;
 constructor(userId:string){
  this.db=new Promise((resolve,reject)=>{const r=indexedDB.open('caocao-special-hulao-user-'+userId,1);r.onupgradeneeded=()=>r.result.createObjectStore('slots');r.onsuccess=()=>resolve(r.result);r.onerror=()=>reject(r.error);});this.db.catch(()=>{});
 }
 async get<T=SpecialSaved>(key:IDBValidKey):Promise<T|undefined>{return this.operation('readonly',s=>s.get(key));}
 async put(key:IDBValidKey,value:unknown){await this.operation('readwrite',s=>s.put(value,key));}
 async remove(key:IDBValidKey){await this.operation('readwrite',s=>s.delete(key));}
 async keys():Promise<IDBValidKey[]>{return this.operation('readonly',s=>s.getAllKeys());}
 private async operation<T>(mode:IDBTransactionMode,work:(s:IDBObjectStore)=>IDBRequest<T>):Promise<T>{const db=await this.db;return new Promise((resolve,reject)=>{const tx=db.transaction('slots',mode),r=work(tx.objectStore('slots'));tx.oncomplete=()=>resolve(r.result);tx.onerror=()=>reject(tx.error);tx.onabort=()=>reject(tx.error??Error('Local storage interrupted'));});}
}
