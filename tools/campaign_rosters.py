#!/usr/bin/env python3
"""Exact roster/appearance extraction for the campaign PNG atlas."""
from pathlib import Path
import json,collections,hashlib,csv
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'output/campaign-atlas';SRC=Path('/Users/henryzhang/Downloads/sgzccz')
S=json.loads((OUT/'scripts.json').read_text());H=json.loads((ROOT/'assets/generated/data.json').read_text())['heroes'];OP=json.loads((OUT/'opcode-names.json').read_text())
def vals(i):return [a['value'] for a in i.get('args',[])]
def uid(n):
 if 0<=n<len(H):return H[n]['name']+f'#{n}'
 return {1024:'任意在场单位',1025:'任意我/友军',1026:'任意敌军',1027:'当前事件武将',-1:'未指定'}.get(n,f'特殊ID{n}')
# Resolved campaign flags; remaining flags are deliberately named by numeric ID.
FN={0:'本战已结算',1:'本战失败已处理',706:'濮阳壹全歼胜利',707:'濮阳贰全歼胜利',710:'典韦阵亡',724:'郭嘉获救',727:'赤壁A全歼胜利',728:'赤壁B全歼胜利',735:'定军山撤退',835:'夏侯渊单挑阵亡',738:'庞德阵亡',645:'红线建业通关'}
REL={0:'≥',1:'<',2:'='}
def flag(n):return FN.get(n,f'F{n}')
def fmt(i,choice=None):
 o=i['opcode'];a=vals(i)
 if o==2:return ''
 if o==-3:return '非('+fmt(i['negated'],choice)+')'
 if o==3:return '前一条件不成立〔上下文见原始列表〕'
 if o==5:return '、'.join([flag(n)+'=1'for n in a[0]]+[flag(n)+'=0'for n in a[1]])
 if o==19:
  options=choice.split('\n')if choice else []
  return '选择「'+options[a[0]-1]+'」'if 0<a[0]<=len(options) else f'选择第{a[0]}项'
 if o==18:return '选择框「'+' / '.join(a[0].splitlines())+'」'
 if o==37:return f'{uid(a[0])}到达({a[1]},{a[2]})'
 if o==38:return f'{uid(a[0])}进入({a[1]},{a[2]})—({a[3]},{a[4]})'
 if o==46:return f'{uid(a[0])}与{uid(a[1])}'+('对话接触'if a[2] else '单挑接触')
 if o==54:return f'{uid(a[0])}.{["攻击","防御","精神","爆发","士气","最大HP","最大MP","当前HP"][a[1]] if a[1]<8 else a[1]}{REL.get(a[3],"?")}{a[2]}'
 if o==55:return f'{["金钱","剧本计数","路线值"][a[0]]}{REL[a[2]]}{a[1]}'
 if o==63:return f'回合{REL[a[1]]}{a[0]}'
 if o==64:return f'行动方={a[0]}〔0我/1友/2敌〕'
 if o==65:return f'阵营{a[0]}人数{REL[a[2]]}{a[1]}'+(f'，区域参数{a[3:]}'if a[3] else '')
 if o==66:return '胜利条件满足'
 if o==67:return '失败条件满足'
 if o==7:return '点击出兵'
 if o==94:return '武将不同'+str(a)
 return OP.get(str(o),f'op{o}')+str(a)
def walk(xs,guards=(),choice=None):
 previous=None
 for i in xs:
  o=i['opcode']
  if o==18:choice=vals(i)[0]
  yield i,guards,choice
  if 'body'in i:
   t=i['test'];t=dict(t)
   if t['opcode']==3 and previous is not None:t={'opcode':-3,'negated':previous,'offset':t['offset'],'args':[]}
   childchoice=vals(t)[0]if t['opcode']==18 else choice
   yield from walk(i['body'],guards+(t,),childchoice)
   previous=t

