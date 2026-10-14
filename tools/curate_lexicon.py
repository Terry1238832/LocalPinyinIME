#!/usr/bin/env python3
"""Prepare, freeze, merge and measure individually authored vocabulary batches.

Preparation produces worklists, never approved review records. The author must
read the complete selected words and keys, and provide every contextual
polyphone decision before freezing. A structural audit is not a second editor.
Python/pypinyin are authoring dependencies and are not packaged in the IME.
"""
from __future__ import annotations
import argparse
from collections import Counter
from datetime import datetime, timezone, date
from functools import lru_cache
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

import pypinyin
from pypinyin import Style, lazy_pinyin, pinyin
from evaluate_typing import summarize

ROOT = Path(__file__).absolute().parents[1]
OUT = ROOT/'resources/dictionary/expansion'
DICTIONARY = OUT.parent
LAYERS = ('core_zh_pinyin.tsv','local_core_zh_pinyin.tsv','standard_zh_pinyin.tsv')
RUNTIME = (*LAYERS,'context_zh.tsv','continuations_zh.tsv')


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def read(path): return json.loads(path.read_text(encoding='utf-8'))
def write(path,value): path.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
def rows(path): return [line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
@lru_cache(maxsize=None)
def readings(char): return tuple(pinyin(char,style=Style.NORMAL,heteronym=True,strict=True)[0])
def required(word): return any(len(readings(char)) > 1 for char in word)
def default_key(word): return ''.join(lazy_pinyin(word,style=Style.NORMAL,strict=True))
def hashes(names=RUNTIME): return {name:sha(DICTIONARY/name) for name in names}


def authoring(number):
    config_path=OUT/f'lexicon_batch_{number:03d}.authoring.json'
    config=read(config_path)
    assert config['author'] and config['review_scope'] and config['scope']
    assert set(config['categories'].values()) <= {5000,5500,6000}
    for name in ('draft','review'): assert Path(config[name]).name == config[name]
    return config,config_path,OUT/config['draft'],OUT/config['review']


def select(config,draft):
    existing={word for name in LAYERS for _,word,_ in rows(DICTIONARY/name)}
    fixtures=sorted((ROOT/'tests/data').glob('typing_*.tsv'))
    answers={line.split('\t')[1] for file in fixtures for line in file.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')}
    selected,seen,rejected,category=[],set(),[],''
    for line in draft.read_text(encoding='utf-8').splitlines():
        if line.startswith('# category:'): category=line.partition(':')[2].strip()
        elif line and not line.startswith('#'):
            assert category in config['categories'],category
            for word in line.split():
                assert re.fullmatch(r'[\u4e00-\u9fff]{2,6}',word),('Expected natural 2–6 Han vocabulary',word)
                reason='duplicate_draft' if word in seen else 'existing_three_layer_display' if word in existing else 'complete_public_answer' if word in answers else config.get('quality_exclusions',{}).get(word)
                seen.add(word)
                if reason: rejected.append({'word':word,'reason':reason}); continue
                selected.append((category,default_key(word),word,config['categories'][category]))
    return selected,rejected,{file.name:sha(file) for file in fixtures},len(answers)


def reviews(path,words):
    decisions={}
    for line in path.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'): continue
        key,word,author,choices,decision=line.split('\t')
        assert word in words and word not in decisions and author and decision
        assert re.fullmatch('[a-z]+',key)
        tokens=choices.split(';')
        assert len(tokens)==len(word),('Missing positional character choices',word)
        syllables=[]
        for position,(char,token) in enumerate(zip(word,tokens),1):
            prefix=f'{position}:{char}='
            assert token.startswith(prefix),(word,token,prefix)
            syllable=token[len(prefix):]
            assert syllable in readings(char),(word,char,syllable)
            syllables.append(syllable)
        assert ''.join(syllables)==key,('Key disagrees with recorded character choices',word)
        assert decision.startswith(('context:','semantic:','variant:')),(word,decision)
        decisions[word]={'key':key,'author':author,'choices':choices,'decision':decision}
    assert set(decisions)==words,('Review membership differs',sorted(words-set(decisions)),sorted(set(decisions)-words))
    return decisions


def bound_files(meta):
    for name,expected in meta['authoring_files_sha256'].items(): assert sha(OUT/name)==expected,name
    assert sha(Path(__file__))==meta['authoring_tool_sha256'],'Authoring tool changed after freeze'
    for name,expected in meta['benchmark_sha256'].items(): assert sha(ROOT/'tests/data'/name)==expected,name
    for name,expected in meta['supporting_tools_sha256'].items(): assert sha(ROOT/name)==expected,name


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=('prepare','freeze','apply','evaluate'))
    parser.add_argument('--batch',type=int,required=True)
    parser.add_argument('--engine',type=Path,default=ROOT/'.build/bin/LocalPinyinImeEval.exe')
    parser.add_argument('--resources',type=Path,default=ROOT/'.build/bin')
    a=parser.parse_args()
    assert pypinyin.__version__=='0.55.0'
    prefix=OUT/f'lexicon_batch_{a.batch:03d}'
    batch=prefix.with_suffix('.tsv'); metadata_path=prefix.with_suffix('.json'); applied_path=prefix.with_suffix('.applied.json')
    if a.action in ('prepare','freeze'):
        assert not applied_path.exists(),'An applied batch cannot be regenerated'
        config,config_path,draft,review=authoring(a.batch)
        selected,rejected,fixtures,answer_count=select(config,draft)
        needed={word for _,_,word,_ in selected if required(word)}
        if a.action=='prepare':
            preview=ROOT/f'.build/authoring{a.batch:03d}.preview.json'
            write(preview,{'selected':[{'category':c,'default_key':k,'word':w,'frequency':f,'polyphone_review_required':required(w)} for c,k,w,f in selected],'rejected':rejected,'baseline_sha256':hashes(LAYERS),'fixture_sha256':fixtures})
            print(json.dumps({'draft_candidates':len(selected)+len(rejected),'selected':len(selected),'mandatory_contextual_reviews':len(needed),'rejected':dict(Counter(row['reason'] for row in rejected)),'preview':preview.name},ensure_ascii=False)); return
        assert not metadata_path.exists() and not batch.exists(),'Frozen batch must not be overwritten'
        decisions=reviews(review,needed)
        final=sorted((c,decisions[w]['key'] if w in decisions else k,w,f) for c,k,w,f in selected)
        assert len(final)==len({(k,w) for _,k,w,_ in final})==len({w for _,_,w,_ in final})
        lines=[]; category=''
        for c,k,w,f in final:
            assert re.fullmatch('[a-z]+',k),(k,w)
            if w not in decisions: assert k==''.join(readings(char)[0] for char in w)
            if c!=category: lines.append('# category: '+c); category=c
            lines.append(f'{k}\t{w}\t{f}')
        batch.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
        overrides={word:item['key'] for word,item in decisions.items() if item['key']!=default_key(word)}
        variants={word:key for word,key in overrides.items() if decisions[word]['decision'].startswith('variant:')}
        for word in overrides: assert decisions[word]['decision'].startswith(('semantic:','variant:'))
        meta={'batch':prefix.name,'license':'MIT','authoring_source':'first_party_individually_authored','external_lexicon_import':False,'scope':config['scope'],'author':config['author'],'review_scope':config['review_scope'],
              'pronunciation_dependency':'pypinyin0.55.0 authoring-only; default agreement is not a review decision','new_entries':len(final),'sha256':sha(batch),
              'authoring_files_sha256':{path.name:sha(path) for path in (config_path,draft,review)},'authoring_tool_sha256':sha(Path(__file__)),
              'supporting_tools_sha256':{name:sha(ROOT/name) for name in ('tools/merge_lexicon_batch.py','tools/evaluate_typing.py')},
              'dedup_baseline_sha256':hashes(LAYERS),'benchmark_sha256':fixtures,'benchmark_unique_full_answers':answer_count,'benchmark_full_answers_added':0,
              'preset_category_weights':config['categories'],'frequency_policy':'Fixed coarse project-relative category bands before native evaluation; no measured corpus frequencies or tuning to public answer ranks.',
              'category_counts':dict(Counter(c for c,_,_,_ in final)),'length_distribution':dict(Counter(str(len(w)) for _,_,w,_ in final)),
              'authoring_rejections':rejected,'mandatory_polyphone_reviews':len(needed),'manual_pronunciation_overrides':overrides,'reading_variant_choices':variants,
              'semantic_pronunciation_corrections':len(overrides)-len(variants),'line_endings':'UTF-8 LF','source_policy':'No external lexicon or source-page text imported; prior upstream provenance remains separately recorded.'}
        write(metadata_path,meta)
        print(json.dumps({'batch':a.batch,'added':len(final),'reviews':len(needed),'semantic_corrections':meta['semantic_pronunciation_corrections'],'variants':len(variants),'sha256':meta['sha256']},ensure_ascii=False)); return
    meta=read(metadata_path)
    bound_files(meta); assert sha(batch)==meta['sha256']
    if a.action=='apply':
        assert not applied_path.exists()
        assert hashes(LAYERS)==meta['dedup_baseline_sha256'],'Canonical baseline changed since freeze'
        before=(DICTIONARY/'local_core_zh_pinyin.tsv').read_bytes(); old_rows=set(rows_as_strings(before))
        config,_,draft,review=authoring(a.batch)
        selected,_,_,_=select(config,draft)
        decisions=reviews(review,{w for _,_,w,_ in selected if required(w)})
        reconstructed={(decisions[w]['key'] if w in decisions else k,w,str(f)) for _,k,w,f in selected}
        assert reconstructed=={tuple(row) for row in rows(batch)}
        subprocess.run([sys.executable,str(ROOT/'tools/merge_lexicon_batch.py'),'--dictionary-dir',str(DICTIONARY),'--batch',str(batch),'--holdout',str(ROOT/'tests/data/typing_holdout.tsv')],check=True)
        after=(DICTIONARY/'local_core_zh_pinyin.tsv').read_bytes()
        assert old_rows <= set(rows_as_strings(after)) and len(rows_as_strings(after))-len(old_rows)==meta['new_entries']
        registry_path=DICTIONARY/'sources/lexicon_sources.json'; registry=read(registry_path)
        source_id='first_party_'+prefix.name
        assert source_id not in {row['source_id'] for row in registry['sources']}
        registry['sources'].append({'source_id':source_id,'source_name':config['scope'],'source_url':'','license':'MIT','license_url':'https://github.com/Terry1238832/LocalPinyinIME/blob/main/LICENSE','redistribution_allowed':True,'attribution_required':True,'share_alike_required':False,'commercial_use_allowed':True,'derivative_work_allowed':True,'review_status':'approved','reviewed_at':date.today().isoformat(),'included_in_release':True,'notes':f"{meta['new_entries']} individually authored project phrases. Single author contextual pronunciation review, not claimed as an independent human linguistic review. Frozen authoring files, explicit character choices and coarse preset weights are bound in expansion/{metadata_path.name}; no public complete answers or external bulk vocabulary imported."})
        write(registry_path,registry)
        applied={'batch':prefix.name,'frozen_batch_sha256':sha(batch),'canonical_before_sha256':hashlib.sha256(before).hexdigest(),'canonical_after_sha256':hashlib.sha256(after).hexdigest(),'canonical_before_entries':len(old_rows),'canonical_after_entries':len(rows_as_strings(after)),'merged_entries':meta['new_entries'],'all_existing_rows_and_weights_preserved':True,'source_policy_preserved':True,'authoring_tool_chain_revalidated':True,'applied_at_utc':datetime.now(timezone.utc).isoformat()}
        write(applied_path,applied); print(json.dumps(applied,ensure_ascii=False)); return
    assert applied_path.exists()
    resources=a.resources.absolute(); engine=a.engine.absolute()
    engine_hash=sha(engine)
    runtime=hashes()
    assert {name:sha(resources/'dictionary'/name) for name in RUNTIME}==runtime,'Native resource copy is stale'
    cases=rows(batch)
    with tempfile.TemporaryDirectory(prefix=f'lexicon{a.batch:03d}-',dir=ROOT/'.build') as directory:
        temp=Path(directory); case_path=temp/'cases.tsv'
        case_path.write_text(''.join(f'{key}\t{word}\t{weight}\n' for key,word,weight in cases),encoding='utf-8',newline='\n')
        env=dict(os.environ,LOCALPINYINIME_LOCALAPPDATA=str(temp),LOCALPINYINIME_USER_LEXICON_PATH=str(temp/'user_lexicon.tsv'))
        output=subprocess.run([str(engine),str(resources),str(case_path)],check=True,capture_output=True,env=env)
    measured=[json.loads(line) for line in output.stdout.decode('utf-8').splitlines() if line]
    assert sha(engine)==engine_hash and hashes()==runtime and sha(batch)==meta['sha256']
    assert {name:sha(resources/'dictionary'/name) for name in RUNTIME}==runtime
    assert {(row['pinyin'],row['expected']) for row in measured}=={(k,w) for k,w,_ in cases} and len(measured)==len(cases)
    summary=summarize(measured)
    missing=[row for row in measured if not 1 <= row['expected_rank'] <= 9]
    summary.update(missing_from_top9_count=len(missing),missing_from_top9=[{'pinyin':r['pinyin'],'word':r['expected'],'candidates':r['candidates']} for r in missing],max_returned_candidate_count=max(len(r['candidates']) for r in measured))
    assert summary['max_returned_candidate_count']<=9
    report={'batch':prefix.name,'evaluation':'native_engine_candidate_coverage','evaluated_at_utc':datetime.now(timezone.utc).isoformat(),'engine_sha256':sha(engine),'resource_sha256':runtime,'frozen_batch_sha256':sha(batch),'personal_data_isolated':True,'expected_answers_learned':False,'summary':summary,'rows':measured}
    validation=prefix.with_suffix('.validation.json')
    if validation.exists(): report['previous_pass']={'sha256':sha(validation),'summary':read(validation)['summary']}
    write(validation,report)
    applied=read(applied_path); applied['native_candidate_coverage']={'status':'passed' if not missing else 'completed_with_missing_candidates','report':validation.name,'sha256':sha(validation),**summary}; write(applied_path,applied)
    print(json.dumps({'batch':a.batch,'summary':summary,'report_sha256':sha(validation)},ensure_ascii=False))


def rows_as_strings(data): return [line for line in data.decode('utf-8').splitlines() if line and not line.startswith('#')]
if __name__=='__main__': main()
