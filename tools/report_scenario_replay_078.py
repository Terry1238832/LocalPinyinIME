"""Public supplemental replay; immutable cases, isolated data, no learning.

This has one short attributed article excerpt and authored everyday scenarios.
It is not a blind test, a representative corpus, or evidence of clean-machine
installation. The original 267 fixtures are separate and remain unchanged.
"""
import hashlib,json,subprocess,sys
from datetime import datetime,timezone
from pathlib import Path
from audit_remote_lexicon_074 import NAMES,parse
ROOT=Path(__file__).absolute().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def write_rank_changes(report,folder):
 rows={}
 for label in ('baseline_077','current_078'):
  path=folder/f'{label}.json';assert sha(path)==report[label]['full_output_sha256']
  data=json.loads(path.read_text(encoding='utf-8'));assert data['summary']==report[label]['summary']
  rows[label]={(r['source'],r['pinyin'],r['expected']):r for r in data['rows']}
 assert rows['baseline_077'].keys()==rows['current_078'].keys() and len(rows['current_078'])==32
 def changes(predicate):
  return [{'source':k[0],'pinyin':k[1],'expected':k[2],'baseline_rank':rows['baseline_077'][k]['expected_rank'],'current_rank':rows['current_078'][k]['expected_rank'],'baseline_candidates':rows['baseline_077'][k]['candidates'],'current_candidates':rows['current_078'][k]['candidates']} for k in rows['current_078'] if predicate(rows['baseline_077'][k]['expected_rank'],rows['current_078'][k]['expected_rank'])]
 output={'version':'0.7.8','scenario_report_sha256':sha(ROOT/'docs/scenario-replay-078.json'),'cases_sha256':report['cases_sha256'],'engine_sha256':report['engine_sha256'],'runtime_sha256':report['runtime_sha256'],'scope':'Observed whole-input candidate changes on the unchanged 32 public development cases. No answer import, learning or per-case weight adjustment. Selecting shorter words interactively remains a separate path.','lost_from_top9':changes(lambda b,c:b>0 and c==0),'gained_in_top9':changes(lambda b,c:b==0 and c>0),'lost_first_place':changes(lambda b,c:b==1 and c!=1),'gained_first_place':changes(lambda b,c:b!=1 and c==1)}
 assert report['baseline_077']['summary']['top9_present']-len(output['lost_from_top9'])+len(output['gained_in_top9'])==report['current_078']['summary']['top9_present']
 (ROOT/'docs/scenario-replay-078-rank-changes.json').write_text(json.dumps(output,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
def main():
 cases=ROOT/'tests/data/scenario_replay_076.tsv';engine=ROOT/'.build/bin/LocalPinyinImeEval.exe'
 case_sha=sha(cases);engine_sha=sha(engine);dictionary=ROOT/'resources/dictionary'
 runtime={n:sha(dictionary/n) for n in NAMES}
 assert runtime=={n:sha(ROOT/'.build/bin/dictionary'/n) for n in NAMES}
 expected=[line.split('\t')[1] for line in cases.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
 all_words={r['word'] for n in ('core_zh_pinyin.tsv','local_core_zh_pinyin.tsv','standard_zh_pinyin.tsv') for r in parse((dictionary/n).read_bytes())[0]}
 assert not (set(expected)&all_words),'Complete replay answer found in dictionary'
 results={};folder=ROOT/'.build/scenario-replay078';folder.mkdir(exist_ok=True)
 for label,resources in (('baseline_077',ROOT/'.build/baseline-077'),('current_078',ROOT/'.build/bin')):
  out=folder/f'{label}.json'
  subprocess.run([sys.executable,str(ROOT/'tools/evaluate_typing.py'),'--engine',str(engine),'--resources',str(resources),'--cases',str(cases),'--output',str(out),'--keystrokes'],check=True,stdout=subprocess.DEVNULL)
  data=json.loads(out.read_text(encoding='utf-8'));results[label]={'summary':data['summary'],'full_output_sha256':sha(out)}
 assert sha(cases)==case_sha and sha(engine)==engine_sha and runtime=={n:sha(dictionary/n) for n in NAMES}
 report={'version':'0.7.8','measured_at_utc':datetime.now(timezone.utc).isoformat(),'cases_sha256':case_sha,'engine_sha256':engine_sha,'runtime_sha256':runtime,'case_count':len(expected),'original_article_excerpt_characters':11,'article_source':{'url':'https://www.nppa.gov.cn/xxfb/ywdt/202604/t20260417_985269.html','title':'书香满华夏 阅读启新篇','published':'2026-04-17','authors':'张君成、李婧璇','original_publication':'中国新闻出版广电报','reuse_scope':'One 11-character attributed excerpt for development input replay; no blanket reuse license inferred.'},'scope':'Public development replay: one short article excerpt and authored everyday scenarios. Not blind or representative. Whole-input top-nine availability is distinct from selecting words interactively.','complete_answers_imported':False,'personal_data_isolated':True,'expected_answers_learned':False,**results}
 (ROOT/'docs/scenario-replay-078.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
 write_rank_changes(report,folder)
 print(json.dumps(results,ensure_ascii=False))
if __name__=='__main__':main()