def evs(sc):
 for sn,sec in enumerate(sc['sections']):
  for en,e in enumerate(sec['events']):
   for i,g,ch in walk(e['body'],tuple(e['conditions'])):
    yield sn,en,e,i,g,ch

def guard(g,ch):return ' 且 '.join(t for x in g if (t:=fmt(x,ch)))or'无额外条件'
def ref(f,i):return f+'.eex:'+f'0x{i["offset"]:04x}'
def dump(n,v):(OUT/n).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n')

raw=(SRC/'Imsg.e5').read_bytes();titles=[raw[0xc350+200*i:0xc350+200*(i+1)].split(b'\0')[0].decode('gbk')for i in range(58)]
base={0,1,5,6,10,11,12}
def pool(n):
 p={0}if n==0 else set(base)
 if n>=4:p|={16,20,24}
 if n>=5:p|={21,22}
 if n>=6:p|={14}
 if n>=7:p|={18}
 if n>=8:p|={15}
 if n>=10:p|={13,25}
 if n>=15:p|={2,17,23}
 if n>=19:p|={7}
 if n>=22:p|={8}
 if n>=32:p|={19}
 if n>=33:p|={9}
 if n>=38:p|={4}
 if n>=48:p|={3}
 return sorted(p)
OPTIONAL={3:'选择「自行追击董卓」才进入；放弃则跳过。',7:'濮阳壹未全歼、曹操撤离（F706=0）才进入。',8:'濮阳壹、贰均未全歼（F706=0且F707=0）才进入。',27:'赤壁A：柳城保全郭嘉（F724=1）；与S28二选一。',28:'赤壁B：郭嘉未获救（F724=0）；与S27二选一。',29:'赤壁A/B选择逃脱路线而非全歼胜利才进入。',36:'红/金线且定军山撤退（F735=1）；含失守后选择撤退，或第20回合据点已失守且敌军仍≥10的撤退分支。',37:'红/金线且F735=1；接汉水之战。'}
def route(n):
 if n in OPTIONAL:return '条件战',OPTIONAL[n]
 if n<32:return '共同必经','继续推进战役时必经；不包括游戏失败后重开。'
 if n<46:return '红/金线必经','R32路线值≥16进入此线；本战在此线必经。'
 return '蓝线必经','R32路线值<16进入此线；本战在此线必经。'
A=[];flagsetters=collections.defaultdict(list);changes=[]
for f,sc in S.items():
 for sn,en,e,i,g,ch in evs(sc):
  a=vals(i)
  if i['opcode']==11:flagsetters[f'{f}:{a[0]}'].append({'ref':ref(f,i),'value':a[1],'condition':guard(g,ch),'event_offset':e['offset']})
  if i['opcode']==59:changes.append({'file':f,'ref':ref(f,i),'unit':uid(a[0]),'id':a[0],'joins':a[1]==0,'condition':guard(g,ch)})
