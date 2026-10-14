#!/usr/bin/env python3
"""Reconstruct 0.7.6 data from Git, finite upstream review and frozen author files.

Read-only structural audit. It verifies recorded contextual decisions and exact
bytes; it does not claim a second human/AI linguistic editor or blind accuracy.
"""
import argparse
from collections import Counter
from datetime import datetime,timezone
from functools import lru_cache
import hashlib,json,re,subprocess
from pathlib import Path
import pypinyin
from pypinyin import Style,pinyin,lazy_pinyin

ROOT=Path(__file__).absolute().parents[1]
BASE_COMMIT='c6a1f769d8e6a1d37d7f86711df03ef2158fcc5e'
REMOTE_COMMIT='435c8d880837cc12cee7228265cf14604515705f'
DICT=ROOT/'resources/dictionary'; OUT=DICT/'expansion'
LAYERS=('core_zh_pinyin.tsv','local_core_zh_pinyin.tsv','standard_zh_pinyin.tsv')
RUNTIME=(*LAYERS,'context_zh.tsv','continuations_zh.tsv')
def digest(data): return hashlib.sha256(data).hexdigest()
def sha(path): return digest(path.read_bytes())
def read(path): return json.loads(path.read_text(encoding='utf-8'))
def blob(commit,name): return subprocess.check_output(['git','-c',f'safe.directory={ROOT.as_posix()}','show',f'{commit}:resources/dictionary/{name}'],cwd=ROOT)
def rows(data):
    result=[]; category=''
    assert b'\r' not in data and not data.startswith(b'\xef\xbb\xbf')
    for line in data.decode('utf-8').splitlines():
        if line.startswith('# category:'): category=line.partition(':')[2].strip()
        elif line and not line.startswith('#'):
            key,word,frequency=line.split('\t')
            assert re.fullmatch('[a-z]+',key) and word and frequency.isdigit()
            result.append((category,key,word,int(frequency)))
    assert len(result)==len({(key,word) for _,key,word,_ in result})
    return result
@lru_cache(maxsize=None)
def choices(char): return tuple(pinyin(char,style=Style.NORMAL,heteronym=True,strict=True)[0])
def default(word): return ''.join(lazy_pinyin(word,style=Style.NORMAL,strict=True))


def repaired_import():
    report=read(ROOT/'docs/upstream-review-076.json')
    data=blob(REMOTE_COMMIT,'local_core_zh_pinyin.tsv')
    assert digest(data)==report['canonical_before_sha256']
    assert sha(ROOT/'tools/apply_upstream_review_076.py')==report['review_tool_sha256']
    changes={item['word']:item for item in report['semantic_or_spelling_key_corrections']}
    originals={(key,word) for _,key,word,_ in rows(data)}
    output=[]; drops=[]; applied=set()
    for line in data.decode('utf-8').splitlines():
        if line and not line.startswith('#'):
            key,word,frequency=line.split('\t')
            change=changes.get(word)
            if change and key==change['before']:
                assert word not in applied; applied.add(word); key=change['after']
                if (key,word) in originals:
                    drops.append((key,word)); continue
                line='\t'.join((key,word,frequency))
        output.append(line)
    assert applied==set(changes)
    assert set(drops)=={(item['key'],item['word']) for item in report['corrected_import_duplicates_removed']}
    original_rows={word:(key,frequency,category) for category,key,word,frequency in rows(data)}
    for alias in report['legitimate_or_regional_aliases']:
        word=alias['word']; key,frequency,category=original_rows[word]
        assert key==alias['original_key_retained']
        insertion=max(i for i,line in enumerate(output) if line=='# category: '+category)+1
        output.insert(insertion,f"{alias['added_key']}\t{word}\t{frequency}")
    result=('\n'.join(output)+'\n').encode('utf-8')
    assert digest(result)==report['canonical_after_sha256']
    return result


