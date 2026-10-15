"""Compare newly authored words against old data using the same native evaluator.

An absent dictionary entry can still be composed by an old engine. Measure
that distinction instead of claiming every added word was previously untypable.
Public author-known development cases, isolated personal data, no answer learning.
"""
import argparse,hashlib,json,subprocess,sys
from datetime import datetime,timezone
from pathlib import Path
from audit_remote_lexicon_074 import NAMES,parse
ROOT=Path(__file__).absolute().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8'))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--batches',nargs='+',required=True,type=int)
    p.add_argument('--baseline',type=Path,default=ROOT/'.build/baseline-077')
    p.add_argument('--resources',type=Path,default=ROOT/'.build/bin')
    p.add_argument('--engine',type=Path,default=ROOT/'.build/bin/LocalPinyinImeEval.exe')
    a=p.parse_args();assert a.batches==sorted(set(a.batches))
    dictionary=ROOT/'resources/dictionary';folder=ROOT/'.build/new-word-comparison078';folder.mkdir(exist_ok=True)
    engine_sha=sha(a.engine);runtime={n:sha(dictionary/n) for n in NAMES}
    assert runtime=={n:sha(a.resources/'dictionary'/n) for n in NAMES}
    baseline_sha={n:sha(a.baseline/'dictionary'/n) for n in NAMES}
    old_words={r['word'] for n in NAMES[:3] for r in parse((a.baseline/'dictionary'/n).read_bytes())[0]}
    cases=[];frozen={}
    for number in a.batches:
        prefix=dictionary/f'expansion/lexicon_batch_{number:03d}';meta=read(prefix.with_suffix('.json'));tsv=prefix.with_suffix('.tsv')
        assert sha(tsv)==meta['sha256'];frozen[tsv.name]=sha(tsv)
        entries=[line.split('\t') for line in tsv.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(entries)==meta['new_entries']
        for key,word,frequency in entries:
            assert word not in old_words
            cases.append((key,word,f'authored_batch_{number:03d}'))
    assert len(cases)==len({(k,w) for k,w,_ in cases})
    case_file=folder/'cases.tsv';case_file.write_text(''.join('\t'.join(row)+'\n' for row in cases),encoding='utf-8',newline='\n')
    case_sha=sha(case_file);results={};rows={}
    for label,resources in (('baseline_077',a.baseline),('current_078',a.resources)):
        output=folder/f'{label}.json'
        subprocess.run([sys.executable,str(ROOT/'tools/evaluate_typing.py'),'--engine',str(a.engine),'--resources',str(resources),'--cases',str(case_file),'--output',str(output)],check=True,stdout=subprocess.DEVNULL)
        data=read(output);assert len(data['rows'])==len(cases)
        assert all(len(r['candidates'])<=9 for r in data['rows'])
        results[label]={'summary':data['summary'],'full_output_sha256':sha(output)}
        rows[label]={(r['pinyin'],r['expected']):r for r in data['rows']}
    assert rows['baseline_077'].keys()==rows['current_078'].keys()
    gained=sum(rows['baseline_077'][k]['expected_rank']==0 and rows['current_078'][k]['expected_rank']>0 for k in rows['current_078'])
    lost=sum(rows['baseline_077'][k]['expected_rank']>0 and rows['current_078'][k]['expected_rank']==0 for k in rows['current_078'])
    assert runtime=={n:sha(dictionary/n) for n in NAMES}=={n:sha(a.resources/'dictionary'/n) for n in NAMES}
    assert baseline_sha=={n:sha(a.baseline/'dictionary'/n) for n in NAMES} and sha(a.engine)==engine_sha and sha(case_file)==case_sha
    report={'version':'0.7.8','baseline_version':'0.7.7','measured_at_utc':datetime.now(timezone.utc).isoformat(),'case_count':len(cases),'cases_sha256':case_sha,'frozen_batches_sha256':frozen,'runtime_sha256':runtime,'baseline_runtime_sha256':baseline_sha,'engine_sha256':engine_sha,'report_tool_sha256':sha(Path(__file__)),'personal_data_isolated':True,'expected_answers_learned':False,'complete_public_sentence_answers_imported':False,'scope':__doc__.strip(),'full_pinyin_queries_only':True,'not_blind_or_representative':True,'all_cases_absent_as_display_entries_in_baseline':True,'gained_in_top9':gained,'lost_from_top9':lost,**results}
    (ROOT/'docs/new-word-comparison-078.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'cases':len(cases),'baseline':results['baseline_077']['summary'],'current':results['current_078']['summary'],'gained_in_top9':gained,'lost_from_top9':lost},ensure_ascii=False))

if __name__=='__main__':main()
