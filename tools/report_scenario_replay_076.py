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
def main():
 cases=ROOT/'tests/data/scenario_replay_076.tsv';engine=ROOT/'.build/bin/LocalPinyinImeEval.exe'
 case_sha=sha(cases);engine_sha=sha(engine);dictionary=ROOT/'resources/dictionary'
 runtime={n:sha(dictionary/n) for n in NAMES}
 assert runtime=={n:sha(ROOT/'.build/bin/dictionary'/n) for n in NAMES}
 expected=[line.split('\t')[1] for line in cases.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
 all_words={r['word'] for n in ('core_zh_pinyin.tsv','local_core_zh_pinyin.tsv','standard_zh_pinyin.tsv') for r in parse((dictionary/n).read_bytes())[0]}
 assert not (set(expected)&all_words),'Complete replay answer found in dictionary'
 results={};folder=ROOT/'.build/scenario-replay076';folder.mkdir(exist_ok=True)
 for label,resources in (('baseline_075',ROOT/'.build/baseline-075'),('current_076',ROOT/'.build/bin')):
  out=folder/f'{label}.json'
  subprocess.run([sys.executable,str(ROOT/'tools/evaluate_typing.py'),'--engine',str(engine),'--resources',str(resources),'--cases',str(cases),'--output',str(out),'--keystrokes'],check=True,stdout=subprocess.DEVNULL)
  data=json.loads(out.read_text(encoding='utf-8'));results[label]={'summary':data['summary'],'full_output_sha256':sha(out)}
 assert sha(cases)==case_sha and sha(engine)==engine_sha and runtime=={n:sha(dictionary/n) for n in NAMES}
 report={'version':'0.7.6','measured_at_utc':datetime.now(timezone.utc).isoformat(),'cases_sha256':case_sha,'engine_sha256':engine_sha,'runtime_sha256':runtime,'case_count':len(expected),'original_article_excerpt_characters':11,'article_source':{'url':'https://www.nppa.gov.cn/xxfb/ywdt/202604/t20260417_985269.html','title':'书香满华夏 阅读启新篇','published':'2026-04-17','authors':'张君成、李婧璇','original_publication':'中国新闻出版广电报','reuse_scope':'One 11-character attributed excerpt for development input replay; no blanket reuse license inferred.'},'scope':'Public development replay: one short article excerpt and authored everyday scenarios. Not blind or representative. Whole-input top-nine availability is distinct from selecting words interactively.','complete_answers_imported':False,'personal_data_isolated':True,'expected_answers_learned':False,**results}
 (ROOT/'docs/scenario-replay-076.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
 print(json.dumps(results,ensure_ascii=False))
if __name__=='__main__':main()
