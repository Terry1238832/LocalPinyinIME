#!/usr/bin/env python3
"""Measure both original imported keys and corrected/alternate final input keys.

Keeps the original immutable snapshot audit intact. Uses isolated empty personal
data, does not learn expected answers, and checks all five final resource hashes.
"""
import argparse,json,hashlib
from datetime import datetime,timezone
from pathlib import Path
import tempfile
from audit_upstream_snapshot import coverage,write
from audit_remote_lexicon_074 import git_blob,parse,pair,NAMES

ROOT=Path(__file__).absolute().parents[1]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__); p.add_argument('--engine',type=Path,required=True); p.add_argument('--resources',type=Path,required=True); a=p.parse_args()
    snapshot=json.loads((ROOT/'docs/remote-lexicon-audit-076.json').read_text(encoding='utf-8'))
    review=json.loads((ROOT/'docs/upstream-review-076.json').read_text(encoding='utf-8'))
    old={pair(row) for row in parse(git_blob(snapshot['base_commit'],'resources/dictionary/local_core_zh_pinyin.tsv'))[0]}
    raw=[row for row in parse(git_blob(snapshot['remote_commit'],'resources/dictionary/local_core_zh_pinyin.tsv'))[0] if pair(row) not in old]
    changes={row['word']:row for row in review['semantic_or_spelling_key_corrections']}
    resolved=[]
    for row in raw:
        item=dict(row); correction=changes.get(item['word'])
        if correction and item['pinyin']==correction['before']: item['pinyin']=correction['after']
        resolved.append(item)
    by_word={row['word']:row for row in raw}
    resolved += [{**by_word[row['word']],'pinyin':row['added_key']} for row in review['legitimate_or_regional_aliases']]
    unique={}
    for row in resolved: unique.setdefault(pair(row),row)
    resolved=list(unique.values())
    resources=a.resources.absolute(); engine=a.engine.absolute(); engine_sha=sha(engine)
    runtime={name:sha(resources/'dictionary'/name) for name in NAMES}
    assert runtime=={name:sha(ROOT/'resources/dictionary'/name) for name in NAMES}
    results={}
    with tempfile.TemporaryDirectory(prefix='upstream-final076-',dir=ROOT/'.build') as directory:
        for label,cases in (('original_imported_keys',raw),('resolved_keys_including_regional_aliases',resolved)):
            # The helper writes cases/private beside resources; copy bytes to the
            # owned temporary directory so production resources stay read-only.
            temporary=Path(directory)/label; (temporary/'dictionary').mkdir(parents=True)
            for name in NAMES: (temporary/'dictionary'/name).write_bytes((resources/'dictionary'/name).read_bytes())
            measured,rows=coverage(engine,temporary,cases)
            output=ROOT/f'.build/upstream-final076-{label}.json'; write(output,rows)
            measured['full_output_sha256']=sha(output); results[label]=measured
    assert sha(engine)==engine_sha and runtime=={name:sha(resources/'dictionary'/name) for name in NAMES}
    report={'version':'0.7.6','evaluated_at_utc':datetime.now(timezone.utc).isoformat(),'engine_sha256':engine_sha,'runtime_sha256':runtime,
            'snapshot_audit_sha256':sha(ROOT/'docs/remote-lexicon-audit-076.json'),'finite_review_sha256':sha(ROOT/'docs/upstream-review-076.json'),
            'personal_data_isolated':True,'expected_answers_learned':False,'original_key_scope':'Raw imported keys include 25 spelling/semantic keys deliberately corrected or deduplicated. Their legacy-key rank is not final preferred-reading coverage.',
            'resolved_key_scope':'Finite reviewed corrections and five explicit regional/ordinary aliases, deduplicated by key/display. Stored-key presence is not formal official phonetic verification of every name.',**results}
    write(ROOT/'docs/upstream-current-076.json',report)
    print(json.dumps({label:value['summary'] for label,value in results.items()},ensure_ascii=False))
if __name__=='__main__': main()
