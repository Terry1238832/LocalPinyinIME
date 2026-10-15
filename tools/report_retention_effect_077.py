"""Separate engine retention changes from dictionary additions on fixed data.

The same final resources are queried by the preserved pre-change executable and
the current executable; the 267 public fixtures stay unchanged. The controlled
regression and four development discoveries are disclosed, not called blind.
"""
import hashlib,json,subprocess,sys
from pathlib import Path
from datetime import datetime,timezone
from evaluate_typing import summarize
ROOT=Path(__file__).absolute().parents[1]
NAMES=('core_zh_pinyin.tsv','local_core_zh_pinyin.tsv','standard_zh_pinyin.tsv','context_zh.tsv','continuations_zh.tsv')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8'))

def main():
    old=ROOT/'.build/baseline-076/LocalPinyinImeEval.exe';new=ROOT/'.build/bin/LocalPinyinImeEval.exe'
    quality=read(ROOT/'docs/lexicon-077-quality.json');runtime={n:sha(ROOT/'resources/dictionary'/n) for n in NAMES}
    assert runtime==quality['runtime_sha256']=={n:sha(ROOT/'.build/bin/dictionary'/n) for n in NAMES}
    assert sha(old)==read(ROOT/'docs/lexicon-076-quality.json')['engine_sha256'] and sha(new)==quality['engine_sha256']
    folder=ROOT/'.build/retention-effect077';folder.mkdir(exist_ok=True)
    old_rows=[]
    for label,name in (('fixed','typing_holdout.tsv'),('fresh','typing_fresh.tsv'),('independent','typing_independent.tsv')):
        out=folder/f'old-engine-current-data-{label}.json'
        subprocess.run([sys.executable,str(ROOT/'tools/evaluate_typing.py'),'--engine',str(old),'--resources',str(ROOT/'.build/bin'),'--cases',str(ROOT/'tests/data'/name),'--output',str(out)],check=True,stdout=subprocess.DEVNULL)
        old_rows.extend(read(out)['rows'])
    assert len(old_rows)==267
    cases=folder/'discoveries.tsv';cases.write_text('nabujuhaimeikan\t那部剧还没看\tauthored049\nnabujukanwanle\t那部剧看完了\tauthored049\nyigongjijia\t一共几集啊\tauthored049\nzhebujudijiji\t这部剧第几集\tauthored049\n',encoding='utf-8',newline='\n')
    discoveries={}
    for label,engine in (('before',old),('after',new)):
        out=folder/f'discoveries-{label}.json'
        subprocess.run([sys.executable,str(ROOT/'tools/evaluate_typing.py'),'--engine',str(engine),'--resources',str(ROOT/'.build/bin'),'--cases',str(cases),'--output',str(out)],check=True,stdout=subprocess.DEVNULL)
        discoveries[label]=read(out)
    assert runtime=={n:sha(ROOT/'resources/dictionary'/n) for n in NAMES}=={n:sha(ROOT/'.build/bin/dictionary'/n) for n in NAMES}
    report={'version':'0.7.7','measured_at_utc':datetime.now(timezone.utc).isoformat(),'method':__doc__.strip(),'old_engine_sha256':sha(old),'current_engine_sha256':sha(new),'same_final_runtime_sha256':runtime,'engine_source_sha256':sha(ROOT/'src/engine/pinyin_engine.cpp'),'controlled_regression_source_sha256':sha(ROOT/'tests/test_pinyin_engine.cpp'),'dictionary_weights_changed_for_cases':False,'personal_data_isolated':True,'expected_answers_learned':False,'old_engine_final_data_summary':summarize(old_rows),'current_engine_final_data_summary':quality['overall']['current_summary'],'current_engine_baseline_data_summary':quality['overall']['baseline_summary'],'development_discoveries':{k:{'summary':v['summary'],'rows':v['rows']} for k,v in discoveries.items()},'limitation':'Up to two eligible complete alternatives can be rescued beside contextual choices and the shorter prefix. This does not guarantee every homophone, all whole sentences or installed-host routing.'}
    (ROOT/'docs/complete-word-retention-077.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'public_before':report['old_engine_final_data_summary'],'public_after':report['current_engine_final_data_summary'],'discoveries_before':discoveries['before']['summary'],'discoveries_after':discoveries['after']['summary']},ensure_ascii=False))

if __name__=='__main__':main()