for n in range(58):
 f=f'S_{n:02}';sc=S[f];tag,why=route(n);rows=[];groups=collections.OrderedDict();placements=[];removals=[];pre=[];triggers=[]
 for sn,en,e,i,g,ch in evs(sc):
  o=i['opcode'];a=vals(i);cond=guard(g,ch)
  if o in(70,71):
   k=11 if o==70 else 12;assert len(a)%k==0
   for j in range(len(a)//k):
    v=a[j*k:(j+1)*k]
    if v[0]<0:continue
    enemy=o==71;extra=int(enemy)
    rows.append({'id':v[0],'name':H[v[0]]['name'],'slot':j+(35 if enemy else 15),'side':'敌援'if enemy and v[1]>0 else'敌军'if enemy else'友军','hidden':v[2 if enemy else 1]>0,'xy':v[2+extra:4+extra],'ref':ref(f,i),'row':j,'raw_values':v,'condition':cond})
  if o==75:placements.append({'slot':a[0],'xy':a[1:3],'hidden':a[4]>0,'section':sn,'ref':ref(f,i),'condition':cond})
  if o in(76,85):
   who=uid(a[1])if a[0] in(0,-1) else f'我军出阵槽{a[2]+1}'
   action='显现'if o==76 else'复活/再登场'
   key=(sn,en,cond,action)
   if key not in groups:groups[key]={'key':f'C{len(groups)+1:02}','section':sn,'event':en,'event_offset':e['offset'],'condition':cond,'action':action,'units':[],'refs':[]}
   t=groups[key];t['units'].append(who);t['refs'].append(ref(f,i));triggers.append({'opcode':o,'args':a,'condition':cond,'section':sn,'event':en,'ref':ref(f,i)})
  if o in(49,83):removals.append({'opcode':o,'args':a,'condition':cond,'ref':ref(f,i),'section':sn,'event':en})
 for pf in [f'R_{n:02}',f]:
  for sn,en,e,i,g,ch in evs(S[pf]):
   a=vals(i);o=i['opcode']
   if o==6 and a[0]==1 or o==74:
    v=a[1:]if o==6 else a;forced=[x for x in v[1:6]if x>0];excluded=[x for x in v[6:11]if x>=0]
    if 0 not in excluded:forced=[0]+forced
    pre.append({'max':v[0],'forced':forced,'excluded':excluded,'condition':guard(g,ch),'ref':ref(pf,i)})
 candidate=pool(n);ban=set(x for p in pre for x in p['excluded'])
 candidate=[x for x in candidate if x not in ban]
 notes=[]
 if n>=11:notes.append('典韦：S10存活（F710=0）才仍在队。')
 if n>=27 and n!=28:notes.append('郭嘉：S24获救（F724=1）才仍在队。')
 if n==28:candidate=[x for x in candidate if x!=22]
 if 36<=n<=45:notes.append('夏侯渊：S35未触发与黄忠单挑阵亡（F835=0）。')
 if 39<=n<=45:notes.append('庞德：S38未触发与周仓单挑阵亡（F738=0）。')
 if n>=48:notes.append('关羽：蓝线R48加入我军；此前友军身份列在固定表中。')
 for r in rows:
  r['appearances']=[t['key']for t in groups.values()if uid(r['id'])in t['units']]
  if r['hidden'] and not r['appearances']:notes.append(f'{uid(r["id"])}初始隐藏，但未找到按ID的显现/复活命令；需检查范围/特殊ID或未使用占位。')
 for t in groups.values():
  t['unit_counts']=dict(collections.Counter(t['units']))
  t['units']=list(dict.fromkeys(t['units']))
 A.append({'id':n,'script':f,'name':titles[n],'route_class':tag,'entry_condition':why,'player_pool':candidate,'player_selection':pre,'player_notes':notes,'roster':rows,'placements':placements,'appearance_groups':list(groups.values()),'appearance_instructions':triggers,'hide_retreat_instructions':removals})
dump('battles.json',A);dump('party-changes.json',changes);dump('flag-setters.json',flagsetters)
with (OUT/'all-units.csv').open('w')as fp:
 w=csv.writer(fp);w.writerow(['battle','name','side','unit_id','unit_name','slot','initial_hidden','x','y','appearance_groups','source'])
 for b in A:
  for r in b['roster']:w.writerow([b['script'],b['name'],r['side'],r['id'],r['name'],r['slot'],r['hidden'],*r['xy'],','.join(r['appearances']),r['ref']+f' row{r["row"]}'])
  for i in b['player_pool']:w.writerow([b['script'],b['name'],'我军可选池',i,H[i]['name'],'依玩家选择','见出阵槽条件','','','', ';'.join(p['ref']for p in b['player_selection'])])
print('battles',len(A),'NPC roster rows',sum(len(x['roster'])for x in A),'appearance groups',sum(len(x['appearance_groups'])for x in A),'appearance calls',sum(len(x['appearance_instructions'])for x in A),'party changes',len(changes))
