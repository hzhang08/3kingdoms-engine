#!/usr/bin/env python3
"""Logged, advisory Jev requests; credentials stay outside the project.

Uses Vercel's documented evaluation API. No model output is executed or used
as game truth. Review recommendations and validate them with original-code
fixtures. Records include exact submitted context, latency and model response.
"""
import argparse,datetime,json,time,urllib.request,urllib.error
from pathlib import Path

ENDPOINT='https://ai-gateway.vercel.sh/v1/evaluate'

def decide(request_path,record_path,credential_path):
    request=json.loads(Path(request_path).read_text())
    if request.get('model')!='typesafe-ai/jev':raise ValueError('only the authorized Jev model is supported')
    if not isinstance(request.get('questions'),dict) or not request['questions']:raise ValueError('questions required')
    for q in request['questions'].values():
        if q.get('type') not in ('boolean','choice','score'):raise ValueError('unsupported question type')
        if 'options' in q:raise ValueError('the documented field is criteria, not options')
        if q['type']=='choice' and not isinstance(q.get('criteria'),dict):raise ValueError('choice criteria required')
    dest=Path(record_path)
    if dest.exists():raise ValueError('use a fresh record name; earlier decisions are immutable')
    secret=Path(credential_path).read_text().strip().strip('\"\'')
    if not secret or any(x.isspace() for x in secret):raise ValueError('credential must contain one bare key')
    body=json.dumps(request,ensure_ascii=False).encode()
    if secret.encode() in body:raise ValueError('credential found in submitted context')
    started=time.monotonic()
    record={'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'endpoint':ENDPOINT,'role':'advisory; original executable tests decide correctness',
            'request':request,'disposition':'pending review and executable verification'}
    req=urllib.request.Request(ENDPOINT,data=body,method='POST',headers={
        'Authorization':'Bearer '+secret,'Content-Type':'application/json'})
    # Do not redirect an authenticated request to an unreviewed endpoint.
    class NoRedirect(urllib.request.HTTPRedirectHandler):
        def redirect_request(self,*args,**kwargs):return None
    try:
        with urllib.request.build_opener(NoRedirect).open(req,timeout=45) as res:
            raw=res.read(1024*1024).decode().replace(secret,'[REDACTED]')
            record.update(http_status=res.status,response=json.loads(raw))
    except urllib.error.HTTPError as err:
        record.update(http_status=err.code,error='HTTP request failed; response body omitted to protect credentials')
    except (urllib.error.URLError,TimeoutError) as err:
        record.update(error='network request failed: '+type(err).__name__)
    record['elapsed_seconds']=round(time.monotonic()-started,3)
    serialized=json.dumps(record,ensure_ascii=False,indent=2)+'\n'
    if secret in serialized:raise ValueError('refusing to log credential')
    dest.parent.mkdir(parents=True,exist_ok=True)
    with dest.open('x') as stream:stream.write(serialized)
    print(json.dumps({k:v for k,v in record.items() if k not in ('request','endpoint')},ensure_ascii=False,indent=2))
    return record.get('http_status')==200

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('request',type=Path);p.add_argument('record',type=Path)
    p.add_argument('--credential',type=Path,default=Path('/Users/henryzhang/Documents/credentials/vercel-cert-jev.txt'))
    a=p.parse_args();raise SystemExit(0 if decide(a.request,a.record,a.credential) else 1)
