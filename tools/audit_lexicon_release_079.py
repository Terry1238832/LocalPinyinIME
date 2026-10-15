"""Read-only reconstruction of the 0.7.8 baseline and frozen new author batches.

The reused structural checks inspect byte bindings and contextual records;
they do not constitute an independent linguistic review.
"""
import argparse,json,pypinyin
from datetime import datetime,timezone
from pathlib import Path
from audit_lexicon_release_076 import ROOT,DICT,OUT,LAYERS,RUNTIME,blob,rows,sha,digest,read,audit_batch

from common_word_priorities_079 import reconstruct_local, MANIFEST, SOURCE_ID

BASE='6a6f1ecd894134d6ab8f2435274b40746e968eba'

def main():
    assert pypinyin.__version__=='0.55.0'
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--batches',nargs='+',type=int,required=True);a=p.parse_args()
    assert a.batches==sorted(set(a.batches)) and min(a.batches)>=100
    fixtures={f.name:sha(f) for f in sorted((ROOT/'tests/data').glob('typing_*.tsv'))}
    answers={line.split('\t')[1] for n in fixtures for line in (ROOT/'tests/data'/n).read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')}
    old={n:blob(BASE,n) for n in RUNTIME};other={n:(DICT/n).read_bytes() for n in RUNTIME}
    assert all(old[n]==other[n] for n in RUNTIME if n!='local_core_zh_pinyin.tsv')
    manifest=read(MANIFEST);priority_base=reconstruct_local(old['local_core_zh_pinyin.tsv'],manifest)
    application=read(ROOT/'docs/common-word-priorities-079-application.json')
    assert application['canonical_after_sha256']==digest(priority_base) and application['manifest_sha256']==sha(MANIFEST)
    prior_pairs={(k,w) for n in LAYERS for _,k,w,_ in rows(old[n])}
    assert all((e['pinyin'],e['word']) in prior_pairs for e in manifest['entries'])
    current=priority_base;results=[]
    for number in a.batches:
        current,result=audit_batch(number,current,other,fixtures,answers);results.append(result)
    assert current==other['local_core_zh_pinyin.tsv']
    before={(k,w):f for _,k,w,f in rows(priority_base)}
    after={(k,w):f for _,k,w,f in rows(current)}
    assert all(after.get(pair)==weight for pair,weight in before.items())
    registry=read(DICT/'sources/lexicon_sources.json');prior=json.loads(blob(BASE,'sources/lexicon_sources.json'))
    assert registry['sources'][:len(prior['sources'])]==prior['sources']
    extra=registry['sources'][len(prior['sources']):]
    assert len(extra)==len(a.batches)+1
    priority_source=next(x for x in extra if x['source_id']==SOURCE_ID)
    assert priority_source['manifest_sha256']==sha(MANIFEST) and priority_source['new_unique_key_display_pairs']==0
    for n in a.batches:
        item=next(x for x in extra if x['source_id']==f'first_party_lexicon_batch_{n:03d}')
        assert item['license']=='MIT'
    metadata=read(DICT/'local_core_zh_pinyin.metadata.json')
    assert metadata['entry_count']==len(after) and metadata['sha256'].lower()==digest(current)
    assert metadata['source_policy'].startswith(json.loads(blob(BASE,'local_core_zh_pinyin.metadata.json'))['source_policy'])
    total=len({(k,w) for n in LAYERS for _,k,w,_ in rows(other[n])})
    report={'version':'0.7.9','status':'passed','audited_at_utc':datetime.now(timezone.utc).isoformat(),'base_commit':BASE,'audit_tool_sha256':sha(Path(__file__)),'reused_structural_checks_sha256':sha(ROOT/'tools/audit_lexicon_release_076.py'),'method':__doc__.strip(),'runtime_sha256':{n:digest(b) for n,b in other.items()},'source_registry_sha256':sha(DICT/'sources/lexicon_sources.json'),'typing_fixture_sha256':fixtures,'public_complete_answers_excluded':len(answers),'current_local_core_entries':len(after),'current_three_layer_unique_pairs':total,'authored_entries':sum(x['entries'] for x in results),'baseline_local_rows_preserved':True,'nonpriority_old_weights_preserved':True,'declared_priority_entries':len(manifest['entries']),'priority_manifest_sha256':sha(MANIFEST),'priority_application_sha256':sha(ROOT/'docs/common-word-priorities-079-application.json'),'all_prior_source_records_preserved':True,'contextual_review_membership_verified':True,'independent_linguistic_review':{'status':'not_performed','scope':'Single AI author reviews displays and keys; separate code-path reconstruction is structural only.'},'batches':results}
    (ROOT/'docs/lexicon-079-authoring-audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    lines=['# 0.7.9 冻结词库结构审计','',f"0.7.8 固定提交加常用词优先级清单及冻结批次逐字节重建通过；原创补词 {report['authored_entries']:,}，有效键词对 {total:,}。",'','旧本地词、旧来源记录及另外四份运行数据保留。118项常用词按单独清单提高优先级，其余旧权重保持。267 个公开整句答案排除；权重在候选测试前按类别固定。','', '逐词语境校音由单一 AI 作者完成，结构检查不等于第二名语言编辑，也不等于所有长句均能直接命中。','', '| 批次 | 新词 | 语境读音记录 | 语义修正 | 合法主读音选择 |','| --- | ---: | ---: | ---: | ---: |']
    lines.extend(f"| {x['batch']} | {x['entries']} | {x['contextual_reviews']} | {x['semantic_corrections']} | {x['legitimate_main_reading_choices']} |" for x in results)
    lines.extend(['','[完整证据](lexicon-079-authoring-audit.json)。此前导入词库的来源和许可状态继承，不重新标记为本轮原创。',''])
    (ROOT/'docs/lexicon-079-authoring-audit.md').write_text('\n'.join(lines),encoding='utf-8',newline='\n')
    print(json.dumps({'status':'passed','batches':len(results),'authored':report['authored_entries'],'total':total}))

if __name__=='__main__':main()