def audit_batch(number,current,other,fixture_hashes,answers):
    prefix=OUT/f'lexicon_batch_{number:03d}'
    meta=read(prefix.with_suffix('.json')); applied=read(prefix.with_suffix('.applied.json'))
    batch=prefix.with_suffix('.tsv'); data=batch.read_bytes(); frozen=rows(data)
    assert digest(data)==meta['sha256']==applied['frozen_batch_sha256']
    assert sha(ROOT/'tools/curate_lexicon.py')==meta['authoring_tool_sha256']
    for name,expected in meta['supporting_tools_sha256'].items(): assert sha(ROOT/name)==expected
    for name,expected in meta['authoring_files_sha256'].items(): assert sha(OUT/name)==expected
    assert meta['benchmark_sha256']==fixture_hashes and meta['benchmark_full_answers_added']==0
    assert meta['dedup_baseline_sha256']=={name:digest(current) if name=='local_core_zh_pinyin.tsv' else digest(other[name]) for name in LAYERS}
    assert digest(current)==applied['canonical_before_sha256']
    config=read(prefix.with_suffix('.authoring.json'))
    assert config['categories']==meta['preset_category_weights'] and set(config['categories'].values())<={5000,5500,6000}
    known={word for _,_,word,_ in rows(current)}|{word for name in ('core_zh_pinyin.tsv','standard_zh_pinyin.tsv') for _,_,word,_ in rows(other[name])}
    selected={}; category=''
    for line in (OUT/config['draft']).read_text(encoding='utf-8').splitlines():
        if line.startswith('# category:'): category=line.partition(':')[2].strip()
        elif line and not line.startswith('#'):
            assert category in config['categories']
            for word in line.split():
                assert re.fullmatch(r'[\u4e00-\u9fff]{2,6}',word)
                if word not in selected and word not in known and word not in answers and word not in config.get('quality_exclusions',{}):
                    selected[word]=(category,default(word),config['categories'][category])
    required={word for word in selected if any(len(choices(char))>1 for char in word)}
    reviewed={}; overrides={}; variants={}
    for line in (OUT/config['review']).read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'): continue
        key,word,author,positions,decision=line.split('\t')
        assert word in required and word not in reviewed and author and decision
        values=positions.split(';'); assert len(values)==len(word)
        syllables=[]
        for index,(char,value) in enumerate(zip(word,values),1):
            marker=f'{index}:{char}='; assert value.startswith(marker)
            syllable=value[len(marker):]; assert syllable in choices(char); syllables.append(syllable)
        assert ''.join(syllables)==key
        reviewed[word]=key
        if key!=default(word):
            assert decision.startswith(('semantic:','variant:'))
            overrides[word]=key
            if decision.startswith('variant:'): variants[word]=key
    assert set(reviewed)==required and len(required)==meta['mandatory_polyphone_reviews']
    assert overrides==meta['manual_pronunciation_overrides'] and variants==meta['reading_variant_choices']
    expected={(category,reviewed.get(word,key),word,frequency) for word,(category,key,frequency) in selected.items()}
    assert expected==set(frozen) and len(frozen)==len(selected)==meta['new_entries']
    result=current.decode('utf-8').rstrip()+'\n\n# category: '+prefix.name+'\n'
    result+=''.join(f'{key}\t{word}\t{frequency}\n' for _,key,word,frequency in frozen)
    result=result.encode('utf-8')
    assert digest(result)==applied['canonical_after_sha256']
    return result,{'batch':prefix.name,'entries':len(frozen),'contextual_reviews':len(required),'semantic_corrections':len(overrides)-len(variants),'legitimate_main_reading_choices':len(variants),'frozen_sha256':digest(data),'authoring_and_review_hashes_verified':True,'existing_display_aliases_added':0,'complete_public_answers_added':0,'preset_weights_verified':True,'editorial_review_scope':config['review_scope']}


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--batches',nargs='+',type=int,required=True); a=parser.parse_args()
    assert pypinyin.__version__=='0.55.0'
    fixtures={file.name:sha(file) for file in sorted((ROOT/'tests/data').glob('typing_*.tsv'))}
    answers={line.split('\t')[1] for name in fixtures for line in (ROOT/'tests/data'/name).read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')}
    old={name:blob(BASE_COMMIT,name) for name in RUNTIME}; other={name:(DICT/name).read_bytes() for name in RUNTIME}
    assert all(old[name]==other[name] for name in RUNTIME if name!='local_core_zh_pinyin.tsv')
    current=repaired_import(); imported_unique=len({(k,w) for name in LAYERS for _,k,w,_ in rows(current if name=='local_core_zh_pinyin.tsv' else other[name])})
    results=[]
    for number in a.batches:
        current,result=audit_batch(number,current,other,fixtures,answers); results.append(result)
    assert current==other['local_core_zh_pinyin.tsv'],'Runtime differs from exact reviewed import plus frozen merges'
    old_rows={f'{k}\t{w}\t{f}' for _,k,w,f in rows(old['local_core_zh_pinyin.tsv'])}
    assert old_rows<={f'{k}\t{w}\t{f}' for _,k,w,f in rows(current)}
    registry=read(DICT/'sources/lexicon_sources.json'); prior=json.loads(blob(BASE_COMMIT,'sources/lexicon_sources.json'))
    assert registry['sources'][:len(prior['sources'])]==prior['sources']
    ids={item['source_id'] for item in registry['sources']}
    metadata=read(DICT/'local_core_zh_pinyin.metadata.json')
    legacy=set(json.loads(blob(BASE_COMMIT,'local_core_zh_pinyin.metadata.json'))['source_ids'])-{item['source_id'] for item in prior['sources']}
    assert set(metadata['source_ids'])-ids==legacy
    assert metadata['source_policy'].startswith(json.loads(blob(BASE_COMMIT,'local_core_zh_pinyin.metadata.json'))['source_policy'])
    assert metadata['entry_count']==len(rows(current)) and metadata['sha256'].lower()==digest(current)
    for n in (20,21,22,23):
        source=next(item for item in registry['sources'] if item['source_id']==f'upstream_contributed_lexicon_batch_{n:03d}')
        assert source['review_status']=='technical_verified_provenance_incomplete' and source['commercial_use_allowed'] is None and source['redistribution_allowed'] is None
    total=len({(k,w) for name in LAYERS for _,k,w,_ in rows(other[name])})
    report={'version':'0.7.6','status':'passed','audited_at_utc':datetime.now(timezone.utc).isoformat(),'base_commit':BASE_COMMIT,'upstream_commit':REMOTE_COMMIT,'audit_tool_sha256':sha(Path(__file__)),
            'method':'Separate read-only reconstruction from immutable Git, finite upstream decisions, frozen category presets and positional pronunciation records. This is structural auditing, not a new linguistic editor.',
            'runtime_sha256':{name:digest(data) for name,data in other.items()},'source_registry_sha256':sha(DICT/'sources/lexicon_sources.json'),'typing_fixture_sha256':fixtures,'public_complete_answers_excluded':len(answers),
            'current_local_core_entries':len(rows(current)),'current_three_layer_unique_pairs':total,'reviewed_import_unique_pairs':imported_unique,'authored_entries':sum(item['entries'] for item in results),
            'baseline_local_rows_and_weights_preserved':True,'all_prior_source_records_preserved':True,'upstream_permissions_not_invented':True,'legacy_unregistered_ids':sorted(legacy),'contextual_review_membership_verified':True,
            'independent_linguistic_review':{'status':'not_performed','scope':'Single AI author read all selected displays and main keys and provided contextual decisions. No second human/AI editorial review is claimed. Native input quality is measured separately.'},'batches':results}
    output=ROOT/'docs/lexicon-076-authoring-audit.json'; output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    lines=['# 0.7.6 作者数据结构与来源审计','',f"只读重建通过。有效词{total:,}，本轮原创词{report['authored_entries']:,}。三份公开测试267个完整答案逐项排除。",'',
           '固定Git上游经过有限读音修订后，与冻结草稿、类别预设权重及逐字位置读音记录重建的每批追加字节完全一致。原0.7.5本地记录、权重和旧来源条款保留；新增本地覆盖对既有跨层有效最大权重的影响另见输入质量报告。','',
           '本轮是单一AI作者逐词语境复核，加上独立代码路径的结构校验；没有第二名人类或AI语言编辑，不把结构通过等同于每个词的现实使用频率或通用整句准确率。上游专名的语言学复核仍为有限范围，不宣称整批官方音得到全审。','',
           '| 批次 | 新词 | 语境读音记录 | 语义修正 | 合法主读音选择 |','| --- | ---: | ---: | ---: | ---: |']
    lines += [f"| {item['batch']} | {item['entries']} | {item['contextual_reviews']} | {item['semantic_corrections']} | {item['legitimate_main_reading_choices']} |" for item in results]
    lines += ['', '完整SHA、源文件及范围：[机器报告](lexicon-076-authoring-audit.json)。原生真实候选和公开句子对照由发布流程另测，数据文件和作者工具不会进入输入法运行依赖。','']
    (ROOT/'docs/lexicon-076-authoring-audit.md').write_text('\n'.join(lines),encoding='utf-8',newline='\n')
    print(json.dumps({'status':'passed','batches':len(results),'authored':report['authored_entries'],'total':total},ensure_ascii=False))


if __name__=='__main__': main()
