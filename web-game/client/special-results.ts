// Account-scoped local results survive reloads and offline/repeated submissions.
export type RankedMatch={id:string;initial?:string};
export type SpecialSaved={snapshot:string;at:number;turn:number;phase:string;rosterVersion?:string;match?:RankedMatch};
export type PendingResult={id:string;snapshot:string};
export function terminalResult(saved:SpecialSaved):PendingResult|undefined{
 if(!saved.match||saved.match.initial||!['victory','defeat'].includes(saved.phase))return;
 return {id:saved.match.id,snapshot:saved.snapshot};
}
